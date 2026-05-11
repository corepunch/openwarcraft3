#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tui.h"

typedef struct {
    char *name;
    bool is_dir;
} entry_t;

typedef struct {
    entry_t *items;
    size_t count;
    size_t cap;
} entry_list_t;

typedef struct {
    char *items;
    size_t count;
    size_t cap;
} char_buffer_t;

typedef struct {
    char *mpq_path;
    char *start_path;
    char *mpqtool_path;
    char *mdxtool_path;
    char *fdftool_path;
    char *current_path;
    entry_list_t entries;
    size_t selection;
    size_t scroll;
    char *cached_info_path;
    char *cached_info_text;
    int   cached_panel_width;
    bool  dump_mode;
} app_t;

typedef struct {
    struct termios saved;
    bool active;
} terminal_state_t;

typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
    KEY_REFRESH,
    KEY_QUIT,
} browser_key_t;

static terminal_state_t g_term = { 0 };
static volatile sig_atomic_t g_resize_pending = 0;

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void *xmalloc(size_t size) {
    void *ptr = calloc(1, size ? size : 1);
    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return ptr;
}

static void *xrealloc(void *ptr, size_t size) {
    void *next = realloc(ptr, size ? size : 1);
    if (!next) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return next;
}

static char *xstrdup(const char *s) {
    size_t len = s ? strlen(s) : 0;
    char *copy = xmalloc(len + 1);
    if (len) {
        memcpy(copy, s, len);
    }
    copy[len] = '\0';
    return copy;
}

static void buffer_append(char_buffer_t *buf, const char *data, size_t len) {
    if (len == 0) {
        return;
    }
    if (buf->count + len + 1 > buf->cap) {
        size_t next = buf->cap ? buf->cap * 2 : 4096;
        while (next < buf->count + len + 1) {
            next *= 2;
        }
        buf->items = xrealloc(buf->items, next);
        buf->cap = next;
    }
    memcpy(buf->items + buf->count, data, len);
    buf->count += len;
    buf->items[buf->count] = '\0';
}

static char *run_capture(char *const argv[], int *exit_code) {
    int pipefd[2];
    pid_t pid;
    char_buffer_t out = { 0 };

    if (pipe(pipefd) != 0) {
        die("pipe");
    }

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0 || dup2(pipefd[1], STDERR_FILENO) < 0) {
            perror("dup2");
            _exit(127);
        }
        close(pipefd[1]);
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    close(pipefd[1]);
    for (;;) {
        char buf[4096];
        ssize_t got = read(pipefd[0], buf, sizeof(buf));
        if (got < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(pipefd[0]);
            break;
        }
        if (got == 0) {
            close(pipefd[0]);
            break;
        }
        buffer_append(&out, buf, (size_t)got);
    }

    if (waitpid(pid, exit_code ? exit_code : &(int){0}, 0) < 0) {
        die("waitpid");
    }
    return out.items ? out.items : xstrdup("");
}

static void free_entries(entry_list_t *list) {
    if (!list) {
        return;
    }
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].name);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->cap = 0;
}

static void entry_list_push(entry_list_t *list, const char *name, bool is_dir) {
    if (list->count == list->cap) {
        size_t next = list->cap ? list->cap * 2 : 32;
        list->items = xrealloc(list->items, next * sizeof(*list->items));
        list->cap = next;
    }
    list->items[list->count].name = xstrdup(name);
    list->items[list->count].is_dir = is_dir;
    list->count++;
}

static int entry_cmp(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *)a;
    const entry_t *eb = (const entry_t *)b;
    if (ea->is_dir != eb->is_dir) {
        return ea->is_dir ? -1 : 1;
    }
    return strcasecmp(ea->name, eb->name);
}

static void normalize_mpq_path(char *path) {
    for (; *path; path++) {
        if (*path == '\\') {
            *path = '/';
        }
    }
}

