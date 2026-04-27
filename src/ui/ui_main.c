/*
 * ui_main.c — Orion-UI integration for OpenWarcraft3.
 *
 * Provides five windows arranged in a vertical stack:
 *   1. Top-bar Window  — thin strip that draws the top resource/status bar.
 *   2. Game Window     — displays the 3-D game scene rendered directly into
 *                        the default framebuffer using glViewport for
 *                        positioning.  All keyboard / mouse input aimed at
 *                        the game is routed through this window's proc.
 *   3. Bottom-bar Window — wide strip that draws command buttons, minimap,
 *                          unit-info panel, etc.
 *   4. Log Window      — shows CON_printf() output in a read-only text area.
 *   5. Map Selector    — lists *.w3m files found in the loaded MPQ archive
 *                        and lets the player pick a map to start.
 *
 * Layout (logical pixels, x=20 left-edge):
 *   y=20  : top-bar   (UI_GAME_W × UI_TOP_BAR_H)
 *   y=32  : game      (UI_GAME_W × UI_GAME_H)
 *   y=488 : bottom-bar (UI_GAME_W × UI_BOTTOM_BAR_H)
 *
 * The game window is now sized to match exactly the 3-D scene area that was
 * previously exposed via glScissor({0, 0.22, 1, 0.76}).  By sizing the window
 * to the desired 3-D area the renderer no longer needs a sub-viewport scissor
 * inside R_RenderView; the OS window boundary provides the clipping instead.
 *
 * NOTE: This file intentionally does NOT include any game headers (client.h,
 * common.h, …) because the game's rect_t typedef conflicts with orion-ui's
 * own rect_t.  All interaction with game code is done through plain C
 * forward declarations that use only primitive types.
 */

#include "../../vendor/orion-ui/ui.h"
#include "../../vendor/orion-ui/commctl/commctl.h"
#include "../../vendor/orion-ui/user/rect.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* ── Constants ───────────────────────────────────────────────────────────── */

#define MAX_MAP_PATH         260   // maximum MPQ-internal path length
#define MAX_MAP_DISPLAY_NAME  64   // combobox_string_t is 64 bytes
#define MAX_MAPS             256
#define LOG_BUF_SIZE (64 * 1024)

// Game-area layout: these match the scissor {0, 0.22, 1, 0.76} that was
// previously applied to the 800×600 game window.
//   top-bar   height = (1.0 - 0.22 - 0.76) * 600 = 0.02 * 600 = 12 px
//   game area height =               0.76   * 600 = 456 px
//   bottom-bar height =              0.22   * 600 = 132 px
#define UI_GAME_W       800
#define UI_TOP_BAR_H     12
#define UI_GAME_H       456
#define UI_BOTTOM_BAR_H 132

// Vertical positions of each window (x=20 is the shared left edge).
#define UI_WIN_X           20
#define UI_TOP_BAR_Y       20
#define UI_GAME_Y         (UI_TOP_BAR_Y + UI_TOP_BAR_H)   // 32
#define UI_BOTTOM_BAR_Y   (UI_GAME_Y   + UI_GAME_H)        // 488

// UI ortho Y sub-ranges (0-0.6 space, y=0 at top, y=0.6 at bottom).
//   top-bar   : 0       to UI_TOP_Y_END
//   game area : UI_TOP_Y_END to UI_BOTTOM_Y_START
//   bottom-bar: UI_BOTTOM_Y_START to 0.6f
#define UI_TOP_Y_END       0.012f   // 12  / 1000 (1 unit = 1000 px at 800×600)
#define UI_BOTTOM_Y_START  0.468f   // 468 / 1000

/* ── Forward declarations from game code ─────────────────────────────────── */

// server / map loading
extern void SV_Map(const char *map_path);

// input bridge — implemented in cl_input.c
extern void CL_MouseButtonDown(int button, float x, float y, unsigned int time);
extern void CL_MouseButtonUp(int button, float x, float y, unsigned int time);
extern void CL_MouseMove(float x, float y, float dx, float dy);
extern void CL_KeyDown(unsigned char key, unsigned int time);
extern void CL_KeyUp(unsigned char key, unsigned int time);

