#ifndef tool_common_h
#define tool_common_h

#include "../common/common.h"

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

#endif