static char *join_path(const char *base, const char *name) {
    size_t base_len = base ? strlen(base) : 0;
    size_t name_len = strlen(name);
    bool need_slash = base_len > 0;
    char *out = xmalloc(base_len + name_len + (need_slash ? 2 : 1));
    if (need_slash) {
        sprintf(out, "%s/%s", base, name);
    } else {
        sprintf(out, "%s", name);
    }
    return out;
}

static char *parent_path(const char *path) {
    char *copy = xstrdup(path ? path : "");
    char *slash;
    normalize_mpq_path(copy);
    slash = strrchr(copy, '/');
    if (slash) {
        *slash = '\0';
    } else {
        copy[0] = '\0';
    }
    return copy;
}

static const char *path_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *back = strrchr(path, '\\');
    const char *base = slash;
    if (!base || (back && back > base)) {
        base = back;
    }
    return base ? base + 1 : path;
}

static const char *path_ext(const char *path) {
    const char *base = path_basename(path);
    const char *dot = strrchr(base, '.');
    return dot ? dot + 1 : "";
}

static bool is_text_ext(const char *ext) {
    return !strcasecmp(ext, "txt") ||
           !strcasecmp(ext, "slk") ||
           !strcasecmp(ext, "fdf") ||
           !strcasecmp(ext, "ini") ||
           !strcasecmp(ext, "cfg") ||
           !strcasecmp(ext, "j") ||
           !strcasecmp(ext, "lua");
}

static bool is_image_ext(const char *ext) {
    return !strcasecmp(ext, "blp") ||
           !strcasecmp(ext, "png") ||
           !strcasecmp(ext, "jpg") ||
           !strcasecmp(ext, "jpeg") ||
           !strcasecmp(ext, "tga");
}

static void terminal_enter(void) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "mpqnc requires an interactive TTY\n");
        exit(1);
    }
    if (tcgetattr(STDIN_FILENO, &g_term.saved) != 0) {
        die("tcgetattr");
    }
    struct termios raw = g_term.saved;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= (tcflag_t) ~(IXON | ICRNL);
    raw.c_oflag &= (tcflag_t) ~(OPOST);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        die("tcsetattr");
    }
    g_term.active = true;
    fputs("\x1b[?1049h\x1b[?25l", stdout);
    fflush(stdout);
}

static void terminal_leave(void) {
    if (!g_term.active) {
        return;
    }
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_term.saved);
    fputs("\x1b[?25h\x1b[?1049l", stdout);
    fflush(stdout);
    g_term.active = false;
}

static void on_signal(int sig) {
    if (sig == SIGWINCH) {
        g_resize_pending = 1;
        return;
    }
    terminal_leave();
    _exit(130);
}

static void append_text_line(char_buffer_t *buf, const char *text) {
    buffer_append(buf, text, strlen(text));
    buffer_append(buf, "\n", 1);
}

static void wrapped_append_line(char_buffer_t *out, const char *line, size_t width) {
    const char *cursor = line;
    size_t len = strlen(line);
    if (width == 0) {
        return;
    }
    while (len > width) {
        size_t cut = width;
        while (cut > 0 && !isspace((unsigned char)cursor[cut])) {
            cut--;
        }
        if (cut == 0) {
            cut = width;
        }
        buffer_append(out, cursor, cut);
        buffer_append(out, "\n", 1);
        cursor += cut;
        while (*cursor == ' ' || *cursor == '\t') {
            cursor++;
        }
        len = strlen(cursor);
    }
    buffer_append(out, cursor, len);
    buffer_append(out, "\n", 1);
}

static char *wrap_text(const char *text, size_t width) {
    char_buffer_t out = { 0 };
    const char *cursor = text ? text : "";
    while (*cursor) {
        const char *line_end = strchr(cursor, '\n');
        size_t len = line_end ? (size_t)(line_end - cursor) : strlen(cursor);
        char *line = xmalloc(len + 1);
        memcpy(line, cursor, len);
        line[len] = '\0';
        wrapped_append_line(&out, line, width);
        free(line);
        cursor = line_end ? line_end + 1 : cursor + len;
    }
    if (out.count == 0) {
        append_text_line(&out, "");
    }
    return out.items ? out.items : xstrdup("");
}