// screen update — renders the current frame directly into the default framebuffer
extern void SCR_UpdateScreen(void);
// UI-bar overlay renders (implemented in cl_scrn.c)
extern void SCR_DrawTopBar(void);
extern void SCR_DrawBottomBar(void);

// renderer — sets the GL viewport position used by the game renderer
extern void R_SetGameViewport(int x, int y, int w, int h);
// renderer — sets the active UI ortho Y sub-range for R_DrawImageEx etc.
extern void R_SetUIRange(float y_start, float y_end);
// renderer — lightweight frame begin/end for UI-only (bar) windows
extern void R_BeginUIFrame(void);
extern void R_EndFrame(void);

// console hook — register a callback to receive every CON_printf() message
typedef void (*con_log_hook_t)(const char *msg);
extern void CON_SetLogHook(con_log_hook_t hook);

// map listing — fills an array of 260-char paths, returns count
extern int FS_ListMaps(char (*out)[MAX_MAP_PATH], int max_maps);

// timing — axGetMilliseconds() from platform
extern unsigned long axGetMilliseconds(void);

/* ── Window handles ──────────────────────────────────────────────────────── */

static window_t *g_topbar_win = NULL;
static window_t *g_game_win   = NULL;
static window_t *g_bottombar_win = NULL;
static window_t *g_log_win    = NULL;
static window_t *g_map_win    = NULL;
static window_t *g_log_edit   = NULL;   // multiedit inside log window
static window_t *g_map_combo  = NULL;   // combobox inside map selector

/* ── Map list storage ────────────────────────────────────────────────────── */

static char g_map_paths[MAX_MAPS][MAX_MAP_PATH];
static int  g_num_maps = 0;

/* ── Log window ──────────────────────────────────────────────────────────── */

static char g_log_buf[LOG_BUF_SIZE];
static int  g_log_len = 0;

// Appended to the multiedit control each time CON_printf fires.
static void on_log_message(const char *msg) {
    int len = (int)strlen(msg);

    // Trim oversized messages so they always fit in an empty buffer.
    if (len > LOG_BUF_SIZE - 2) {
        msg += len - (LOG_BUF_SIZE - 2);
        len = LOG_BUF_SIZE - 2;
    }

    // If we would overflow, discard old text.  Drop at least half the
    // buffer, but always enough to fit the new message + newline + NUL.
    if (g_log_len + len + 2 >= LOG_BUF_SIZE) {
        int half = LOG_BUF_SIZE / 2;
        int drop = g_log_len + len + 2 - LOG_BUF_SIZE;
        int move_len;

        if (drop < half)
            drop = half;
        if (drop > g_log_len)
            drop = g_log_len;

        move_len = g_log_len - drop;
        if (move_len > 0)
            memmove(g_log_buf, g_log_buf + drop, (size_t)move_len);
        g_log_len = move_len;
    }

    memcpy(g_log_buf + g_log_len, msg, (size_t)len);
    g_log_len += len;
    g_log_buf[g_log_len++] = '\n';
    g_log_buf[g_log_len]   = '\0';

    if (g_log_edit)
        send_message(g_log_edit, edSetText, 0, g_log_buf);
}

static result_t win_log_proc(window_t *win, uint32_t msg,
                              uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evCreate: {
            rect_t client = get_client_rect(win);
            g_log_edit = create_window("", WINDOW_NOTITLE | WINDOW_VSCROLL,
                                       MAKERECT(0, 0, client.w, client.h),
                                       win, win_multiedit, 0, NULL);
            return 1;
        }
        case evResize: {
            if (g_log_edit) {
                rect_t client = get_client_rect(win);
                move_window(g_log_edit, 0, 0);
                resize_window(g_log_edit, client.w, client.h);
            }
            return 0;
        }
        case evDestroy:
            g_log_edit = NULL;
            g_log_win  = NULL;
            return 0;
        default:
            return 0;
    }
}

