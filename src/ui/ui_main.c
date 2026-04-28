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
 * Layout (logical pixels, x=20 left-edge, default values):
 *   y=20  : top-bar    (UI_GAME_W × g_top_bar_h)    — default 40 px
 *   y=60  : game       (UI_GAME_W × g_game_h)        — default 428 px
 *   y=488 : bottom-bar (UI_GAME_W × g_bottom_bar_h)  — default 132 px
 *
 * Bar heights are dynamic: after the server runs UI_Init() and knows the
 * actual FDF frame heights, it writes CS_VIEWPORT = "game_y game_h" and the
 * client calls UI_SetViewport() to resize/reposition the windows accordingly.
 *
 * Each bar window uses R_BeginBarFrame / R_EndBarFrame which restrict GL
 * drawing to the bar's own pixel region via scissor.  This keeps the 3-D game
 * viewport (tr.game_x/y/w/h) exclusively owned by the game window, and
 * eliminates the need for a narrowed ortho projection to clip bar frames.
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

// Default 800x600 gameplay layout used before CS_VIEWPORT arrives:
//   top-bar    40 px (includes the WC3 day/night clock)
//   game area 428 px
//   bottom-bar 132 px
//
// At runtime these are updated by UI_SetViewport() when CS_VIEWPORT arrives
// from the server, so the server's FDF-parsed bar heights drive the layout.
#define UI_GAME_W       800

// Vertical positions of each window (x=20 is the shared left edge).
#define UI_WIN_X           20
#define UI_TOP_BAR_Y       20

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
extern void SCR_ProcessOverlays(void);

// renderer — sets the GL viewport position used by the game renderer
extern void R_SetGameViewport(int x, int y, int w, int h);
// renderer — frame begin/end for UI bar windows (does NOT touch game viewport)
extern void R_BeginBarFrame(int x, int y, int w, int h);
extern void R_EndBarFrame(void);

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

/* ── Dynamic layout globals ──────────────────────────────────────────────── */
// Updated by UI_SetViewport() when CS_VIEWPORT is received from the server.
// Default heights (pixels): top=40 (WC3 resource bar + day/night clock),
// game=428 (3-D area), bottom=132 (console panel).
// top_h + game_h + bottom_h must equal 600 (the WC3 virtual screen height).
static int g_top_bar_h    = 40;    // logical pixels
static int g_game_h       = 428;   // logical pixels
static int g_bottom_bar_h = 132;   // logical pixels

static rect_t make_window_frame_rect(int x, int y, int w, int h, flags_t flags)
{
    rect_t frame = { 0, 0, w, h };
    adjust_window_rect(&frame, flags);
    frame.x += x;
    frame.y += y;
    return frame;
}

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

// Convert a bar-local y coordinate to the full virtual-screen y used by the
// game input system.  The bar window's logical frame origin (minus the game
// area's logical top, UI_TOP_BAR_Y) gives the correct offset without relying
// on sibling-window height constants.
static int16_t to_game_y(window_t *win, int16_t local_y)
{
    return (int16_t)(local_y + win->frame.y - UI_TOP_BAR_Y);
}

// Helper: compute the absolute GL rect for a window's client area.
static rect_t client_gl_rect(window_t *win)
{
    rect_t client = get_client_rect(win);
    rect_t client_abs = {
        win->frame.x,
        win->frame.y + win->frame.h - client.h,
        client.w,
        client.h,
    };
    return get_opengl_rect(&client_abs);
}