static char *build_listing(app_t *app) {
    char *argv[8];
    int exit_code = 0;
    char *out;
    size_t i = 0;

    argv[i++] = app->mpqtool_path;
    argv[i++] = "-mpq";
    argv[i++] = app->mpq_path;
    argv[i++] = "ls";
    if (app->current_path && app->current_path[0]) {
        argv[i++] = app->current_path;
    }
    argv[i] = NULL;
    out = run_capture(argv, &exit_code);
    if (exit_code != 0 && (!out || !*out)) {
        free(out);
        out = xstrdup("(mpqtool ls failed)");
    }
    return out;
}

static char *build_mdx_info(app_t *app, const char *path) {
    char *argv[8];
    int exit_code = 0;
    size_t i = 0;
    argv[i++] = app->mdxtool_path;
    argv[i++] = "-mpq";
    argv[i++] = app->mpq_path;
    argv[i++] = "-model";
    argv[i++] = (char *)path;
    argv[i++] = "--info";
    argv[i] = NULL;
    return run_capture(argv, &exit_code);
}

static char *build_fdf_info(app_t *app, const char *path) {
    char *argv[8];
    int exit_code = 0;
    size_t i = 0;
    argv[i++] = app->fdftool_path;
    argv[i++] = "-mpq";
    argv[i++] = app->mpq_path;
    argv[i++] = "-fdf";
    argv[i++] = (char *)path;
    argv[i++] = "--info";
    argv[i] = NULL;
    return run_capture(argv, &exit_code);
}

static char *build_img_info(app_t *app, const char *path) {
    char *argv[8];
    int exit_code = 0;
    size_t i = 0;
    argv[i++] = app->mpqtool_path;
    argv[i++] = "-mpq";
    argv[i++] = app->mpq_path;
    argv[i++] = "imginfo";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_capture(argv, &exit_code);
}

static char *build_text_preview(app_t *app, const char *path) {
    char *argv[8];
    int exit_code = 0;
    size_t i = 0;
    argv[i++] = app->mpqtool_path;
    argv[i++] = "-mpq";
    argv[i++] = app->mpq_path;
    argv[i++] = "cat";
    argv[i++] = (char *)path;
    argv[i] = NULL;
    return run_capture(argv, &exit_code);
}

static void load_entries(app_t *app) {
    char *listing = build_listing(app);
    char *save = listing;
    entry_list_t list = { 0 };

    if (app->current_path && app->current_path[0]) {
        entry_list_push(&list, "..", true);
    }

    while (listing && *listing) {
        char *line = listing;
        char *end = strchr(listing, '\n');
        if (end) {
            *end = '\0';
            listing = end + 1;
        } else {
            listing += strlen(listing);
        }
        while (*line == '\r' || *line == '\n' || *line == ' ' || *line == '\t') {
            line++;
        }
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
            line[--len] = '\0';
        }
        if (len == 0) {
            continue;
        }
        bool is_dir = len > 0 && line[len - 1] == '/';
        if (is_dir) {
            line[--len] = '\0';
        }
        entry_list_push(&list, line, is_dir);
    }

    qsort(list.items, list.count, sizeof(list.items[0]), entry_cmp);
    free_entries(&app->entries);
    app->entries = list;
    free(save);
    if (app->selection >= app->entries.count) {
        app->selection = app->entries.count ? app->entries.count - 1 : 0;
    }
    if (app->entries.count > 0 && app->selection == 0 && strcmp(app->entries.items[0].name, "..") == 0) {
        app->selection = app->entries.count > 1 ? 1 : 0;
    }
}

static void ensure_scroll(app_t *app, size_t visible_rows) {
    if (app->selection < app->scroll) {
        app->scroll = app->selection;
    }
    if (app->selection >= app->scroll + visible_rows) {
        app->scroll = app->selection - visible_rows + 1;
    }
}

static void reset_selection(app_t *app) {
    if (app->entries.count > 1 && strcmp(app->entries.items[0].name, "..") == 0) {
        app->selection = 1;
    } else {
        app->selection = 0;
    }
    app->scroll = 0;
}