/* ── Map selector ────────────────────────────────────────────────────────── */

static result_t win_map_proc(window_t *win, uint32_t msg,
                              uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evCreate: {
            rect_t client = get_client_rect(win);

            // Combobox occupies the upper portion (30px high).
            g_map_combo = create_window("Select map…", WINDOW_NOTITLE,
                                        MAKERECT(4, 4, client.w - 8, 20),
                                        win, win_combobox, 0, NULL);

            // Populate with discovered .w3m files.
            if (g_map_combo) {
                for (int i = 0; i < g_num_maps; i++) {
                    // Truncate to combobox_string_t capacity (64 bytes including NUL).
                    char display[MAX_MAP_DISPLAY_NAME];
                    strncpy(display, g_map_paths[i], MAX_MAP_DISPLAY_NAME - 1);
                    display[MAX_MAP_DISPLAY_NAME - 1] = '\0';
                    post_message(g_map_combo, cbAddString, 0, display);
                }
            }

            // "Load" button below the combobox.
            window_t *btn = create_window("Load", WINDOW_NOTITLE,
                                          MAKERECT(client.w / 2 - 40, 30, 80, 20),
                                          win, win_button, 0, NULL);
            if (btn) btn->id = 1;

            return 1;
        }
        case evCommand: {
            if (LOWORD(wparam) == 1 && HIWORD(wparam) == btnClicked) {
                // "Load" clicked.
                if (!g_map_combo) break;
                result_t sel = send_message(g_map_combo, cbGetCurrentSelection, 0, NULL);
                if (sel >= 0 && sel < g_num_maps) {
                    SV_Map(g_map_paths[(int)sel]);
                    destroy_window(win);
                }
            }
            return 1;
        }
        case evDestroy:
            g_map_combo = NULL;
            g_map_win   = NULL;
            return 0;
        default:
            return 0;
    }
    return 0;
}

/* ── Top-bar window ──────────────────────────────────────────────────────── */

// Physical-pixel GL rect for a logical UI rect (handles HiDPI/retina scaling
// and Y-axis flip to GL bottom-left origin).
// Defined in vendor/orion-ui/user/draw_impl.c.
extern rect_t get_opengl_rect(rect_t const *r);

// Renders the thin resource/status bar at the top of the game area.
// The full 0-0.8 × 0-0.6 UI overlay is drawn with an ortho matrix clamped
// to the top-bar Y range [0, UI_TOP_Y_END] so only top-bar frames are visible.
static result_t win_topbar_proc(window_t *win, uint32_t msg,
                                uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evPaint: {
            rect_t client = get_client_rect(win);
            rect_t client_abs = {
                win->frame.x,
                win->frame.y + win->frame.h - client.h,
                client.w,
                client.h,
            };
            rect_t gl_rect = get_opengl_rect(&client_abs);
            R_SetGameViewport(gl_rect.x, gl_rect.y, gl_rect.w, gl_rect.h);
            R_SetUIRange(0.0f, UI_TOP_Y_END);
            R_BeginUIFrame();
            SCR_DrawTopBar();
            R_EndFrame();
            return 1;
        }
        case evDestroy:
            g_topbar_win = NULL;
            return 0;
        default:
            return 0;
    }
}

/* ── Bottom-bar window ───────────────────────────────────────────────────── */

