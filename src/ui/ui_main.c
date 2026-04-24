/*
 * ui_main.c — Orion-UI integration for OpenWarcraft3.
 *
 * Provides three windows:
 *   1. Game Window  — displays the 3-D game scene rendered to an off-screen
 *                     FBO via the renderer.  All keyboard / mouse input aimed
 *                     at the game is routed through this window's proc.
 *   2. Log Window   — shows CON_printf() output in a read-only text area.
 *   3. Map Selector — lists *.w3m files found in the loaded MPQ archive and
 *                     lets the player pick a map to start.
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

/* ── Forward declarations from game code ─────────────────────────────────── */

// server / map loading
extern void SV_Map(const char *map_path);

// input bridge — implemented in cl_input.c
extern void CL_MouseButtonDown(int button, float x, float y, unsigned int time);
extern void CL_MouseButtonUp(int button, float x, float y, unsigned int time);
extern void CL_MouseMove(float x, float y, float dx, float dy);
extern void CL_KeyDown(unsigned char key, unsigned int time);
extern void CL_KeyUp(unsigned char key, unsigned int time);

// renderer — returns the GLuint texture ID of the game FBO colour attachment
extern unsigned int R_GetGameTexture(void);

// console hook — register a callback to receive every CON_printf() message
typedef void (*con_log_hook_t)(const char *msg);
extern void CON_SetLogHook(con_log_hook_t hook);

// map listing — fills an array of 260-char paths, returns count
extern int FS_ListMaps(char (*out)[260], int max_maps);

// timing — axGetMilliseconds() from platform
extern unsigned long axGetMilliseconds(void);

/* ── Window handles ──────────────────────────────────────────────────────── */

static window_t *g_game_win  = NULL;
static window_t *g_log_win   = NULL;
static window_t *g_map_win   = NULL;
static window_t *g_log_edit  = NULL;   // multiedit inside log window
static window_t *g_map_combo = NULL;   // combobox inside map selector

/* ── Map list storage ────────────────────────────────────────────────────── */

#define MAX_MAPS 256
static char g_map_paths[MAX_MAPS][260];
static int  g_num_maps = 0;

/* ── Log window ──────────────────────────────────────────────────────────── */

#define LOG_BUF_SIZE (64 * 1024)
static char g_log_buf[LOG_BUF_SIZE];
static int  g_log_len = 0;

// Appended to the multiedit control each time CON_printf fires.
static void on_log_message(const char *msg) {
    int len = (int)strlen(msg);
    // If we would overflow, discard the oldest half.
    if (g_log_len + len + 2 >= LOG_BUF_SIZE) {
        int half = LOG_BUF_SIZE / 2;
        memmove(g_log_buf, g_log_buf + half, (size_t)(g_log_len - half));
        g_log_len -= half;
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
                    // Truncate to 63 chars (combobox_string_t is 64 bytes).
                    char display[64];
                    strncpy(display, g_map_paths[i], 63);
                    display[63] = '\0';
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
}

/* ── Game window ─────────────────────────────────────────────────────────── */

static result_t win_game_proc(window_t *win, uint32_t msg,
                               uint32_t wparam, void *lparam)
{
    switch (msg) {
        case evPaint: {
            unsigned int tex = R_GetGameTexture();
            if (tex) {
                rect_t client = get_client_rect(win);
                draw_rect((int)tex, &client);
            }
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
    ui_init_graphics(0, "OpenWarcraft3", 1400, 900);

    // Game window: hosts the 3-D render FBO and captures all game input.
    // WINDOW_NORESIZE keeps it at a fixed 1024x768 client size.
    g_game_win = create_window("OpenWarcraft3",
                               WINDOW_NORESIZE,
                               MAKERECT(20, 20, 1024, 768),
                               NULL, win_game_proc, 0, NULL);
    if (g_game_win)
        set_focus(g_game_win);

    // Log window: small console that mirrors CON_printf output.
    g_log_win = create_window("Log",
                              0,
                              MAKERECT(1064, 20, 320, 400),
                              NULL, win_log_proc, 0, NULL);

    // Map selector: lists *.w3m files from the MPQ.
    g_num_maps = FS_ListMaps(g_map_paths, MAX_MAPS);
    if (g_num_maps > 0) {
        g_map_win = create_window("Select Map",
                                  WINDOW_NORESIZE,
                                  MAKERECT(1064, 440, 320, 120),
                                  NULL, win_map_proc, 0, NULL);
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
    // Invalidate the game window each frame so evPaint fires and the new
    // frame from the game FBO is displayed.
    if (g_game_win)
        invalidate_window(g_game_win);

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