// Renders the thin resource/status bar at the top of the game area.
// R_BeginBarFrame sets the scissor to this bar's pixel rect so only frames
// that fall within it are visible.  The game viewport (tr.game_x/y/w/h) is
// not touched; it remains owned by the game window.
static result_t win_topbar_proc(window_t *win, uint32_t msg,
                                uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evPaint: {
            rect_t gl = client_gl_rect(win);
            R_BeginBarFrame(gl.x, gl.y, gl.w, gl.h);
            SCR_DrawTopBar();
            R_EndBarFrame();
            return 1;
        }
        case evMouseMove: {
            int16_t lx  = (int16_t)LOWORD(wparam);
            int16_t ly  = (int16_t)HIWORD(wparam);
            int16_t rdx = (int16_t)LOWORD((uint32_t)(intptr_t)lparam);
            int16_t rdy = (int16_t)HIWORD((uint32_t)(intptr_t)lparam);
            CL_MouseMove((float)lx, (float)to_game_y(win, ly),
                         (float)rdx, (float)rdy);
            invalidate_window(win);
            return 1;
        }
        case evLeftButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(1, (float)lx, (float)to_game_y(win, ly),
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evLeftButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(1, (float)lx, (float)to_game_y(win, ly),
                             (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(3, (float)lx, (float)to_game_y(win, ly),
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(3, (float)lx, (float)to_game_y(win, ly),
                             (unsigned int)axGetMilliseconds());
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
// R_BeginBarFrame clips drawing to this bar's pixel rect.
// Mouse events are translated from bar-local coordinates to the full virtual-
// screen space via to_game_y() so that hit-testing in SCR_DrawCommandButton
// etc. works correctly.
static result_t win_bottombar_proc(window_t *win, uint32_t msg,
                                   uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evPaint: {
            rect_t gl = client_gl_rect(win);
            R_BeginBarFrame(gl.x, gl.y, gl.w, gl.h);
            SCR_DrawBottomBar();
            R_EndBarFrame();
            return 1;
        }
        case evMouseMove: {
            int16_t lx  = (int16_t)LOWORD(wparam);
            int16_t ly  = (int16_t)HIWORD(wparam);
            int16_t rdx = (int16_t)LOWORD((uint32_t)(intptr_t)lparam);
            int16_t rdy = (int16_t)HIWORD((uint32_t)(intptr_t)lparam);
            CL_MouseMove((float)lx, (float)to_game_y(win, ly),
                         (float)rdx, (float)rdy);
            invalidate_window(win);
            return 1;
        }
        case evLeftButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(1, (float)lx, (float)to_game_y(win, ly),
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evLeftButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(1, (float)lx, (float)to_game_y(win, ly),
                             (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonDown: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonDown(3, (float)lx, (float)to_game_y(win, ly),
                               (unsigned int)axGetMilliseconds());
            return 1;
        }
        case evRightButtonUp: {
            int16_t lx = (int16_t)LOWORD(wparam);
            int16_t ly = (int16_t)HIWORD(wparam);
            CL_MouseButtonUp(3, (float)lx, (float)to_game_y(win, ly),
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
            rect_t gl = client_gl_rect(win);
            R_SetGameViewport(gl.x, gl.y, gl.w, gl.h);
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

// UI_SetViewport — called by the client when CS_VIEWPORT arrives from the
// server.  game_y and game_h are in WC3 ortho units (0–0.6); multiply by
// 1000 to convert to logical pixels (800×600 virtual screen).
// Repositions / resizes the three bar/game windows so the OS window boundary
// provides the correct clipping for each section.
void UI_SetViewport(float game_y, float game_h) {
    int top_h;
    int gh;
    int bottom_h;

    // game_y and game_h are in WC3 ortho units (0–0.6 range).
    // Virtual pixel height is 600, so 1 ortho unit = 1000 pixels.
    top_h = (int)(game_y * 1000.0f + 0.5f);
    gh    = (int)(game_h * 1000.0f + 0.5f);

    if (top_h < 1)   top_h = 1;   // minimum 1 px per bar
    if (gh    < 1)   gh    = 1;
    if (top_h > 598) top_h = 598; // leave room for game + bottom (each >= 1 px)
    if (gh    > 598) gh    = 598;

    if (top_h + gh > 599) {        // leave at least 1 px for bottom bar
        gh = 599 - top_h;
        if (gh < 1) { gh = 1; top_h = 598; }
    }

    bottom_h = 600 - top_h - gh;  // remaining pixels go to bottom bar

    g_top_bar_h    = top_h;
    g_game_h       = gh;
    g_bottom_bar_h = bottom_h;

    flags_t bar_flags = WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE;
    int game_y_px   = UI_TOP_BAR_Y + top_h;
    int bottom_y_px = game_y_px + gh;

    if (g_topbar_win) {
        rect_t f = make_window_frame_rect(UI_WIN_X, UI_TOP_BAR_Y,
                                          UI_GAME_W, top_h, bar_flags);
        move_window(g_topbar_win, f.x, f.y);
        resize_window(g_topbar_win, f.w, f.h);
        invalidate_window(g_topbar_win);
    }
    if (g_game_win) {
        rect_t f = make_window_frame_rect(UI_WIN_X, game_y_px,
                                          UI_GAME_W, gh, bar_flags);
        move_window(g_game_win, f.x, f.y);
        resize_window(g_game_win, f.w, f.h);
        invalidate_window(g_game_win);
    }
    if (g_bottombar_win) {
        rect_t f = make_window_frame_rect(UI_WIN_X, bottom_y_px,
                                          UI_GAME_W, bottom_h, bar_flags);
        move_window(g_bottombar_win, f.x, f.y);
        resize_window(g_bottombar_win, f.w, f.h);
        invalidate_window(g_bottombar_win);
    }
}

// UI_Init — create all UI windows and register the CON_printf hook.
// Must be called AFTER the MPQ archives are open (for FS_ListMaps) but
// BEFORE Com_Init() (which initialises the renderer using the GL context
// created here by ui_init_graphics).
void UI_Init(void) {
    rect_t frame;

    // Create the platform window + OpenGL context.
    // Width / height are the virtual coordinate space; the physical OS window
    // is width * UI_WINDOW_SCALE x height * UI_WINDOW_SCALE pixels.
    ui_init_graphics(UI_INIT_DESKTOP, "OpenWarcraft3", 1176, 720);

    // Top-bar window: chrome-free strip that draws the resource/status bar.
    frame = make_window_frame_rect(UI_WIN_X, UI_TOP_BAR_Y,
                                   UI_GAME_W, g_top_bar_h,
                                   WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE);
    g_topbar_win = create_window("Resources",
                                 WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE,
                                 &frame,
                                 NULL, win_topbar_proc, 0, NULL);
    if (g_topbar_win)
        show_window(g_topbar_win, true);

    // Game window: renders the 3-D scene directly into the default framebuffer
    // using glViewport.  Sized to exactly the 3-D area so no sub-viewport
    // scissor is required inside the renderer.
    frame = make_window_frame_rect(UI_WIN_X, UI_TOP_BAR_Y + g_top_bar_h,
                                   UI_GAME_W, g_game_h,
                                   WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE);
    g_game_win = create_window("Viewport",
                               WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE,
                               &frame,
                               NULL, win_game_proc, 0, NULL);
    if (g_game_win) {
        show_window(g_game_win, true);
        set_focus(g_game_win);
    }

    // Bottom-bar window: chrome-free strip that draws command/minimap strip.
    frame = make_window_frame_rect(UI_WIN_X, UI_TOP_BAR_Y + g_top_bar_h + g_game_h,
                                   UI_GAME_W, g_bottom_bar_h,
                                   WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE);
    g_bottombar_win = create_window("Commands",
                                    WINDOW_NORESIZE | WINDOW_NOFILL | WINDOW_NOTITLE,
                                    &frame,
                                    NULL, win_bottombar_proc, 0, NULL);
    if (g_bottombar_win)
        show_window(g_bottombar_win, true);

    // Log window: small console that mirrors CON_printf output.
    frame = make_window_frame_rect(840, 20, 320, 400, 0);
    g_log_win = create_window("Log",
                              0,
                              &frame,
                              NULL, win_log_proc, 0, NULL);
    if (g_log_win)
        show_window(g_log_win, true);

    // Map selector: lists *.w3m files from the MPQ.
    g_num_maps = FS_ListMaps(g_map_paths, MAX_MAPS);
    if (g_num_maps > 0) {
        frame = make_window_frame_rect(840, 440, 320, 120, WINDOW_NORESIZE);
        g_map_win = create_window("Select Map",
                                  WINDOW_NORESIZE,
                                  &frame,
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
    // Process onclick and other UI input events exactly once per tick, before
    // any window paints, so that clicks are never dispatched more than once
    // even when both bar windows redraw in the same tick.
    SCR_ProcessOverlays();

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