static void clear_info_cache(app_t *app) {
    free(app->cached_info_path);
    free(app->cached_info_text);
    app->cached_info_path = NULL;
    app->cached_info_text = NULL;
    app->cached_panel_width = 0;
}

static const entry_t *selected_entry(const app_t *app) {
    if (!app->entries.count) {
        return NULL;
    }
    if (app->selection >= app->entries.count) {
        return NULL;
    }
    return &app->entries.items[app->selection];
}

static void build_info_cache(app_t *app, int panel_width) {
    const entry_t *entry = selected_entry(app);
    char new_path[1024];
    char *raw = NULL;
    char *wrapped = NULL;

    if (!entry) {
        if (!app->cached_info_text) {
            clear_info_cache(app);
            app->cached_info_path = xstrdup("");
            app->cached_info_text = xstrdup("No entries");
        }
        return;
    }

    /* Compute the canonical path for the currently selected entry */
    {
        const char *base = app->current_path ? app->current_path : "";
        if (strcmp(entry->name, "..") == 0) {
            char *tmp = parent_path(base);
            snprintf(new_path, sizeof(new_path), "%s", tmp);
            free(tmp);
        } else {
            char *tmp = join_path(base, entry->name);
            snprintf(new_path, sizeof(new_path), "%s", tmp);
            free(tmp);
        }
    }

    /* Return early if cache is still valid */
    if (app->cached_info_text &&
        app->cached_info_path &&
        strcmp(app->cached_info_path, new_path) == 0 &&
        app->cached_panel_width == panel_width) {
        return;
    }

    free(app->cached_info_path);
    app->cached_info_path = xstrdup(new_path);
    free(app->cached_info_text);
    app->cached_info_text = NULL;
    app->cached_panel_width = panel_width;

    if (entry->is_dir) {
        size_t dirs = 0;
        size_t files = 0;
        for (size_t i = 0; i < app->entries.count; i++) {
            if (strcmp(app->entries.items[i].name, "..") == 0) {
                continue;
            }
            if (app->entries.items[i].is_dir) {
                dirs++;
            } else {
                files++;
            }
        }
        char tmp[1024];
        snprintf(tmp, sizeof(tmp),
                 "Directory\n\npath: %s\nentries: %zu\nsubdirs: %zu\nfiles: %zu\n\nEnter opens a directory.\nBackspace goes to the parent.",
                 new_path[0] ? new_path : "/",
                 app->entries.count,
                 dirs,
                 files);
        raw = xstrdup(tmp);
    } else {
        const char *ext = path_ext(entry->name);
        char *full_path = join_path(app->current_path ? app->current_path : "", entry->name);
        if (!strcasecmp(ext, "mdx")) {
            raw = build_mdx_info(app, full_path);
        } else if (!strcasecmp(ext, "fdf")) {
            raw = build_fdf_info(app, full_path);
        } else if (is_image_ext(ext)) {
            raw = build_img_info(app, full_path);
        } else if (is_text_ext(ext)) {
            raw = build_text_preview(app, full_path);
        } else {
            char tmp[512];
            snprintf(tmp, sizeof(tmp),
                     "No specialized inspector\n\npath: %s\nextension: %s\n\nTry Enter on a folder, or select a .mdx/.fdf/.blp/.txt file.",
                     full_path,
                     ext[0] ? ext : "(none)");
            raw = xstrdup(tmp);
        }
        free(full_path);
    }

    if (!raw) {
        raw = xstrdup("(no info available)");
    }
    wrapped = wrap_text(raw, (size_t)(panel_width > 0 ? panel_width : 1));
    free(raw);
    app->cached_info_text = wrapped;
}

