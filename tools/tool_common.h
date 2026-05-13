#ifndef tool_common_h
#define tool_common_h

#include "../common/common.h"

#include <stdlib.h>
#include <string.h>

HANDLE Tool_AddArchive(HANDLE *archives, size_t count, LPCSTR filename);
HANDLE Tool_OpenFile(HANDLE const *archives, size_t count, LPCSTR fileName);
void Tool_CloseFile(HANDLE file);
bool Tool_ExtractFile(HANDLE const *archives, size_t count, LPCSTR toExtract, LPCSTR extracted);
bool Tool_FileExists(HANDLE const *archives, size_t count, LPCSTR fileName);
void Tool_CloseArchives(HANDLE *archives, size_t count);

HANDLE Tool_MemAlloc(long size);
void Tool_MemFree(HANDLE mem);
void *Tool_XMalloc(size_t size);
void *Tool_XRealloc(void *ptr, size_t size);
char *Tool_XStrdup(const char *s);

void Tool_NormalizeSlashes(char *path, char slash);
void Tool_TrimEdgeSlashes(char *path);
char *Tool_PathJoin(const char *base, const char *name);
char *Tool_PathParent(const char *path);
const char *Tool_PathBasename(const char *path);
const char *Tool_PathExt(const char *path);

#ifndef TOOL_COMMON_NO_MPQ
#include "../common/mpq.h"
#include "../common/mpq.c"

HANDLE Tool_AddArchive(HANDLE *archives, size_t count, LPCSTR filename) {
    for (size_t i = 0; i < count; i++) {
        if (archives[i]) {
            continue;
        }
        if (!SFileOpenArchive(filename, 0, 0, &archives[i])) {
            fprintf(stderr, "Can't add archive %s\n", filename);
            return NULL;
        }
        return archives[i];
    }
    fprintf(stderr, "Too many archives\n");
    return NULL;
}

HANDLE Tool_OpenFile(HANDLE const *archives, size_t count, LPCSTR fileName) {
    if (!fileName || !*fileName) {
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        HANDLE file = NULL;
        if (archives[i] && SFileOpenFileEx(archives[i], fileName, SFILE_OPEN_FROM_MPQ, &file)) {
            return file;
        }
    }
    return NULL;
}

void Tool_CloseFile(HANDLE file) {
    if (file) {
        SFileCloseFile(file);
    }
}

bool Tool_ExtractFile(HANDLE const *archives, size_t count, LPCSTR toExtract, LPCSTR extracted) {
    for (size_t i = 0; i < count; i++) {
        if (archives[i] && SFileExtractFile(archives[i], toExtract, extracted, 0)) {
            return true;
        }
    }
    return false;
}

bool Tool_FileExists(HANDLE const *archives, size_t count, LPCSTR fileName) {
    HANDLE file = Tool_OpenFile(archives, count, fileName);
    if (!file) {
        return false;
    }
    Tool_CloseFile(file);
    return true;
}

void Tool_CloseArchives(HANDLE *archives, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (archives[i]) {
            SFileCloseArchive(archives[i]);
            archives[i] = NULL;
        }
    }
}
#endif

HANDLE Tool_MemAlloc(long size) {
    void *mem = calloc(1, (size_t)size);
    if (!mem) {
        fprintf(stderr, "Out of memory allocating %ld bytes\n", size);
        exit(1);
    }
    return mem;
}

void Tool_MemFree(HANDLE mem) {
    free(mem);
}

void *Tool_XMalloc(size_t size) {
    void *ptr = calloc(1, size ? size : 1);
    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return ptr;
}

void *Tool_XRealloc(void *ptr, size_t size) {
    void *next = realloc(ptr, size ? size : 1);
    if (!next) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return next;
}

char *Tool_XStrdup(const char *s) {
    size_t len = s ? strlen(s) : 0;
    char *copy = Tool_XMalloc(len + 1);
    if (len) {
        memcpy(copy, s, len);
    }
    copy[len] = '\0';
    return copy;
}

void Tool_NormalizeSlashes(char *path, char slash) {
    if (!path) {
        return;
    }
    for (; *path; path++) {
        if (*path == '/' || *path == '\\') {
            *path = slash;
        }
    }
}

void Tool_TrimEdgeSlashes(char *path) {
    size_t len;

    if (!path) {
        return;
    }

    len = strlen(path);
    while (len > 0 && (path[len - 1] == '\\' || path[len - 1] == '/')) {
        path[--len] = '\0';
    }
    while (*path == '\\' || *path == '/') {
        memmove(path, path + 1, strlen(path));
    }
}

char *Tool_PathJoin(const char *base, const char *name) {
    size_t base_len = base ? strlen(base) : 0;
    size_t name_len = name ? strlen(name) : 0;
    bool need_slash = base_len > 0 && name_len > 0;
    char *out = Tool_XMalloc(base_len + name_len + (need_slash ? 2 : 1));

    if (need_slash) {
        snprintf(out, base_len + name_len + 2, "%s/%s", base, name);
    } else if (base_len > 0) {
        snprintf(out, base_len + 1, "%s", base);
    } else {
        snprintf(out, name_len + 1, "%s", name ? name : "");
    }
    return out;
}

char *Tool_PathParent(const char *path) {
    char *copy = Tool_XStrdup(path ? path : "");
    char *slash;

    Tool_NormalizeSlashes(copy, '/');
    slash = strrchr(copy, '/');
    if (slash) {
        *slash = '\0';
    } else {
        copy[0] = '\0';
    }
    return copy;
}

const char *Tool_PathBasename(const char *path) {
    const char *slash;
    const char *back;
    const char *base;

    if (!path) {
        return "";
    }

    slash = strrchr(path, '/');
    back = strrchr(path, '\\');
    base = slash;
    if (!base || (back && back > base)) {
        base = back;
    }
    return base ? base + 1 : path;
}

const char *Tool_PathExt(const char *path) {
    const char *base = Tool_PathBasename(path);
    const char *dot = strrchr(base, '.');
    return dot ? dot + 1 : "";
}

#endif
