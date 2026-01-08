#ifndef __MPQ_ADAPTER_H__
#define __MPQ_ADAPTER_H__

/*
 * MPQ Adapter - Abstraction layer for MPQ archive access
 * 
 * This header provides a unified interface for MPQ archive operations,
 * supporting both StormLib (default) and libmpq (OpenBSD).
 * 
 * On OpenBSD, use libmpq since StormLib is not well supported.
 * On other platforms, use StormLib which is more feature-complete.
 */

#if defined(__OpenBSD__)
    #define USE_LIBMPQ 1
    #include <mpq.h>
#else
    #define USE_STORMLIB 1
    #include <StormLib.h>
#endif

#include "shared.h"

#ifdef USE_LIBMPQ

/* libmpq types and constants */
typedef mpq_archive_s* MPQ_ARCHIVE;
typedef struct {
    mpq_archive_s* archive;
    int file_number;
    off_t offset;  /* Current read position */
    unsigned char* cached_data;  /* Cached file data for efficient seeking */
    off_t cached_size;
} MPQ_FILE_HANDLE;

typedef MPQ_FILE_HANDLE* MPQ_FILE;

/* File opening flags (StormLib compatibility) */
#define SFILE_OPEN_FROM_MPQ 0

/* File pointer positioning methods */
#ifndef FILE_BEGIN
#define FILE_BEGIN 0
#endif
#ifndef FILE_CURRENT
#define FILE_CURRENT 1
#endif
#ifndef FILE_END
#define FILE_END 2
#endif

/* Find file data structure */
typedef struct {
    char cFileName[260];
    mpq_archive_s* archive;
    int current_file;
    int file_count;
} MPQ_FIND_DATA, *LPMPQ_FIND_DATA;

/* Find handle type */
typedef void* MPQ_FIND_HANDLE;

/* Adapter function prototypes */
BOOL MPQ_OpenArchive(LPCSTR szMpqName, DWORD dwPriority, DWORD dwFlags, MPQ_ARCHIVE* phMpq);
BOOL MPQ_CloseArchive(MPQ_ARCHIVE hMpq);
BOOL MPQ_OpenFileEx(MPQ_ARCHIVE hMpq, LPCSTR szFileName, DWORD dwSearchScope, MPQ_FILE* phFile);
BOOL MPQ_CloseFile(MPQ_FILE hFile);
BOOL MPQ_ReadFile(MPQ_FILE hFile, LPVOID lpBuffer, DWORD dwToRead, LPDWORD pdwRead, LPOVERLAPPED lpOverlapped);
DWORD MPQ_GetFileSize(MPQ_FILE hFile, LPDWORD pdwFileSizeHigh);
DWORD MPQ_SetFilePointer(MPQ_FILE hFile, LONG lFilePos, LONG* plFilePosHigh, DWORD dwMoveMethod);
BOOL MPQ_ExtractFile(MPQ_ARCHIVE hMpq, LPCSTR szToExtract, LPCSTR szExtracted, DWORD dwSearchScope);
MPQ_FIND_HANDLE MPQ_FindFirstFile(MPQ_ARCHIVE hMpq, LPCSTR szMask, LPMPQ_FIND_DATA lpFindFileData, LPCSTR szListFile);
BOOL MPQ_FindNextFile(MPQ_FIND_HANDLE hFind, LPMPQ_FIND_DATA lpFindFileData);
BOOL MPQ_FindClose(MPQ_FIND_HANDLE hFind);

/* Macro definitions for transparent API */
#define SFileOpenArchive MPQ_OpenArchive
#define SFileCloseArchive MPQ_CloseArchive
#define SFileOpenFileEx MPQ_OpenFileEx
#define SFileCloseFile MPQ_CloseFile
#define SFileReadFile MPQ_ReadFile
#define SFileGetFileSize MPQ_GetFileSize
#define SFileSetFilePointer MPQ_SetFilePointer
#define SFileExtractFile MPQ_ExtractFile
#define SFileFindFirstFile MPQ_FindFirstFile
#define SFileFindNextFile MPQ_FindNextFile
#define SFileFindClose MPQ_FindClose

/* Type aliases */
#define SFILE_FIND_DATA MPQ_FIND_DATA

#else /* USE_STORMLIB */

/* StormLib is used as-is, no wrapper needed */
#define SFILE_FIND_DATA _SFILE_FIND_DATA

#endif /* USE_LIBMPQ */

#endif /* __MPQ_ADAPTER_H__ */