static void render(app_t *app) {
    tui_size_t size;
    int cols;
    int rows;

    if (app->dump_mode) {
        /* In dump mode stdout is a pipe, so ioctl would read stdin's real terminal
           size instead. Use only the COLUMNS/LINES env vars (or defaults). */
        const char *ce = getenv("COLUMNS");
        const char *re = getenv("LINES");
        int cv = ce ? atoi(ce) : 0;
        int rv = re ? atoi(re) : 0;
        size.cols = cv > 0 ? cv : 120;
        size.rows = rv > 0 ? rv : 32;
    } else {
        size = tui_get_size();
    }
    cols = size.cols;
    rows = size.rows;
    int left_w;
    int right_w;
    int body_h = rows - 4;
    size_t visible = body_h > 0 ? (size_t)body_h : 0;
    char **info_lines = NULL;
    size_t info_count = 0;
    size_t info_cap = 0;

    if (body_h < 0) {
        body_h = 0;
        visible = 0;
    }

    if (cols < 37) {
        left_w = cols / 2;
        right_w = cols - left_w - 1;
    } else {
        left_w = (cols * 46) / 100;
        if (left_w < 18) {
            left_w = 18;
        }
        if (left_w > cols - 19) {
            left_w = cols - 19;
        }
        right_w = cols - left_w - 1;
    }
    if (left_w < 1) {
        left_w = 1;
    }
    if (right_w < 1) {
        right_w = 1;
    }
    ensure_scroll(app, visible ? visible : 1);
    build_info_cache(app, right_w);

    if (app->cached_info_text) {
        char *copy = xstrdup(app->cached_info_text);
        char *cursor = copy;
        while (cursor && *cursor) {
            char *line = cursor;
            char *nl = strchr(cursor, '\n');
            if (nl) {
                *nl = '\0';
                cursor = nl + 1;
            } else {
                cursor += strlen(cursor);
            }
            if (info_count == info_cap) {
                size_t next = info_cap ? info_cap * 2 : 32;
                info_lines = xrealloc(info_lines, next * sizeof(*info_lines));
                info_cap = next;
            }
            info_lines[info_count++] = xstrdup(line);
        }
        if (info_count == 0) {
            info_lines = xrealloc(info_lines, sizeof(*info_lines));
            info_lines[info_count++] = xstrdup("");
        }
        free(copy);
    }

    if (!app->dump_mode) {
        tui_clear_home();
    }
    {
        char title[1024];
        snprintf(title, sizeof(title), "mpqnc  archive: %s", app->mpq_path ? app->mpq_path : "");
        tui_write_clipped(title, cols);
    }
    fputc('\n', stdout);
    {
        char path[1024];
        snprintf(path, sizeof(path), "path: %s", app->current_path && app->current_path[0] ? app->current_path : "/");
        tui_write_clipped(path, cols);
    }
    fputc('\n', stdout);
    tui_write_cell("FILES", left_w, false);
    fputc('|', stdout);
    tui_write_cell("INFO", right_w, false);
    fputc('\n', stdout);

    for (size_t row = 0; row < visible; row++) {
        size_t idx = app->scroll + row;
        const entry_t *e = idx < app->entries.count ? &app->entries.items[idx] : NULL;
        const char *name = e ? e->name : "";
        char cell[1024];
        char right[1024];
        const char *suffix = (e && e->is_dir) ? "/" : "";
        if (e) {
            snprintf(cell, sizeof(cell), "%s%s", name, suffix);
        } else {
            cell[0] = '\0';
        }
        if (row < info_count) {
            snprintf(right, sizeof(right), "%s", info_lines[row]);
        } else {
            right[0] = '\0';
        }

        tui_write_cell(cell, left_w, e && idx == app->selection);
        fputc('|', stdout);
        tui_write_cell(right, right_w, false);
        fputc('\n', stdout);
    }

    if (visible < (size_t)body_h) {
        for (size_t row = visible; row < (size_t)body_h; row++) {
            tui_write_cell("", left_w, false);
            fputc('|', stdout);
            tui_write_cell("", right_w, false);
            fputc('\n', stdout);
        }
    }

    tui_write_clipped("j/k or arrows move, Enter opens folders, Backspace goes up, q quits, r refresh", cols);
    fputc('\n', stdout);
    fflush(stdout);

    for (size_t i = 0; i < info_count; i++) {
        free(info_lines[i]);
    }
    free(info_lines);
}

