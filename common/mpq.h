#ifndef MPQ_H
#define MPQ_H

#include "shared.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SFILE_OPEN_FROM_MPQ 0
#define SFILE_INVALID_POS ((DWORD)-1)

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef FILE_BEGIN
#define FILE_BEGIN 0
#endif

#ifndef FILE_CURRENT
#define FILE_CURRENT 1
#endif

#ifndef FILE_END
#define FILE_END 2
#endif

typedef struct {
    char cFileName[MAX_PATH];
    char *szPlainName;
    DWORD dwHashIndex;
    DWORD dwBlockIndex;
    DWORD dwFileSize;
    DWORD dwFileFlags;
    DWORD dwCompSize;
    DWORD dwFileTimeLo;
    DWORD dwFileTimeHi;
    LCID lcLocale;
} SFILE_FIND_DATA;

BOOL SFileOpenArchive(LPCSTR filename, DWORD priority, DWORD flags, HANDLE *archive);
BOOL SFileCloseArchive(HANDLE archive);

BOOL SFileOpenFileEx(HANDLE archive, LPCSTR fileName, DWORD searchScope, HANDLE *file);
BOOL SFileCloseFile(HANDLE file);

BOOL SFileReadFile(HANDLE file, void *buffer, DWORD toRead, LPDWORD bytesRead, LPOVERLAPPED overlapped);
DWORD SFileGetFileSize(HANDLE file, LPDWORD highSize);
DWORD SFileSetFilePointer(HANDLE file, LONG distance, PLONG distanceHigh, DWORD moveMethod);

BOOL SFileExtractFile(HANDLE archive, LPCSTR toExtract, LPCSTR extracted, DWORD flags);

HANDLE SFileFindFirstFile(HANDLE archive, LPCSTR mask, SFILE_FIND_DATA *findData, LPCSTR listFile);
BOOL SFileFindNextFile(HANDLE find, SFILE_FIND_DATA *findData);
BOOL SFileFindClose(HANDLE find);

#ifdef __cplusplus
}
#endif

#endif
