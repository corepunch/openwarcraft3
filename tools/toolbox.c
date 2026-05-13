#include <SDL2/SDL.h>

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define MAX_TOOLS 6
#define MAX_FIELDS 4
#define MAX_ENTRIES 512
#define MAX_HISTORY 64
#define MAX_OUTPUT 32768

typedef struct {
    int x, y, w, h;
} rect_t;

typedef struct {
    const char *name;
    const char *exe;
    const char *summary;
    const char *template_args;
} tool_t;

typedef struct {
    char name[256];
    bool is_dir;
} mpq_entry_t;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width;
    int height;
    char exe_dir[PATH_MAX];
    char history_path[PATH_MAX];
    char mpqtool_path[PATH_MAX];
    char mpq_path[PATH_MAX];
    char browser_path[PATH_MAX];
    char archive_path[PATH_MAX];
    char out_dir[PATH_MAX];
    char extra_args[1024];
    int selected_tool;
    int selected_field;
    int selected_entry;
    int browser_scroll;
    int history_scroll;
    int output_scroll;
    int dragging_entry;
    bool running;
    bool needs_browser_refresh;
    bool needs_help_refresh;
    char tool_help[8192];
    char command[4096];
    char output[MAX_OUTPUT];
    mpq_entry_t entries[MAX_ENTRIES];
    int entry_count;
    char history[MAX_HISTORY][4096];
    int history_count;
} app_t;

static const tool_t tools[MAX_TOOLS] = {
    { "mpqtool", "mpqtool", "List, inspect, pack, and dump MPQ files.", "-mpq {mpq} cat {file}" },
    { "blpconvert", "blpconvert", "Convert MPQ BLP textures to JPEG.", "-mpq {mpq} jpg {out} {path}" },
    { "fdftool", "fdftool", "Inspect FDF frame definitions.", "-mpq {mpq} -fdf {file} --info" },
    { "mdxtool", "mdxtool", "Inspect or preview MDX models.", "-mpq {mpq} -model {file} --info" },
    { "maptool", "maptool", "Open a Warcraft III map.", "-mpq {mpq} -map {file}" },
    { "mpqnc", "mpqnc", "Terminal MPQ browser.", "-mpq {mpq} -path {path}" },
};