static int read_byte(int fd, unsigned char *c) {
    ssize_t got = read(fd, c, 1);
    if (got == 1) {
        return 1;
    }
    if (got < 0 && errno == EINTR) {
        return 0;
    }
    return -1;
}

static bool stdin_ready(int timeout_ms) {
    fd_set set;
    struct timeval tv;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

static browser_key_t read_key(void) {
    unsigned char c = 0;
    if (read_byte(STDIN_FILENO, &c) <= 0) {
        return KEY_NONE;
    }

    switch (c) {
        case 'q':
        case 'Q':
            return KEY_QUIT;
        case 'r':
        case 'R':
            return KEY_REFRESH;
        case 'j':
        case 'J':
            return KEY_DOWN;
        case 'k':
        case 'K':
            return KEY_UP;
        case 'h':
        case 'H':
        case '\b':
        case 127:
            return KEY_LEFT;
        case 'l':
        case 'L':
        case '\r':
        case '\n':
            return KEY_ENTER;
        case '\x1b':
            if (!stdin_ready(10)) {
                return KEY_QUIT;
            }
            if (read_byte(STDIN_FILENO, &c) <= 0 || c != '[') {
                return KEY_NONE;
            }
            if (read_byte(STDIN_FILENO, &c) <= 0) {
                return KEY_NONE;
            }
            if (c == 'A') return KEY_UP;
            if (c == 'B') return KEY_DOWN;
            if (c == 'C') return KEY_RIGHT;
            if (c == 'D') return KEY_LEFT;
            if (c == '5') {
                if (stdin_ready(10)) {
                    read_byte(STDIN_FILENO, &c);
                }
                return KEY_PAGEUP;
            }
            if (c == '6') {
                if (stdin_ready(10)) {
                    read_byte(STDIN_FILENO, &c);
                }
                return KEY_PAGEDOWN;
            }
            return KEY_NONE;
        default:
            return KEY_NONE;
    }
}

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  mpqnc -mpq <archive.mpq> [-path <archive/path>] [-mpqtool <path>] [-mdxtool <path>] [-fdftool <path>] [--dump]\n"
            "\n"
            "Options:\n"
            "  --dump          Render one frame to stdout and quit (no TTY required; useful for testing)\n"
            "\n"
            "Keyboard:\n"
            "  Up/Down or j/k  move selection\n"
            "  Enter/Right     open directory\n"
            "  Backspace/Left  go up one directory\n"
            "  r               refresh\n"
            "  q               quit\n");
}

static void app_init(app_t *app) {
    memset(app, 0, sizeof(*app));
    app->mpqtool_path = xstrdup("build/bin/mpqtool");
    app->mdxtool_path = xstrdup("build/bin/mdxtool");
    app->fdftool_path = xstrdup("build/bin/fdftool");
}

static void app_destroy(app_t *app) {
    free(app->mpq_path);
    free(app->start_path);
    free(app->mpqtool_path);
    free(app->mdxtool_path);
    free(app->fdftool_path);
    free(app->current_path);
    clear_info_cache(app);
    free_entries(&app->entries);
}

static void app_set_path(app_t *app, const char *path) {
    free(app->current_path);
    app->current_path = xstrdup(path ? path : "");
}

static void open_selected(app_t *app) {
    const entry_t *entry = selected_entry(app);
    if (!entry) {
        return;
    }
    if (strcmp(entry->name, "..") == 0) {
        char *parent = parent_path(app->current_path ? app->current_path : "");
        app_set_path(app, parent);
        free(parent);
        reset_selection(app);
        return;
    }
    if (!entry->is_dir) {
        return;
    }
    char *next = join_path(app->current_path ? app->current_path : "", entry->name);
    app_set_path(app, next);
    free(next);
    reset_selection(app);
}

