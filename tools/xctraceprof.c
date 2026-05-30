#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    long id;
    char *name;
} frame_name_t;

typedef struct {
    frame_name_t *items;
    size_t count;
    size_t cap;
} frame_names_t;

typedef struct {
    char *name;
    unsigned leaf;
    unsigned inclusive;
} symbol_count_t;

typedef struct {
    symbol_count_t *items;
    size_t count;
    size_t cap;
} symbol_counts_t;

typedef struct {
    double start;
    double end;
    int top;
    bool all_threads;
    char const *focus;
} options_t;

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  xctraceprof [--window <start:end>] [--top <n>] [--focus <symbol>] [--all-threads] <time-profile.xml>\n"
            "\n"
            "Input is XML exported from xctrace, for example:\n"
            "  xctrace export --input run.trace --xpath '/trace-toc/run[@number=\"1\"]/data/table[@schema=\"time-profile\"]' > time-profile.xml\n"
            "\n"
            "Examples:\n"
            "  xctraceprof /tmp/time-profile.xml\n"
            "  xctraceprof --window 8:18 --top 40 /tmp/time-profile.xml\n"
            "  xctraceprof --window 8:18 --focus R_RenderFogOfWar /tmp/time-profile.xml\n");
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

static char *xstrndup(char const *text, size_t len) {
    char *out = xmalloc(len + 1);
    memcpy(out, text, len);
    out[len] = '\0';
    return out;
}

static char *xstrdup(char const *text) {
    return xstrndup(text, strlen(text));
}

static char *read_text_file(char const *path, size_t *out_size) {
    FILE *file = fopen(path, "rb");
    char *buffer;
    long size;

    if (!file) {
        fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
        exit(1);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Cannot seek %s\n", path);
        exit(1);
    }
    size = ftell(file);
    if (size < 0) {
        fprintf(stderr, "Cannot size %s\n", path);
        exit(1);
    }
    rewind(file);

    buffer = xmalloc((size_t)size + 1);
    if (size && fread(buffer, 1, (size_t)size, file) != (size_t)size) {
        fprintf(stderr, "Cannot read %s\n", path);
        exit(1);
    }
    fclose(file);
    buffer[size] = '\0';
    if (out_size) {
        *out_size = (size_t)size;
    }
    return buffer;
}

static bool starts_with(char const *text, char const *prefix) {
    return !strncmp(text, prefix, strlen(prefix));
}

