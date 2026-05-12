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
        "  mpqtool -mpq <archive.mpq> imginfo <file>\n"
    "  mpqtool -mpq <archive.mpq> create [max-files]\n"
    "  mpqtool -mpq <archive.mpq> pack <src> <archive-file> [<src> <archive-file> ...]\n"
        "\n"
        "Examples:\n"
        "  mpqtool -mpq War3.mpq ls\n"
        "  mpqtool -mpq War3.mpq ls Units\n"
        "  mpqtool -mpq War3.mpq cat Units/UnitData.slk\n"
    "  mpqtool -mpq tests.mpq create 32\n"
    "  mpqtool -mpq tests.mpq pack ./basic.fdf TestUI/Frames/basic.fdf ./checker.blp TestUI/Textures/checker.blp\n"
    "  mpqtool -mpq War3.mpq imginfo UI/Widgets/Glues/GlueScreen-Button1-Border.blp\n");
}

static unsigned short rd_u16le(const unsigned char *p) {
    return (unsigned short)(p[0] | (p[1] << 8));
}

static unsigned int rd_u32le(const unsigned char *p) {
    return (unsigned int)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static const char *blp_content_name(unsigned int content) {
    switch (content) {
        case 0: return "JPEG";
        case 1: return "DIRECT_PALETTED";
        default: return "UNKNOWN";
    }
}

static const char *blp2_encoding_name(unsigned int encoding) {
    switch (encoding) {
        case 0: return "JPEG";
        case 1: return "PALETTIZED";
        case 2: return "DXT";
        case 3: return "BGRA";
        default: return "UNKNOWN";
    }
}

static int cmd_imginfo(HANDLE archive, const char *file_path) {
    HANDLE file;
    unsigned char header[512];
    DWORD read_bytes = 0;
    DWORD file_size = 0;

    if (!SFileOpenFileEx(archive, file_path, SFILE_OPEN_FROM_MPQ, &file)) {
        fprintf(stderr, "Cannot open MPQ file: %s\n", file_path);
        return 1;
    }

    file_size = SFileGetFileSize(file, NULL);
    if (!SFileReadFile(file, header, sizeof(header), &read_bytes, NULL) || read_bytes < 16) {
        fprintf(stderr, "Failed reading file header: %s\n", file_path);
        SFileCloseFile(file);
        return 1;
    }

    printf("file=%s\n", file_path);
    printf("size=%u\n", (unsigned)file_size);

    if (read_bytes >= 4 && memcmp(header, "BLP1", 4) == 0) {
        unsigned int content;
        unsigned int alpha_bits;
        unsigned int width;
        unsigned int height;
        unsigned int extra;
        unsigned int has_mips;
        unsigned int mip_count = 0;

        if (read_bytes < 0x9C) {
            fprintf(stderr, "BLP1 header too small in %s\n", file_path);
            SFileCloseFile(file);
            return 1;
        }

        content = rd_u32le(header + 0x04);
        alpha_bits = rd_u32le(header + 0x08);
        width = rd_u32le(header + 0x0C);
        height = rd_u32le(header + 0x10);
        extra = rd_u32le(header + 0x14);
        has_mips = rd_u32le(header + 0x18);

        for (unsigned int i = 0; i < 16; i++) {
            unsigned int off = rd_u32le(header + 0x1C + i * 4);
            unsigned int sz = rd_u32le(header + 0x5C + i * 4);
            if (off != 0 && sz != 0) {
                mip_count++;
            }
        }

        printf("type=BLP1\n");
        printf("content=%u (%s)\n", content, blp_content_name(content));
        printf("alpha_bits=%u\n", alpha_bits);
        printf("width=%u\n", width);
        printf("height=%u\n", height);
        printf("extra=%u\n", extra);
        printf("has_mipmaps=%u\n", has_mips ? 1u : 0u);
        printf("mip_levels_present=%u\n", mip_count);

        for (unsigned int i = 0; i < 16; i++) {
            unsigned int off = rd_u32le(header + 0x1C + i * 4);
            unsigned int sz = rd_u32le(header + 0x5C + i * 4);
            if (off == 0 && sz == 0) {
                continue;
            }
            printf("mip[%u].offset=%u mip[%u].size=%u\n", i, off, i, sz);
        }
    } else if (read_bytes >= 4 && memcmp(header, "BLP2", 4) == 0) {
        unsigned int version;
        unsigned int encoding;
        unsigned int alpha_bits;
        unsigned int alpha_type;
        unsigned int has_mips;
        unsigned int width;
        unsigned int height;
        unsigned int mip_count = 0;

        if (read_bytes < 0x94) {
            fprintf(stderr, "BLP2 header too small in %s\n", file_path);
            SFileCloseFile(file);
            return 1;
        }

        version = rd_u32le(header + 0x04);
        encoding = header[0x08];
        alpha_bits = header[0x09];
        alpha_type = header[0x0A];
        has_mips = header[0x0B];
        width = rd_u32le(header + 0x0C);
        height = rd_u32le(header + 0x10);

        for (unsigned int i = 0; i < 16; i++) {
            unsigned int off = rd_u32le(header + 0x14 + i * 4);
            unsigned int sz = rd_u32le(header + 0x54 + i * 4);
            if (off != 0 && sz != 0) {
                mip_count++;
            }
        }

        printf("type=BLP2\n");
        printf("version=%u\n", version);
        printf("encoding=%u (%s)\n", encoding, blp2_encoding_name(encoding));
        printf("alpha_bits=%u\n", alpha_bits);
        printf("alpha_type=%u\n", alpha_type);
        printf("width=%u\n", width);
        printf("height=%u\n", height);
        printf("has_mipmaps=%u\n", has_mips ? 1u : 0u);
        printf("mip_levels_present=%u\n", mip_count);

        for (unsigned int i = 0; i < 16; i++) {
            unsigned int off = rd_u32le(header + 0x14 + i * 4);
            unsigned int sz = rd_u32le(header + 0x54 + i * 4);
            if (off == 0 && sz == 0) {
                continue;
            }
            printf("mip[%u].offset=%u mip[%u].size=%u\n", i, off, i, sz);
        }
    } else if (read_bytes >= 8 &&
               header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G') {
        printf("type=PNG\n");
        if (read_bytes >= 24) {
            unsigned int width = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) | header[19];
            unsigned int height = (header[20] << 24) | (header[21] << 16) | (header[22] << 8) | header[23];
            printf("width=%u\n", width);
            printf("height=%u\n", height);
        }
    } else if (read_bytes >= 3 && header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
        printf("type=JPEG\n");
    } else if (read_bytes >= 18) {
        unsigned int id_len = header[0];
        unsigned int image_type = header[2];
        unsigned int width = rd_u16le(header + 12);
        unsigned int height = rd_u16le(header + 14);
        unsigned int bpp = header[16];

        if (image_type != 0 && width > 0 && height > 0) {
            printf("type=TGA\n");
            printf("image_type=%u\n", image_type);
            printf("id_length=%u\n", id_len);
            printf("width=%u\n", width);
            printf("height=%u\n", height);
            printf("bits_per_pixel=%u\n", bpp);
        } else {
            printf("type=UNKNOWN\n");
        }
    } else {
        printf("type=UNKNOWN\n");
    }

    SFileCloseFile(file);
    return 0;
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

static int cmd_create(const char *mpq_path, const char *arg)
{
    HANDLE archive;
    DWORD max_files = 16;

    if (arg && *arg) {
        char *end = NULL;
        unsigned long parsed = strtoul(arg, &end, 10);
        if (!end || *end != '\0' || parsed == 0 || parsed > 65535) {
            fprintf(stderr, "Invalid max-files value: %s\n", arg);
            return 1;
        }
        max_files = (DWORD)parsed;
    }

    if (!SFileCreateArchive(mpq_path, 0, max_files, &archive)) {
        fprintf(stderr, "Cannot create archive: %s\n", mpq_path);
        return 1;
    }
    if (!SFileCloseArchive(archive)) {
        fprintf(stderr, "Cannot finalize archive: %s\n", mpq_path);
        return 1;
    }
    return 0;
}

static int cmd_pack(const char *mpq_path, int pair_count, char **pairs)
{
    HANDLE archive;
    int i;

    if (pair_count <= 0 || (pair_count % 2) != 0) {
        fprintf(stderr, "pack requires <src> <archive-file> pairs\n");
        return 1;
    }

    if (!SFileCreateArchive(mpq_path, 0, (DWORD)(pair_count / 2 + 1), &archive)) {
        fprintf(stderr, "Cannot create archive: %s\n", mpq_path);
        return 1;
    }

    for (i = 0; i < pair_count; i += 2) {
        char path[512];
        strncpy(path, pairs[i + 1], sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        normalize_slashes(path);
        trim_edge_slashes(path);
        if (!SFileAddFile(archive, pairs[i], path)) {
            fprintf(stderr, "Cannot add file %s as %s\n", pairs[i], path);
            SFileCloseArchive(archive);
            return 1;
        }
    }

    if (!SFileCloseArchive(archive)) {
        fprintf(stderr, "Cannot finalize archive: %s\n", mpq_path);
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    const char *mpq = NULL;
    const char *cmd = NULL;
    const char *arg = NULL;
    char **extra = NULL;
    int extra_count = 0;
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
            extra = &argv[i];
            extra_count = argc - i;
            break;
        }
    }

    if (!mpq || !cmd) {
        usage();
        return 1;
    }

    if (strcmp(cmd, "create") == 0) {
        return cmd_create(mpq, arg);
    } else if (strcmp(cmd, "pack") == 0) {
        if (!arg) {
            usage();
            return 1;
        }
        if (!extra) {
            fprintf(stderr, "pack requires <src> <archive-file> pairs\n");
            return 1;
        }
        {
            char *pairs[256];
            int pair_count = 0;
            pairs[pair_count++] = (char *)arg;
            for (int i = 0; i < extra_count && pair_count < (int)(sizeof(pairs) / sizeof(pairs[0])); i++) {
                pairs[pair_count++] = extra[i];
            }
            return cmd_pack(mpq, pair_count, pairs);
        }
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
    } else if (strcmp(cmd, "imginfo") == 0) {
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
        rc = cmd_imginfo(archive, path);
    } else {
        usage();
        rc = 1;
    }

    SFileCloseArchive(archive);
    return rc;
}
