#include "common/mpq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef PATHSTR
#define PATHSTR char[512]
#endif

typedef struct {
    char name[512];
    bool is_dir;
} entry_t;

typedef struct {
    entry_t *items;
    size_t count;
    size_t cap;
} entry_list_t;

static void usage(void) {
    fprintf(stderr,
        "Usage:\n"
        "  mpqtool -mpq <archive.mpq> ls [path]\n"
        "  mpqtool -mpq <archive.mpq> cat <file>\n"
        "\n"
        "Examples:\n"
        "  mpqtool -mpq War3.mpq ls\n"
        "  mpqtool -mpq War3.mpq ls Units\n"
        "  mpqtool -mpq War3.mpq cat Units/UnitData.slk\n");
}

static void normalize_slashes(char *s) {
    for (; *s; s++) {
        if (*s == '/') {
            *s = '\\';
        }
    }
}

static void trim_edge_slashes(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\\' || s[len - 1] == '/')) {
        s[--len] = '\0';
    }
    while (*s == '\\' || *s == '/') {
        memmove(s, s + 1, strlen(s));
    }
}

static bool starts_with_ci(const char *s, const char *prefix) {
#ifdef _WIN32
    return _strnicmp(s, prefix, strlen(prefix)) == 0;
#else
    return strncasecmp(s, prefix, strlen(prefix)) == 0;
#endif
}

static void entries_add_or_merge(entry_list_t *list, const char *name, bool is_dir) {
    size_t i;
    for (i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].name, name) == 0) {
            list->items[i].is_dir = list->items[i].is_dir || is_dir;
            return;
        }
    }

    if (list->count == list->cap) {
        size_t next = (list->cap == 0) ? 64 : list->cap * 2;
        entry_t *tmp = (entry_t *)realloc(list->items, next * sizeof(*tmp));
        if (!tmp) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
        list->items = tmp;
        list->cap = next;
    }

    strncpy(list->items[list->count].name, name, sizeof(list->items[list->count].name) - 1);
    list->items[list->count].name[sizeof(list->items[list->count].name) - 1] = '\0';
    list->items[list->count].is_dir = is_dir;
    list->count++;
}

static int entry_cmp(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *)a;
    const entry_t *eb = (const entry_t *)b;
#ifdef _WIN32
    return _stricmp(ea->name, eb->name);
#else
    return strcasecmp(ea->name, eb->name);
#endif
}

static int cmd_cat(HANDLE archive, const char *file_path) {
    HANDLE file;
    unsigned char buf[64 * 1024];
    DWORD read_bytes = 0;

    if (!SFileOpenFileEx(archive, file_path, SFILE_OPEN_FROM_MPQ, &file)) {
        fprintf(stderr, "Cannot open MPQ file: %s\n", file_path);
        return 1;
    }

    while (SFileReadFile(file, buf, sizeof(buf), &read_bytes, NULL) && read_bytes > 0) {
        size_t wrote = fwrite(buf, 1, read_bytes, stdout);
        if (wrote != read_bytes) {
            fprintf(stderr, "Failed writing output\n");
            SFileCloseFile(file);
            return 1;
        }
    }

    SFileCloseFile(file);
    return 0;
}

static int cmd_ls(HANDLE archive, const char *path) {
    SFILE_FIND_DATA fd;
    HANDLE hfind;
    char prefix[512];
    size_t prefix_len;
    entry_list_t out = { 0 };

    prefix[0] = '\0';
    if (path && *path) {
        strncpy(prefix, path, sizeof(prefix) - 1);
        prefix[sizeof(prefix) - 1] = '\0';
        normalize_slashes(prefix);
        trim_edge_slashes(prefix);
    }

    prefix_len = strlen(prefix);
    hfind = SFileFindFirstFile(archive, "*", &fd, NULL);
    if (!hfind) {
        fprintf(stderr, "Unable to enumerate archive contents\n");
        return 1;
    }

    do {
        const char *full = fd.cFileName;
        const char *rest = full;

        if (prefix_len > 0) {
            if (!starts_with_ci(full, prefix)) {
                continue;
            }
            rest = full + prefix_len;
            if (*rest == '\\') {
                rest++;
            } else if (*rest != '\0') {
                continue;
            }
            if (*rest == '\0') {
                continue;
            }
        }

        {
            const char *slash = strchr(rest, '\\');
            char child[512];
            bool is_dir = (slash != NULL);
            size_t n = is_dir ? (size_t)(slash - rest) : strlen(rest);
            if (n == 0 || n >= sizeof(child)) {
                continue;
            }
            memcpy(child, rest, n);
            child[n] = '\0';
            entries_add_or_merge(&out, child, is_dir);
        }
    } while (SFileFindNextFile(hfind, &fd));

    SFileFindClose(hfind);

    qsort(out.items, out.count, sizeof(*out.items), entry_cmp);
    for (size_t i = 0; i < out.count; i++) {
        if (out.items[i].is_dir) {
            printf("%s/\n", out.items[i].name);
        } else {
            printf("%s\n", out.items[i].name);
        }
    }

    free(out.items);
    return 0;
}

int main(int argc, char **argv) {
    const char *mpq = NULL;
    const char *cmd = NULL;
    const char *arg = NULL;
    HANDLE archive;
    int rc;

    if (argc < 4) {
        usage();
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-mpq") == 0) {
            if (i + 1 >= argc) {
                usage();
                return 1;
            }
            mpq = argv[++i];
        } else if (!cmd) {
            cmd = argv[i];
        } else if (!arg) {
            arg = argv[i];
        } else {
            usage();
            return 1;
        }
    }

    if (!mpq || !cmd) {
        usage();
        return 1;
    }

    if (!SFileOpenArchive(mpq, 0, 0, &archive)) {
        fprintf(stderr, "Cannot open archive: %s\n", mpq);
        return 1;
    }

    if (strcmp(cmd, "ls") == 0) {
        rc = cmd_ls(archive, arg ? arg : "");
    } else if (strcmp(cmd, "cat") == 0) {
        char path[512];
        if (!arg) {
            usage();
            SFileCloseArchive(archive);
            return 1;
        }
        strncpy(path, arg, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        normalize_slashes(path);
        trim_edge_slashes(path);
        rc = cmd_cat(archive, path);
    } else {
        usage();
        rc = 1;
    }

    SFileCloseArchive(archive);
    return rc;
}