int main(int argc, char **argv) {
    app_t app;
    bool running = true;

    app_init(&app);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-mpq") || !strcmp(argv[i], "--mpq")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            free(app.mpq_path);
            app.mpq_path = xstrdup(argv[i]);
        } else if (!strncmp(argv[i], "-mpq=", 5)) {
            free(app.mpq_path);
            app.mpq_path = xstrdup(argv[i] + 5);
        } else if (!strcmp(argv[i], "-path") || !strcmp(argv[i], "--path")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            free(app.start_path);
            app.start_path = xstrdup(argv[i]);
        } else if (!strncmp(argv[i], "-path=", 6)) {
            free(app.start_path);
            app.start_path = xstrdup(argv[i] + 6);
        } else if (!strcmp(argv[i], "-mpqtool") || !strcmp(argv[i], "--mpqtool")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            free(app.mpqtool_path);
            app.mpqtool_path = xstrdup(argv[i]);
        } else if (!strncmp(argv[i], "-mpqtool=", 9)) {
            free(app.mpqtool_path);
            app.mpqtool_path = xstrdup(argv[i] + 9);
        } else if (!strcmp(argv[i], "-mdxtool") || !strcmp(argv[i], "--mdxtool")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            free(app.mdxtool_path);
            app.mdxtool_path = xstrdup(argv[i]);
        } else if (!strncmp(argv[i], "-mdxtool=", 9)) {
            free(app.mdxtool_path);
            app.mdxtool_path = xstrdup(argv[i] + 9);
        } else if (!strcmp(argv[i], "-fdftool") || !strcmp(argv[i], "--fdftool")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            free(app.fdftool_path);
            app.fdftool_path = xstrdup(argv[i]);
        } else if (!strncmp(argv[i], "-fdftool=", 9)) {
            free(app.fdftool_path);
            app.fdftool_path = xstrdup(argv[i] + 9);
        } else if (!strcmp(argv[i], "-dump") || !strcmp(argv[i], "--dump")) {
            app.dump_mode = true;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage();
            app_destroy(&app);
            return 0;
        } else if (argv[i][0] != '-' && !app.mpq_path) {
            free(app.mpq_path);
            app.mpq_path = xstrdup(argv[i]);
        } else {
            usage();
            app_destroy(&app);
            return 1;
        }
    }

    if (!app.mpq_path) {
        usage();
        app_destroy(&app);
        return 1;
    }

    if (app.start_path) {
        app_set_path(&app, app.start_path);
    } else {
        app_set_path(&app, "");
    }

    if (app.dump_mode) {
        load_entries(&app);
        reset_selection(&app);
        render(&app);
        app_destroy(&app);
        return 0;
    }

    terminal_enter();
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGQUIT, on_signal);
    signal(SIGWINCH, on_signal);
    atexit(terminal_leave);

    load_entries(&app);
    reset_selection(&app);
    render(&app);

    while (running) {
        if (g_resize_pending) {
            g_resize_pending = 0;
            render(&app);
        }
        browser_key_t key = read_key();
        if (key == KEY_NONE) {
            continue;
        }
        switch (key) {
            case KEY_QUIT:
                running = false;
                break;
            case KEY_UP:
                if (app.selection > 0) {
                    app.selection--;
                }
                break;
            case KEY_DOWN:
                if (app.selection + 1 < app.entries.count) {
                    app.selection++;
                }
                break;
            case KEY_LEFT:
                if (app.current_path && app.current_path[0]) {
                    char *parent = parent_path(app.current_path);
                    app_set_path(&app, parent);
                    free(parent);
                    load_entries(&app);
                    reset_selection(&app);
                }
                break;
            case KEY_RIGHT:
            case KEY_ENTER:
                if (selected_entry(&app) && selected_entry(&app)->is_dir) {
                    open_selected(&app);
                    load_entries(&app);
                    reset_selection(&app);
                }
                break;
            case KEY_PAGEUP:
                if (app.selection > 10) {
                    app.selection -= 10;
                } else {
                    app.selection = 0;
                }
                break;
            case KEY_PAGEDOWN:
                if (app.selection + 10 < app.entries.count) {
                    app.selection += 10;
                } else if (app.entries.count > 0) {
                    app.selection = app.entries.count - 1;
                }
                break;
            case KEY_REFRESH:
                load_entries(&app);
                break;
            case KEY_NONE:
            default:
                break;
        }
        render(&app);
    }

    app_destroy(&app);
    return 0;
}