// Renders the command-button panel, minimap, and unit-info strip.
// The ortho matrix is clamped to [UI_BOTTOM_Y_START, 0.6] so only bottom-bar
// frames are visible.  Mouse events are forwarded with coordinates adjusted
// to the full game-window space so hit-testing in SCR_DrawCommandButton etc.
// works correctly.
static result_t win_bottombar_proc(window_t *win, uint32_t msg,
                                   uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evPaint: {
            rect_t client = get_client_rect(win);
            rect_t client_abs = {
                win->frame.x,
                win->frame.y + win->frame.h - client.h,
                client.w,
                client.h,
            };
            rect_t gl_rect = get_opengl_rect(&client_abs);
            R_SetGameViewport(gl_rect.x, gl_rect.y, gl_rect.w, gl_rect.h);
            R_SetUIRange(UI_BOTTOM_Y_START, 0.6f);
            R_BeginUIFrame();
            SCR_DrawBottomBar();
            R_EndFrame();
            return 1;
        }
        case evMouseMove: {
            int16_t lx  = (int16_t)LOWORD(wparam);
            int16_t ly  = (int16_t)HIWORD(wparam);
            int16_t rdx = (int16_t)LOWORD((uint32_t)(intptr_t)lparam);
            int16_t rdy = (int16_t)HIWORD((uint32_t)(intptr_t)lparam);
            // Translate bar-local y into full virtual-coordinate y so that
            // UI hit-testing is consistent: bottom bar starts at
            // UI_TOP_BAR_H + UI_GAME_H pixels below the top of the layout.
            CL_MouseMove((float)lx,
                         (float)(UI_TOP_BAR_H + UI_GAME_H + ly),
                         (float)rdx, (float)rdy);
            invalidate_window(win);
            return 1;
        }
        case evLeftButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(1, (float)lx, (float)(UI_TOP_BAR_H + UI_GAME_H + ly),
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evLeftButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(1, (float)lx, (float)(UI_TOP_BAR_H + UI_GAME_H + ly),
                             (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(3, (float)lx, (float)(UI_TOP_BAR_H + UI_GAME_H + ly),
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(3, (float)lx, (float)(UI_TOP_BAR_H + UI_GAME_H + ly),
                             (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evDestroy:
            g_bottombar_win = NULL;
            return 0;
        default:
            return 0;
    }
}

/* ── Game window ─────────────────────────────────────────────────────────── */

static result_t win_game_proc(window_t *win, uint32_t msg,
                               uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evPaint: {
            rect_t client = get_client_rect(win);
            // Build the absolute logical rect for the client area.
            // frame.y + (frame.h - client.h) == frame.y + titlebar_height.
            rect_t client_abs = {
                win->frame.x,
                win->frame.y + win->frame.h - client.h,
                client.w,
                client.h,
            };
            // get_opengl_rect converts logical → physical framebuffer pixels
            // and flips Y to GL's bottom-left origin, handling HiDPI/retina.
            rect_t gl_rect = get_opengl_rect(&client_abs);
            R_SetGameViewport(gl_rect.x, gl_rect.y, gl_rect.w, gl_rect.h);
            SCR_UpdateScreen();
            return 1;
        }

        case evMouseMove: {
            int16_t lx  = (int16_t)LOWORD(wparam);
            int16_t ly  = (int16_t)HIWORD(wparam);
            int16_t rdx = (int16_t)LOWORD((uint32_t)(intptr_t)lparam);
            int16_t rdy = (int16_t)HIWORD((uint32_t)(intptr_t)lparam);
            CL_MouseMove((float)lx, (float)ly, (float)rdx, (float)rdy);
            invalidate_window(win);
            return 1;
        }

        case evLeftButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(1, (float)lx, (float)ly,
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evLeftButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(1, (float)lx, (float)ly,
                             (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(3, (float)lx, (float)ly,
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(3, (float)lx, (float)ly,
                             (unsigned int)axGetMilliseconds());
            return 1;
        }

        case evKeyDown:
            CL_KeyDown((unsigned char)wparam, (unsigned int)axGetMilliseconds());
            return 1;
        case evKeyUp:
            CL_KeyUp((unsigned char)wparam, (unsigned int)axGetMilliseconds());
            return 1;

        case evDestroy:
            g_game_win = NULL;
            ui_request_quit();
            return 0;

        default:
            return 0;
    }
}

/* ── Public API ──────────────────────────────────────────────────────────── */

// UI_Init — create all UI windows and register the CON_printf hook.
// Must be called AFTER the MPQ archives are open (for FS_ListMaps) but
// BEFORE Com_Init() (which initialises the renderer using the GL context
// created here by ui_init_graphics).
void UI_Init(void) {
    // Create the platform window + OpenGL context.
    // Width / height are the virtual coordinate space; the physical OS window
    // is width * UI_WINDOW_SCALE x height * UI_WINDOW_SCALE pixels.
    ui_init_graphics(UI_INIT_DESKTOP, "OpenWarcraft3", 1176, 720);

    // Top-bar window: draws the resource/status strip at the top of the game
    // area.  WINDOW_NOTITLE keeps it chrome-free so it appears flush with the
    // game window below it.
    g_topbar_win = create_window("",
                                 WINDOW_NOTITLE | WINDOW_NORESIZE | WINDOW_NOFILL,
                                 MAKERECT(UI_WIN_X, UI_TOP_BAR_Y,
                                          UI_GAME_W, UI_TOP_BAR_H),
                                 NULL, win_topbar_proc, 0, NULL);
    if (g_topbar_win)
        show_window(g_topbar_win, true);

    // Game window: renders the 3-D scene directly into the default framebuffer
    // using glViewport.  Sized to exactly the 3-D area so no sub-viewport
    // scissor is required inside the renderer.
    g_game_win = create_window("OpenWarcraft3",
                               WINDOW_NORESIZE | WINDOW_NOFILL,
                               MAKERECT(UI_WIN_X, UI_GAME_Y,
                                        UI_GAME_W, UI_GAME_H),
                               NULL, win_game_proc, 0, NULL);
    if (g_game_win) {
        show_window(g_game_win, true);
        set_focus(g_game_win);
    }

    // Bottom-bar window: draws command buttons, minimap and unit-info panel.
    g_bottombar_win = create_window("",
                                    WINDOW_NOTITLE | WINDOW_NORESIZE | WINDOW_NOFILL,
                                    MAKERECT(UI_WIN_X, UI_BOTTOM_BAR_Y,
                                             UI_GAME_W, UI_BOTTOM_BAR_H),
                                    NULL, win_bottombar_proc, 0, NULL);
    if (g_bottombar_win)
        show_window(g_bottombar_win, true);

    // Log window: small console that mirrors CON_printf output.
    g_log_win = create_window("Log",
                              0,
                              MAKERECT(840, 20, 320, 400),
                              NULL, win_log_proc, 0, NULL);
    if (g_log_win)
        show_window(g_log_win, true);

    // Map selector: lists *.w3m files from the MPQ.
    g_num_maps = FS_ListMaps(g_map_paths, MAX_MAPS);
    if (g_num_maps > 0) {
        g_map_win = create_window("Select Map",
                                  WINDOW_NORESIZE,
                                  MAKERECT(840, 440, 320, 120),
                                  NULL, win_map_proc, 0, NULL);
        if (g_map_win)
            show_window(g_map_win, true);
    }

    // Hook CON_printf so messages are mirrored to the log window.
    CON_SetLogHook(on_log_message);
}

// UI_IsRunning — returns non-zero while the event loop should keep going.
int UI_IsRunning(void) {
    return ui_is_running() ? 1 : 0;
}

// UI_ProcessEvents — drain the platform event queue and repaint windows.
// Must be called once per game tick, after SV_Frame() and CL_Frame().
void UI_ProcessEvents(void) {
    // Invalidate all three game-area windows each frame so evPaint fires and
    // the new frame is rendered directly into the default framebuffer.
    if (g_topbar_win)
        invalidate_window(g_topbar_win);
    if (g_game_win)
        invalidate_window(g_game_win);
    if (g_bottombar_win)
        invalidate_window(g_bottombar_win);

    ui_event_t evt;
    while (get_message(&evt))
        dispatch_message(&evt);
    repost_messages();
}

// UI_Shutdown — tear down the platform context.
void UI_Shutdown(void) {
    CON_SetLogHook(NULL);
    ui_shutdown_graphics();
}