static void xml_unescape(char *text) {
    char *read = text;
    char *write = text;

    while (*read) {
        if (starts_with(read, "&lt;")) {
            *write++ = '<';
            read += 4;
        } else if (starts_with(read, "&gt;")) {
            *write++ = '>';
            read += 4;
        } else if (starts_with(read, "&amp;")) {
            *write++ = '&';
            read += 5;
        } else if (starts_with(read, "&quot;")) {
            *write++ = '"';
            read += 6;
        } else if (starts_with(read, "&apos;")) {
            *write++ = '\'';
            read += 6;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

static char const *tag_end(char const *tag) {
    char const *end = strchr(tag, '>');
    return end ? end : tag + strlen(tag);
}

static char *attr_dup(char const *tag, char const *attr) {
    char pattern[64];
    char const *end = tag_end(tag);
    char const *found;
    char const *value;
    char *out;

    snprintf(pattern, sizeof(pattern), "%s=\"", attr);
    found = strstr(tag, pattern);
    if (!found || found > end) {
        return NULL;
    }
    value = found + strlen(pattern);
    found = value;
    while (*found && found < end && *found != '"') {
        found++;
    }
    out = xstrndup(value, (size_t)(found - value));
    xml_unescape(out);
    return out;
}

static long attr_long(char const *tag, char const *attr, long fallback) {
    char *value = attr_dup(tag, attr);
    long out;

    if (!value) {
        return fallback;
    }
    out = strtol(value, NULL, 10);
    free(value);
    return out;
}

static void frame_names_add(frame_names_t *names, long id, char const *name) {
    for (size_t i = 0; i < names->count; i++) {
        if (names->items[i].id == id) {
            return;
        }
    }
    if (names->count == names->cap) {
        names->cap = names->cap ? names->cap * 2 : 1024;
        names->items = xrealloc(names->items, names->cap * sizeof(*names->items));
    }
    names->items[names->count].id = id;
    names->items[names->count].name = xstrdup(name ? name : "");
    names->count++;
}

static char const *frame_name(frame_names_t const *names, long id) {
    static char unknown[64];

    for (size_t i = 0; i < names->count; i++) {
        if (names->items[i].id == id) {
            return names->items[i].name;
        }
    }
    snprintf(unknown, sizeof(unknown), "<frame %ld>", id);
    return unknown;
}

static void collect_frame_names(char const *xml, frame_names_t *names) {
    char const *cursor = xml;

    while ((cursor = strstr(cursor, "<frame "))) {
        char *name = attr_dup(cursor, "name");
        long id = attr_long(cursor, "id", -1);

        if (id >= 0 && name) {
            frame_names_add(names, id, name);
        }
        free(name);
        cursor++;
    }
}

static symbol_count_t *symbol_count(symbol_counts_t *counts, char const *name) {
    for (size_t i = 0; i < counts->count; i++) {
        if (!strcmp(counts->items[i].name, name)) {
            return counts->items + i;
        }
    }
    if (counts->count == counts->cap) {
        counts->cap = counts->cap ? counts->cap * 2 : 1024;
        counts->items = xrealloc(counts->items, counts->cap * sizeof(*counts->items));
    }
    counts->items[counts->count].name = xstrdup(name);
    counts->items[counts->count].leaf = 0;
    counts->items[counts->count].inclusive = 0;
    return counts->items + counts->count++;
}

static bool symbol_seen(char const **seen, size_t count, char const *name) {
    for (size_t i = 0; i < count; i++) {
        if (!strcmp(seen[i], name)) {
            return true;
        }
    }
    return false;
}

static double row_time(char const *row, char const *row_end) {
    char const *tag = strstr(row, "<sample-time");
    char const *value;
    char const *end;

    if (!tag || tag > row_end) {
        return -1.0;
    }
    value = strchr(tag, '>');
    if (!value || value > row_end) {
        return -1.0;
    }
    value++;
    end = strstr(value, "</sample-time>");
    if (!end || end > row_end) {
        return -1.0;
    }
    return strtod(value, NULL) / 1000000000.0;
}

static long row_thread(char const *row, char const *row_end) {
    char const *tag = strstr(row, "<thread ");

    if (!tag || tag > row_end) {
        return -1;
    }
    return attr_long(tag, "id", attr_long(tag, "ref", -1));
}

static long find_main_thread(char const *xml) {
    char const *cursor = xml;

    while ((cursor = strstr(cursor, "<thread "))) {
        char *fmt = attr_dup(cursor, "fmt");
        long id = attr_long(cursor, "id", -1);
        bool main_thread = fmt && strstr(fmt, "Main Thread");

        free(fmt);
        if (id >= 0 && main_thread) {
            return id;
        }
        cursor++;
    }
    return -1;
}

static char *row_frame_name(char const *frame_tag, frame_names_t const *names) {
    char *name = attr_dup(frame_tag, "name");
    long ref;

    if (name) {
        return name;
    }
    ref = attr_long(frame_tag, "ref", -1);
    if (ref >= 0) {
        return xstrdup(frame_name(names, ref));
    }
    ref = attr_long(frame_tag, "id", -1);
    if (ref >= 0) {
        return xstrdup(frame_name(names, ref));
    }
    return NULL;
}

static void count_row_frames(char const *row, char const *row_end,
                             frame_names_t const *frames,
                             symbol_counts_t *counts,
                             unsigned *sample_count,
                             char const *focus) {
    char const *bt = strstr(row, "<backtrace");
    char const *bt_end;
    char const *cursor;
    char *stack[256];
    size_t stack_count = 0;
    char const *seen[256];
    size_t seen_count = 0;
    bool has_focus = focus == NULL;

    if (!bt || bt > row_end) {
        return;
    }
    bt_end = strstr(bt, "</backtrace>");
    if (!bt_end || bt_end > row_end) {
        return;
    }

    cursor = bt;
    while ((cursor = strstr(cursor, "<frame "))) {
        char *name;

        if (cursor > bt_end) {
            break;
        }
        name = row_frame_name(cursor, frames);
        if (name && *name) {
            if (focus && strstr(name, focus)) {
                has_focus = true;
            }
            if (stack_count < sizeof(stack) / sizeof(stack[0])) {
                stack[stack_count++] = name;
                name = NULL;
            }
        }
        free(name);
        cursor++;
    }

    if (!has_focus || stack_count == 0) {
        for (size_t i = 0; i < stack_count; i++) {
            free(stack[i]);
        }
        return;
    }

    symbol_count(counts, stack[0])->leaf++;
    for (size_t i = 0; i < stack_count; i++) {
        if (!symbol_seen(seen, seen_count, stack[i])) {
            symbol_count_t *count = symbol_count(counts, stack[i]);
            count->inclusive++;
            if (seen_count < sizeof(seen) / sizeof(seen[0])) {
                seen[seen_count++] = count->name;
            }
        }
        free(stack[i]);
    }
    (*sample_count)++;
}

static int compare_leaf(void const *a, void const *b) {
    symbol_count_t const *sa = a;
    symbol_count_t const *sb = b;
    if (sa->leaf != sb->leaf) {
        return sb->leaf < sa->leaf ? -1 : 1;
    }
    return strcmp(sa->name, sb->name);
}

static int compare_inclusive(void const *a, void const *b) {
    symbol_count_t const *sa = a;
    symbol_count_t const *sb = b;
    if (sa->inclusive != sb->inclusive) {
        return sb->inclusive < sa->inclusive ? -1 : 1;
    }
    return strcmp(sa->name, sb->name);
}

static void print_top(symbol_counts_t const *counts, int top,
                      int (*compare)(void const *, void const *),
                      bool inclusive) {
    symbol_count_t *copy = xmalloc(counts->count * sizeof(*copy));
    size_t limit;

    memcpy(copy, counts->items, counts->count * sizeof(*copy));
    qsort(copy, counts->count, sizeof(*copy), compare);
    limit = counts->count < (size_t)top ? counts->count : (size_t)top;
    for (size_t i = 0; i < limit; i++) {
        unsigned count = inclusive ? copy[i].inclusive : copy[i].leaf;
        if (!count) {
            break;
        }
        printf("%5u  %s\n", count, copy[i].name);
    }
    free(copy);
}

static void parse_window(char const *arg, options_t *opts) {
    char *colon;
    char *copy = xstrdup(arg);

    colon = strchr(copy, ':');
    if (!colon) {
        fprintf(stderr, "Invalid window '%s', expected start:end\n", arg);
        exit(1);
    }
    *colon = '\0';
    opts->start = strtod(copy, NULL);
    opts->end = strtod(colon + 1, NULL);
    if (opts->end <= opts->start) {
        fprintf(stderr, "Invalid window '%s'\n", arg);
        exit(1);
    }
    free(copy);
}

static char const *parse_args(int argc, char **argv, options_t *opts) {
    char const *input = NULL;

    opts->start = 0.0;
    opts->end = 1.0e30;
    opts->top = 30;
    opts->all_threads = false;
    opts->focus = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            usage();
            exit(0);
        } else if (!strcmp(argv[i], "--window") && i + 1 < argc) {
            parse_window(argv[++i], opts);
        } else if (!strcmp(argv[i], "--top") && i + 1 < argc) {
            opts->top = atoi(argv[++i]);
            if (opts->top <= 0) {
                opts->top = 30;
            }
        } else if (!strcmp(argv[i], "--focus") && i + 1 < argc) {
            opts->focus = argv[++i];
        } else if (!strcmp(argv[i], "--all-threads")) {
            opts->all_threads = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            exit(1);
        } else {
            input = argv[i];
        }
    }
    if (!input) {
        usage();
        exit(1);
    }
    return input;
}

int main(int argc, char **argv) {
    options_t opts;
    char const *input = parse_args(argc, argv, &opts);
    size_t xml_size;
    char *xml = read_text_file(input, &xml_size);
    frame_names_t frames = { 0 };
    symbol_counts_t counts = { 0 };
    long main_thread;
    unsigned samples = 0;
    unsigned rows = 0;
    char const *cursor = xml;

    collect_frame_names(xml, &frames);
    main_thread = find_main_thread(xml);

    while ((cursor = strstr(cursor, "<row>"))) {
        char const *row_end = strstr(cursor, "</row>");
        double time;
        long thread;

        if (!row_end) {
            break;
        }
        rows++;
        time = row_time(cursor, row_end);
        thread = row_thread(cursor, row_end);
        if (time >= opts.start && time < opts.end &&
            (opts.all_threads || main_thread < 0 || thread == main_thread))
        {
            count_row_frames(cursor, row_end, &frames, &counts, &samples, opts.focus);
        }
        cursor = row_end + strlen("</row>");
    }

    printf("input=%s\n", input);
    printf("xml_bytes=%zu rows=%u frame_names=%zu\n", xml_size, rows, frames.count);
    printf("thread=%s", opts.all_threads ? "all" : "main");
    if (!opts.all_threads && main_thread >= 0) {
        printf("(%ld)", main_thread);
    }
    if (opts.focus) {
        printf(" focus=\"%s\"", opts.focus);
    }
    printf(" window=%.3f:%.3f samples=%u symbols=%zu\n\n",
           opts.start,
           opts.end > 1.0e20 ? -1.0 : opts.end,
           samples,
           counts.count);

    printf("Top leaf symbols:\n");
    print_top(&counts, opts.top, compare_leaf, false);
    printf("\nTop inclusive symbols:\n");
    print_top(&counts, opts.top, compare_inclusive, true);

    free(xml);
    return 0;
}