static const char *font_rows(char c) {
    switch (toupper((unsigned char)c)) {
        case 'A': return "01110""10001""10001""11111""10001""10001""10001";
        case 'B': return "11110""10001""10001""11110""10001""10001""11110";
        case 'C': return "01111""10000""10000""10000""10000""10000""01111";
        case 'D': return "11110""10001""10001""10001""10001""10001""11110";
        case 'E': return "11111""10000""10000""11110""10000""10000""11111";
        case 'F': return "11111""10000""10000""11110""10000""10000""10000";
        case 'G': return "01111""10000""10000""10111""10001""10001""01111";
        case 'H': return "10001""10001""10001""11111""10001""10001""10001";
        case 'I': return "11111""00100""00100""00100""00100""00100""11111";
        case 'J': return "00111""00010""00010""00010""10010""10010""01100";
        case 'K': return "10001""10010""10100""11000""10100""10010""10001";
        case 'L': return "10000""10000""10000""10000""10000""10000""11111";
        case 'M': return "10001""11011""10101""10101""10001""10001""10001";
        case 'N': return "10001""11001""10101""10011""10001""10001""10001";
        case 'O': return "01110""10001""10001""10001""10001""10001""01110";
        case 'P': return "11110""10001""10001""11110""10000""10000""10000";
        case 'Q': return "01110""10001""10001""10001""10101""10010""01101";
        case 'R': return "11110""10001""10001""11110""10100""10010""10001";
        case 'S': return "01111""10000""10000""01110""00001""00001""11110";
        case 'T': return "11111""00100""00100""00100""00100""00100""00100";
        case 'U': return "10001""10001""10001""10001""10001""10001""01110";
        case 'V': return "10001""10001""10001""10001""10001""01010""00100";
        case 'W': return "10001""10001""10001""10101""10101""10101""01010";
        case 'X': return "10001""10001""01010""00100""01010""10001""10001";
        case 'Y': return "10001""10001""01010""00100""00100""00100""00100";
        case 'Z': return "11111""00001""00010""00100""01000""10000""11111";
        case '0': return "01110""10001""10011""10101""11001""10001""01110";
        case '1': return "00100""01100""00100""00100""00100""00100""01110";
        case '2': return "01110""10001""00001""00010""00100""01000""11111";
        case '3': return "11110""00001""00001""01110""00001""00001""11110";
        case '4': return "00010""00110""01010""10010""11111""00010""00010";
        case '5': return "11111""10000""10000""11110""00001""00001""11110";
        case '6': return "01110""10000""10000""11110""10001""10001""01110";
        case '7': return "11111""00001""00010""00100""01000""01000""01000";
        case '8': return "01110""10001""10001""01110""10001""10001""01110";
        case '9': return "01110""10001""10001""01111""00001""00001""01110";
        case '.': return "00000""00000""00000""00000""00000""01100""01100";
        case ',': return "00000""00000""00000""00000""01100""01100""01000";
        case ':': return "00000""01100""01100""00000""01100""01100""00000";
        case ';': return "00000""01100""01100""00000""01100""01100""01000";
        case '-': return "00000""00000""00000""11111""00000""00000""00000";
        case '_': return "00000""00000""00000""00000""00000""00000""11111";
        case '+': return "00000""00100""00100""11111""00100""00100""00000";
        case '/': return "00001""00010""00010""00100""01000""01000""10000";
        case '\\': return "10000""01000""01000""00100""00010""00010""00001";
        case '*': return "00000""10101""01110""11111""01110""10101""00000";
        case '?': return "01110""10001""00001""00010""00100""00000""00100";
        case '!': return "00100""00100""00100""00100""00100""00000""00100";
        case '"': return "01010""01010""01010""00000""00000""00000""00000";
        case '\'': return "00100""00100""01000""00000""00000""00000""00000";
        case '#': return "01010""11111""01010""01010""11111""01010""00000";
        case '$': return "00100""01111""10100""01110""00101""11110""00100";
        case '%': return "11001""11010""00010""00100""01000""01011""10011";
        case '&': return "01100""10010""10100""01000""10101""10010""01101";
        case '(': return "00010""00100""01000""01000""01000""00100""00010";
        case ')': return "01000""00100""00010""00010""00010""00100""01000";
        case '[': return "01110""01000""01000""01000""01000""01000""01110";
        case ']': return "01110""00010""00010""00010""00010""00010""01110";
        case '<': return "00010""00100""01000""10000""01000""00100""00010";
        case '>': return "01000""00100""00010""00001""00010""00100""01000";
        case '=': return "00000""11111""00000""11111""00000""00000""00000";
        case '@': return "01110""10001""10111""10101""10111""10000""01110";
        case '|': return "00100""00100""00100""00100""00100""00100""00100";
        case '~': return "00000""00000""01001""10110""00000""00000""00000";
        case ' ': return "00000""00000""00000""00000""00000""00000""00000";
        default:  return "11111""10001""00010""00100""00000""00100""00100";
    }
}

static void set_color(SDL_Renderer *r, int hex) {
    SDL_SetRenderDrawColor(r, (hex >> 16) & 255, (hex >> 8) & 255, hex & 255, 255);
}

static void fill_rect(SDL_Renderer *r, rect_t rc, int color) {
    SDL_Rect s = { rc.x, rc.y, rc.w, rc.h };
    set_color(r, color);
    SDL_RenderFillRect(r, &s);
}

static void stroke_rect(SDL_Renderer *r, rect_t rc, int color) {
    SDL_Rect s = { rc.x, rc.y, rc.w, rc.h };
    set_color(r, color);
    SDL_RenderDrawRect(r, &s);
}

