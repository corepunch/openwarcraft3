#ifndef OW3_TUI_H
#define OW3_TUI_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct {
    int cols;
    int rows;
} tui_size_t;

static inline bool tui_size_from_fd(int fd, struct winsize *ws) {
    return fd >= 0 && ioctl(fd, TIOCGWINSZ, ws) == 0 && ws->ws_col > 0 && ws->ws_row > 0;
}

static inline tui_size_t tui_get_size(void) {
    struct winsize ws = { 0 };
    tui_size_t size = { 120, 32 };

    if (tui_size_from_fd(STDOUT_FILENO, &ws) ||
        tui_size_from_fd(STDERR_FILENO, &ws) ||
        tui_size_from_fd(STDIN_FILENO, &ws)) {
        size.cols = (int)ws.ws_col;
        size.rows = (int)ws.ws_row;
        return size;
    }

    const char *cols_env = getenv("COLUMNS");
    const char *rows_env = getenv("LINES");
    if (cols_env) {
        int cols = atoi(cols_env);
        if (cols > 0) {
            size.cols = cols;
        }
    }
    if (rows_env) {
        int rows = atoi(rows_env);
        if (rows > 0) {
            size.rows = rows;
        }
    }
    return size;
}

static inline void tui_enter_alt_screen(void) {
    fputs("\x1b[?1049h\x1b[?25l", stdout);
    fflush(stdout);
}

static inline void tui_leave_alt_screen(void) {
    fputs("\x1b[?25h\x1b[?1049l", stdout);
    fflush(stdout);
}

static inline void tui_clear_home(void) {
    fputs("\x1b[H\x1b[2J\x1b[H", stdout);
}

static inline void tui_set_reverse(bool on) {
    fputs(on ? "\x1b[7m" : "\x1b[0m", stdout);
}

static inline void tui_write_spaces(int count) {
    for (int i = 0; i < count; i++) {
        fputc(' ', stdout);
    }
}

static inline void tui_write_clipped(const char *text, int width) {
    int used = 0;

    if (width < 0) {
        width = 0;
    }
    if (!text) {
        text = "";
    }
    while (*text && used < width) {
        unsigned char ch = (unsigned char)*text++;
        if (ch == '\n' || ch == '\r') {
            break;
        }
        fputc(ch, stdout);
        used++;
    }
    tui_write_spaces(width - used);
}

static inline void tui_write_cell(const char *text, int width, bool selected) {
    if (selected) {
        tui_set_reverse(true);
    }
    tui_write_clipped(text, width);
    if (selected) {
        tui_set_reverse(false);
    }
}

#endif