static void draw_char(SDL_Renderer *r, char c, int x, int y, int scale, int color) {
    const char *rows = font_rows(c);
    set_color(r, color);
    for (int yy = 0; yy < 7; yy++) {
        for (int xx = 0; xx < 5; xx++) {
            if (rows[yy * 5 + xx] == '1') {
                SDL_Rect px = { x + xx * scale, y + yy * scale, scale, scale };
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

static void draw_text_clip(SDL_Renderer *r, const char *text, int x, int y, int scale, int color, int max_w) {
    int cx = x;
    int step = 6 * scale;
    if (!text) {
        return;
    }
    for (; *text; text++) {
        if (*text == '\n' || cx + 5 * scale > x + max_w) {
            break;
        }
        draw_char(r, *text, cx, y, scale, color);
        cx += step;
    }
}

static void draw_label(SDL_Renderer *r, rect_t rc, const char *text, bool active) {
    fill_rect(r, rc, active ? 0x2c4a5a : 0x18242a);
    stroke_rect(r, rc, active ? 0x86d9ff : 0x42525b);
    draw_text_clip(r, text, rc.x + 8, rc.y + 8, 2, active ? 0xffffff : 0xc9d6d3, rc.w - 16);
}

static bool point_in(rect_t rc, int x, int y) {
    return x >= rc.x && y >= rc.y && x < rc.x + rc.w && y < rc.y + rc.h;
}

static bool equals_ci(const char *a, const char *b) {
#ifdef _WIN32
    return _stricmp(a, b) == 0;
#else
    return strcasecmp(a, b) == 0;
#endif
}

static void dirname_of(const char *path, char *out, size_t out_size) {
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *back = strrchr(path, '\\');
    if (!slash || (back && back > slash)) {
        slash = back;
    }
#endif
    if (!slash) {
        snprintf(out, out_size, ".");
        return;
    }
    size_t n = (size_t)(slash - path);
    if (n >= out_size) {
        n = out_size - 1;
    }
    memcpy(out, path, n);
    out[n] = 0;
}

static void path_join(char *out, size_t out_size, const char *dir, const char *name) {
    size_t len = strlen(dir);
    const char *sep = (len > 0 && dir[len - 1] == '/') ? "" : "/";
    snprintf(out, out_size, "%s%s%s", dir, sep, name);
}

static void shell_quote_append(char *out, size_t out_size, const char *s) {
    strncat(out, "'", out_size - strlen(out) - 1);
    for (; s && *s; s++) {
        if (*s == '\'') {
            strncat(out, "'\\''", out_size - strlen(out) - 1);
        } else {
            char tmp[2] = { *s, 0 };
            strncat(out, tmp, out_size - strlen(out) - 1);
        }
    }
    strncat(out, "'", out_size - strlen(out) - 1);
}

static char *run_capture(const char *cmd) {
    FILE *fp;
    char *buf = malloc(MAX_OUTPUT);
    size_t used = 0;
    char shell_cmd[8192];
    if (!buf) {
        return NULL;
    }
    buf[0] = 0;
    snprintf(shell_cmd, sizeof(shell_cmd), "%s 2>&1", cmd);
    fp = popen(shell_cmd, "r");
    if (!fp) {
        snprintf(buf, MAX_OUTPUT, "popen failed: %s\n", strerror(errno));
        return buf;
    }
    while (used + 1 < MAX_OUTPUT) {
        size_t got = fread(buf + used, 1, MAX_OUTPUT - used - 1, fp);
        used += got;
        if (got == 0) {
            break;
        }
    }
    buf[used] = 0;
    pclose(fp);
    return buf;
}

static void add_history(app_t *app, const char *cmd) {
    if (!cmd || !*cmd) {
        return;
    }
    if (app->history_count == MAX_HISTORY) {
        memmove(app->history, app->history + 1, sizeof(app->history[0]) * (MAX_HISTORY - 1));
        app->history_count--;
    }
    snprintf(app->history[app->history_count++], sizeof(app->history[0]), "%s", cmd);
}

static void save_history(app_t *app) {
    FILE *f = fopen(app->history_path, "w");
    if (!f) {
        return;
    }
    for (int i = 0; i < app->history_count; i++) {
        fprintf(f, "%s\n", app->history[i]);
    }
    fclose(f);
}

static void load_history(app_t *app) {
    FILE *f = fopen(app->history_path, "r");
    char line[4096];
    if (!f) {
        return;
    }
    while (fgets(line, sizeof(line), f) && app->history_count < MAX_HISTORY) {
        line[strcspn(line, "\r\n")] = 0;
        if (*line) {
            snprintf(app->history[app->history_count++], sizeof(app->history[0]), "%s", line);
        }
    }
    fclose(f);
}

static void build_command(app_t *app) {
    const tool_t *tool = &tools[app->selected_tool];
    char exe[PATH_MAX];
    char *dst = app->command;
    size_t left = sizeof(app->command);
    path_join(exe, sizeof(exe), app->exe_dir, tool->exe);
    app->command[0] = 0;
    shell_quote_append(app->command, sizeof(app->command), exe);
    strncat(app->command, " ", sizeof(app->command) - strlen(app->command) - 1);

    const char *p = tool->template_args;
    while (*p && strlen(app->command) + 8 < sizeof(app->command)) {
        if (!strncmp(p, "{mpq}", 5)) {
            shell_quote_append(app->command, sizeof(app->command), app->mpq_path);
            p += 5;
        } else if (!strncmp(p, "{file}", 6)) {
            shell_quote_append(app->command, sizeof(app->command), app->archive_path);
            p += 6;
        } else if (!strncmp(p, "{path}", 6)) {
            shell_quote_append(app->command, sizeof(app->command), app->archive_path);
            p += 6;
        } else if (!strncmp(p, "{out}", 5)) {
            shell_quote_append(app->command, sizeof(app->command), app->out_dir);
            p += 5;
        } else {
            char tmp[2] = { *p++, 0 };
            strncat(app->command, tmp, sizeof(app->command) - strlen(app->command) - 1);
        }
    }
    if (app->extra_args[0]) {
        strncat(app->command, " ", sizeof(app->command) - strlen(app->command) - 1);
        strncat(app->command, app->extra_args, sizeof(app->command) - strlen(app->command) - 1);
    }
    (void)dst;
    (void)left;
}

static void refresh_help(app_t *app) {
    char exe[PATH_MAX], cmd[PATH_MAX + 16];
    char *out;
    path_join(exe, sizeof(exe), app->exe_dir, tools[app->selected_tool].exe);
    cmd[0] = 0;
    shell_quote_append(cmd, sizeof(cmd), exe);
    strncat(cmd, " -?", sizeof(cmd) - strlen(cmd) - 1);
    out = run_capture(cmd);
    if (out) {
        snprintf(app->tool_help, sizeof(app->tool_help), "%s", out);
        free(out);
    }
    app->needs_help_refresh = false;
}

static int entry_cmp(const void *a, const void *b) {
    const mpq_entry_t *ea = (const mpq_entry_t *)a;
    const mpq_entry_t *eb = (const mpq_entry_t *)b;
    if (ea->is_dir != eb->is_dir) {
        return ea->is_dir ? -1 : 1;
    }
#ifdef _WIN32
    return _stricmp(ea->name, eb->name);
#else
    return strcasecmp(ea->name, eb->name);
#endif
}

static void parent_path(char *path) {
    char *slash;
    if (!path[0]) {
        return;
    }
    slash = strrchr(path, '\\');
    if (!slash) {
        slash = strrchr(path, '/');
    }
    if (slash) {
        *slash = 0;
    } else {
        path[0] = 0;
    }
}

static void append_archive_child(app_t *app, const char *name, bool is_dir) {
    size_t len = strlen(is_dir ? app->browser_path : app->archive_path);
    if (is_dir) {
        if (len > 0) {
            strncat(app->browser_path, "\\", sizeof(app->browser_path) - strlen(app->browser_path) - 1);
        }
        strncat(app->browser_path, name, sizeof(app->browser_path) - strlen(app->browser_path) - 1);
    } else {
        if (len > 0) {
            strncat(app->archive_path, "\\", sizeof(app->archive_path) - strlen(app->archive_path) - 1);
        }
        strncat(app->archive_path, name, sizeof(app->archive_path) - strlen(app->archive_path) - 1);
    }
}

static void refresh_browser(app_t *app) {
    char cmd[8192];
    char *out;
    app->entry_count = 0;
    cmd[0] = 0;
    shell_quote_append(cmd, sizeof(cmd), app->mpqtool_path);
    strncat(cmd, " -mpq ", sizeof(cmd) - strlen(cmd) - 1);
    shell_quote_append(cmd, sizeof(cmd), app->mpq_path);
    strncat(cmd, " ls ", sizeof(cmd) - strlen(cmd) - 1);
    shell_quote_append(cmd, sizeof(cmd), app->browser_path);
    out = run_capture(cmd);
    if (!out) {
        return;
    }

    if (app->browser_path[0] && app->entry_count < MAX_ENTRIES) {
        snprintf(app->entries[app->entry_count].name, sizeof(app->entries[0].name), "..");
        app->entries[app->entry_count].is_dir = true;
        app->entry_count++;
    }

    char *save = NULL;
    for (char *line = strtok_r(out, "\r\n", &save); line && app->entry_count < MAX_ENTRIES; line = strtok_r(NULL, "\r\n", &save)) {
        size_t n = strlen(line);
        if (n == 0 || strstr(line, "Cannot open archive")) {
            continue;
        }
        bool is_dir = line[n - 1] == '/';
        if (is_dir) {
            line[n - 1] = 0;
        }
        snprintf(app->entries[app->entry_count].name, sizeof(app->entries[0].name), "%s", line);
        app->entries[app->entry_count].is_dir = is_dir;
        app->entry_count++;
    }
    free(out);

    if (app->entry_count > (app->browser_path[0] ? 1 : 0)) {
        qsort(app->entries + (app->browser_path[0] ? 1 : 0),
              (size_t)(app->entry_count - (app->browser_path[0] ? 1 : 0)),
              sizeof(app->entries[0]), entry_cmp);
    }
    app->selected_entry = app->entry_count ? 0 : -1;
    app->browser_scroll = 0;
    app->needs_browser_refresh = false;
}

static void run_current_command(app_t *app) {
    build_command(app);
    add_history(app, app->command);
    save_history(app);
    snprintf(app->output, sizeof(app->output), "Running:\n%s\n\n", app->command);
    SDL_RenderPresent(app->renderer);
    char *out = run_capture(app->command);
    if (out) {
        strncat(app->output, out, sizeof(app->output) - strlen(app->output) - 1);
        free(out);
    }
    app->output_scroll = 0;
}

static void put_selected_entry_in_field(app_t *app) {
    if (app->dragging_entry < 0 || app->dragging_entry >= app->entry_count) {
        return;
    }
    mpq_entry_t *e = &app->entries[app->dragging_entry];
    if (!strcmp(e->name, "..")) {
        return;
    }
    char value[PATH_MAX];
    snprintf(value, sizeof(value), "%s", app->browser_path);
    if (value[0]) {
        strncat(value, "\\", sizeof(value) - strlen(value) - 1);
    }
    strncat(value, e->name, sizeof(value) - strlen(value) - 1);
    if (app->selected_field == 1) {
        snprintf(app->archive_path, sizeof(app->archive_path), "%s", value);
    } else if (app->selected_field == 3) {
        if (app->extra_args[0]) {
            strncat(app->extra_args, " ", sizeof(app->extra_args) - strlen(app->extra_args) - 1);
        }
        shell_quote_append(app->extra_args, sizeof(app->extra_args), value);
    }
}

static void draw_panel_title(SDL_Renderer *r, rect_t rc, const char *title) {
    fill_rect(r, (rect_t){ rc.x, rc.y, rc.w, 26 }, 0x22343c);
    stroke_rect(r, rc, 0x4f6972);
    draw_text_clip(r, title, rc.x + 8, rc.y + 7, 2, 0xf6e8b1, rc.w - 16);
}

static void draw_lines(SDL_Renderer *r, const char *text, rect_t rc, int scroll, int color) {
    int y = rc.y + 8;
    int line_h = 18;
    int skip = scroll;
    const char *p = text ? text : "";
    while (*p && y + line_h <= rc.y + rc.h) {
        char line[512];
        int n = 0;
        while (*p && *p != '\n' && n < (int)sizeof(line) - 1) {
            line[n++] = *p++;
        }
        while (*p && *p != '\n') {
            p++;
        }
        if (*p == '\n') {
            p++;
        }
        line[n] = 0;
        if (skip > 0) {
            skip--;
            continue;
        }
        draw_text_clip(r, line, rc.x + 8, y, 2, color, rc.w - 16);
        y += line_h;
    }
}

static void render(app_t *app) {
    SDL_Renderer *r = app->renderer;
    rect_t header = { 0, 0, app->width, 42 };
    rect_t browser = { 10, 52, app->width * 34 / 100 - 15, app->height - 240 };
    rect_t center = { browser.x + browser.w + 10, 52, app->width * 32 / 100, app->height - 240 };
    rect_t right = { center.x + center.w + 10, 52, app->width - center.x - center.w - 20, app->height - 240 };
    rect_t bottom = { 10, app->height - 178, app->width - 20, 168 };

    set_color(r, 0x101619);
    SDL_RenderClear(r);
    fill_rect(r, header, 0x1c2a30);
    draw_text_clip(r, "OPENWARCRAFT3 TOOLBOX", 14, 12, 3, 0xffffff, 420);
    draw_text_clip(r, "F5 RUN  F6 HELP  ENTER OPEN  BACKSPACE UP  D DRAG/COPY  ESC QUIT", 460, 15, 2, 0xa9bbb8, app->width - 470);

    draw_panel_title(r, browser, "MPQ BROWSER");
    draw_text_clip(r, app->browser_path[0] ? app->browser_path : "\\", browser.x + 8, browser.y + 34, 2, 0x8bd7ff, browser.w - 16);
    int row_y = browser.y + 58;
    int rows = (browser.h - 64) / 20;
    for (int i = 0; i < rows && i + app->browser_scroll < app->entry_count; i++) {
        int idx = i + app->browser_scroll;
        rect_t row = { browser.x + 6, row_y + i * 20, browser.w - 12, 18 };
        if (idx == app->selected_entry) {
            fill_rect(r, row, 0x315263);
        }
        char label[300];
        snprintf(label, sizeof(label), "%s%s", app->entries[idx].is_dir ? "[+] " : "    ", app->entries[idx].name);
        draw_text_clip(r, label, row.x + 4, row.y + 3, 2, app->entries[idx].is_dir ? 0xf8d77e : 0xdde9e4, row.w - 8);
    }

    draw_panel_title(r, center, "TOOLS AND FIELDS");
    int y = center.y + 36;
    for (int i = 0; i < MAX_TOOLS; i++) {
        rect_t row = { center.x + 8, y, center.w - 16, 30 };
        draw_label(r, row, tools[i].name, i == app->selected_tool);
        y += 34;
    }
    y += 6;
    const char *field_names[MAX_FIELDS] = { "MPQ", "ARCHIVE PATH / FILE", "OUTPUT DIR", "EXTRA ARGS" };
    const char *field_values[MAX_FIELDS] = { app->mpq_path, app->archive_path, app->out_dir, app->extra_args };
    for (int i = 0; i < MAX_FIELDS; i++) {
        draw_text_clip(r, field_names[i], center.x + 8, y, 2, 0x91a6a6, center.w - 16);
        y += 18;
        rect_t fr = { center.x + 8, y, center.w - 16, 28 };
        draw_label(r, fr, field_values[i], i == app->selected_field);
        y += 34;
    }
    build_command(app);
    draw_text_clip(r, "COMMAND", center.x + 8, y, 2, 0x91a6a6, center.w - 16);
    y += 18;
    fill_rect(r, (rect_t){ center.x + 8, y, center.w - 16, 58 }, 0x0c1113);
    stroke_rect(r, (rect_t){ center.x + 8, y, center.w - 16, 58 }, 0x4f6972);
    draw_lines(r, app->command, (rect_t){ center.x + 8, y, center.w - 16, 58 }, 0, 0xdde9e4);

    draw_panel_title(r, right, "HELP / ACCEPTED SETTINGS");
    draw_text_clip(r, tools[app->selected_tool].summary, right.x + 8, right.y + 34, 2, 0xdde9e4, right.w - 16);
    draw_lines(r, app->tool_help, (rect_t){ right.x + 4, right.y + 58, right.w - 8, right.h - 64 }, app->output_scroll, 0xa9bbb8);

    draw_panel_title(r, bottom, "OUTPUT AND RECENT COMMANDS");
    rect_t out = { bottom.x + 4, bottom.y + 32, bottom.w * 62 / 100, bottom.h - 38 };
    rect_t hist = { out.x + out.w + 8, out.y, bottom.w - out.w - 16, out.h };
    fill_rect(r, out, 0x0c1113);
    stroke_rect(r, out, 0x4f6972);
    draw_lines(r, app->output[0] ? app->output : "No command run yet.", out, app->output_scroll, 0xdde9e4);
    fill_rect(r, hist, 0x0c1113);
    stroke_rect(r, hist, 0x4f6972);
    int hy = hist.y + 8;
    int max_h = (hist.h - 16) / 18;
    int start = app->history_count - max_h - app->history_scroll;
    if (start < 0) start = 0;
    for (int i = start; i < app->history_count && hy + 18 <= hist.y + hist.h; i++) {
        draw_text_clip(r, app->history[i], hist.x + 8, hy, 2, 0x9fd3a8, hist.w - 16);
        hy += 18;
    }

    SDL_RenderPresent(r);
}

static int field_at(app_t *app, int mx, int my) {
    rect_t browser = { 10, 52, app->width * 34 / 100 - 15, app->height - 240 };
    rect_t center = { browser.x + browser.w + 10, 52, app->width * 32 / 100, app->height - 240 };
    int y = center.y + 36 + MAX_TOOLS * 34 + 6;
    for (int i = 0; i < MAX_FIELDS; i++) {
        y += 18;
        rect_t fr = { center.x + 8, y, center.w - 16, 28 };
        if (point_in(fr, mx, my)) {
            return i;
        }
        y += 34;
    }
    return -1;
}

static int tool_at(app_t *app, int mx, int my) {
    rect_t browser = { 10, 52, app->width * 34 / 100 - 15, app->height - 240 };
    rect_t center = { browser.x + browser.w + 10, 52, app->width * 32 / 100, app->height - 240 };
    int y = center.y + 36;
    for (int i = 0; i < MAX_TOOLS; i++) {
        rect_t row = { center.x + 8, y, center.w - 16, 30 };
        if (point_in(row, mx, my)) {
            return i;
        }
        y += 34;
    }
    return -1;
}

static int entry_at(app_t *app, int mx, int my) {
    rect_t browser = { 10, 52, app->width * 34 / 100 - 15, app->height - 240 };
    int row_y = browser.y + 58;
    int row = (my - row_y) / 20;
    if (mx < browser.x || mx >= browser.x + browser.w || row < 0) {
        return -1;
    }
    int idx = app->browser_scroll + row;
    return idx >= 0 && idx < app->entry_count ? idx : -1;
}

static void open_selected_entry(app_t *app) {
    if (app->selected_entry < 0 || app->selected_entry >= app->entry_count) {
        return;
    }
    mpq_entry_t *e = &app->entries[app->selected_entry];
    if (!strcmp(e->name, "..")) {
        parent_path(app->browser_path);
        app->needs_browser_refresh = true;
        return;
    }
    if (e->is_dir) {
        append_archive_child(app, e->name, true);
        app->needs_browser_refresh = true;
    } else {
        char base[PATH_MAX];
        snprintf(base, sizeof(base), "%s", app->browser_path);
        if (base[0]) {
            strncat(base, "\\", sizeof(base) - strlen(base) - 1);
        }
        strncat(base, e->name, sizeof(base) - strlen(base) - 1);
        snprintf(app->archive_path, sizeof(app->archive_path), "%s", base);
    }
}

static void edit_field(app_t *app, const char *text) {
    char *dst = NULL;
    size_t cap = 0;
    if (app->selected_field == 0) { dst = app->mpq_path; cap = sizeof(app->mpq_path); }
    if (app->selected_field == 1) { dst = app->archive_path; cap = sizeof(app->archive_path); }
    if (app->selected_field == 2) { dst = app->out_dir; cap = sizeof(app->out_dir); }
    if (app->selected_field == 3) { dst = app->extra_args; cap = sizeof(app->extra_args); }
    if (dst && text) {
        strncat(dst, text, cap - strlen(dst) - 1);
        if (app->selected_field == 0) {
            app->needs_browser_refresh = true;
        }
    }
}

static void backspace_field(app_t *app) {
    char *dst = NULL;
    if (app->selected_field == 0) dst = app->mpq_path;
    if (app->selected_field == 1) dst = app->archive_path;
    if (app->selected_field == 2) dst = app->out_dir;
    if (app->selected_field == 3) dst = app->extra_args;
    if (dst && dst[0]) {
        dst[strlen(dst) - 1] = 0;
        if (app->selected_field == 0) {
            app->needs_browser_refresh = true;
        }
    }
}

static void init_app(app_t *app, int argc, char **argv) {
    memset(app, 0, sizeof(*app));
    app->width = 1280;
    app->height = 820;
    app->selected_entry = -1;
    app->dragging_entry = -1;
    dirname_of(argv[0], app->exe_dir, sizeof(app->exe_dir));
    path_join(app->history_path, sizeof(app->history_path), app->exe_dir, "toolbox_recent.txt");
    path_join(app->mpqtool_path, sizeof(app->mpqtool_path), app->exe_dir, "mpqtool");
    snprintf(app->mpq_path, sizeof(app->mpq_path), "data/Warcraft III/War3.mpq");
    snprintf(app->out_dir, sizeof(app->out_dir), "/tmp/openwarcraft3-toolbox");

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-mpq") && i + 1 < argc) {
            snprintf(app->mpq_path, sizeof(app->mpq_path), "%s", argv[++i]);
        } else if (!strcmp(argv[i], "-mpqtool") && i + 1 < argc) {
            snprintf(app->mpqtool_path, sizeof(app->mpqtool_path), "%s", argv[++i]);
        }
    }

    load_history(app);
    app->needs_browser_refresh = true;
    app->needs_help_refresh = true;
}

static void update_render_scale(app_t *app) {
    int window_w = 0;
    int window_h = 0;

    SDL_GetWindowSize(app->window, &window_w, &window_h);
    if (window_w > 0 && window_h > 0) {
        app->width = window_w;
        app->height = window_h;
        SDL_RenderSetLogicalSize(app->renderer, app->width, app->height);
    }
}

int main(int argc, char **argv) {
    app_t app;
    bool quit = false;

    init_app(&app, argc, argv);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    app.window = SDL_CreateWindow("OpenWarcraft3 Toolbox",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  app.width, app.height,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!app.window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer) {
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
        if (!app.renderer) {
            fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(app.window);
            SDL_Quit();
            return 1;
        }
    }
    update_render_scale(&app);

    SDL_StartTextInput();
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    while (!quit) {
        if (app.needs_help_refresh) {
            refresh_help(&app);
        }
        if (app.needs_browser_refresh) {
            refresh_browser(&app);
        }
        render(&app);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                quit = true;
            } else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                app.width = ev.window.data1;
                app.height = ev.window.data2;
                update_render_scale(&app);
            } else if (ev.type == SDL_TEXTINPUT) {
                edit_field(&app, ev.text.text);
            } else if (ev.type == SDL_DROPFILE) {
                const char *d = ev.drop.file;
                size_t n = strlen(d);
                if (n >= 4 && equals_ci(d + n - 4, ".mpq")) {
                    snprintf(app.mpq_path, sizeof(app.mpq_path), "%s", d);
                    app.archive_path[0] = 0;
                    app.browser_path[0] = 0;
                    app.needs_browser_refresh = true;
                } else {
                    snprintf(app.out_dir, sizeof(app.out_dir), "%s", d);
                }
                SDL_free(ev.drop.file);
            } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int t = tool_at(&app, ev.button.x, ev.button.y);
                int f = field_at(&app, ev.button.x, ev.button.y);
                int e = entry_at(&app, ev.button.x, ev.button.y);
                if (t >= 0) {
                    app.selected_tool = t;
                    app.needs_help_refresh = true;
                } else if (f >= 0) {
                    app.selected_field = f;
                } else if (e >= 0) {
                    app.selected_entry = e;
                    app.dragging_entry = e;
                    if (ev.button.clicks >= 2) {
                        open_selected_entry(&app);
                        app.dragging_entry = -1;
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                int f = field_at(&app, ev.button.x, ev.button.y);
                int t = tool_at(&app, ev.button.x, ev.button.y);
                if (f >= 0) {
                    app.selected_field = f;
                    put_selected_entry_in_field(&app);
                } else if (t >= 0) {
                    app.selected_tool = t;
                    app.selected_field = 1;
                    put_selected_entry_in_field(&app);
                    app.needs_help_refresh = true;
                }
                app.dragging_entry = -1;
            } else if (ev.type == SDL_MOUSEWHEEL) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                rect_t browser = { 10, 52, app.width * 34 / 100 - 15, app.height - 240 };
                if (point_in(browser, mx, my)) {
                    app.browser_scroll -= ev.wheel.y * 3;
                    if (app.browser_scroll < 0) app.browser_scroll = 0;
                    if (app.browser_scroll > app.entry_count - 1) app.browser_scroll = app.entry_count - 1;
                } else {
                    app.output_scroll -= ev.wheel.y * 3;
                    if (app.output_scroll < 0) app.output_scroll = 0;
                }
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) {
                    quit = true;
                } else if (k == SDLK_F5) {
                    run_current_command(&app);
                } else if (k == SDLK_F6) {
                    app.needs_help_refresh = true;
                } else if (k == SDLK_RETURN) {
                    open_selected_entry(&app);
                } else if (k == SDLK_BACKSPACE) {
                    backspace_field(&app);
                } else if (k == SDLK_TAB) {
                    app.selected_field = (app.selected_field + 1) % MAX_FIELDS;
                } else if (k == SDLK_UP) {
                    if (app.selected_entry > 0) app.selected_entry--;
                    if (app.selected_entry < app.browser_scroll) app.browser_scroll = app.selected_entry;
                } else if (k == SDLK_DOWN) {
                    if (app.selected_entry + 1 < app.entry_count) app.selected_entry++;
                    int rows = (app.height - 240 - 64) / 20;
                    if (app.selected_entry >= app.browser_scroll + rows) app.browser_scroll++;
                } else if (k == SDLK_d) {
                    app.dragging_entry = app.selected_entry;
                    put_selected_entry_in_field(&app);
                    app.dragging_entry = -1;
                } else if (k == SDLK_r) {
                    app.needs_browser_refresh = true;
                }
            }
        }
    }

    save_history(&app);
    SDL_StopTextInput();
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 0;
}
