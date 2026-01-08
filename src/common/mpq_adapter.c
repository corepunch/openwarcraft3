#include "mpq_adapter.h"

#ifdef USE_LIBMPQ

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Windows types compatibility - these should be defined in shared.h */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Open an MPQ archive */
BOOL MPQ_OpenArchive(LPCSTR szMpqName, DWORD dwPriority, DWORD dwFlags, MPQ_ARCHIVE* phMpq) {
    mpq_archive_s* archive = NULL;
    int result = libmpq__archive_open(&archive, (char*)szMpqName, -1);
    
    if (result != 0 || archive == NULL) {
        return FALSE;
    }
    
    *phMpq = archive;
    return TRUE;
}

/* Close an MPQ archive */
BOOL MPQ_CloseArchive(MPQ_ARCHIVE hMpq) {
    if (hMpq == NULL) {
        return FALSE;
    }
    
    int result = libmpq__archive_close(hMpq);
    return (result == 0) ? TRUE : FALSE;
}

/* Open a file within an MPQ archive */
BOOL MPQ_OpenFileEx(MPQ_ARCHIVE hMpq, LPCSTR szFileName, DWORD dwSearchScope, MPQ_FILE* phFile) {
    if (hMpq == NULL || szFileName == NULL || phFile == NULL) {
        return FALSE;
    }
    
    MPQ_FILE_HANDLE* file_handle = (MPQ_FILE_HANDLE*)malloc(sizeof(MPQ_FILE_HANDLE));
    if (file_handle == NULL) {
        return FALSE;
    }
    
    /* Get file number from filename */
    int file_number = 0;
    int result = libmpq__file_number(hMpq, (char*)szFileName, &file_number);
    
    if (result != 0) {
        free(file_handle);
        return FALSE;
    }
    
    /* Get file size */
    off_t file_size = 0;
    result = libmpq__file_unpacked_size(hMpq, file_number, &file_size);
    
    if (result != 0) {
        free(file_handle);
        return FALSE;
    }
    
    /* Allocate buffer and read entire file for efficient random access */
    unsigned char* buffer = (unsigned char*)malloc(file_size);
    if (buffer == NULL) {
        free(file_handle);
        return FALSE;
    }
    
    result = libmpq__file_read(hMpq, file_number, buffer, file_size);
    
    if (result != 0) {
        free(buffer);
        free(file_handle);
        return FALSE;
    }
    
    file_handle->archive = hMpq;
    file_handle->file_number = file_number;
    file_handle->offset = 0;
    file_handle->cached_data = buffer;
    file_handle->cached_size = file_size;
    
    *phFile = file_handle;
    return TRUE;
}

/* Close a file handle */
BOOL MPQ_CloseFile(MPQ_FILE hFile) {
    if (hFile == NULL) {
        return FALSE;
    }
    
    if (hFile->cached_data != NULL) {
        free(hFile->cached_data);
    }
    
    free(hFile);
    return TRUE;
}

/* Read data from a file */
BOOL MPQ_ReadFile(MPQ_FILE hFile, LPVOID lpBuffer, DWORD dwToRead, LPDWORD pdwRead, LPOVERLAPPED lpOverlapped) {
    if (hFile == NULL || lpBuffer == NULL) {
        return FALSE;
    }
    
    /* Check if we're trying to read beyond file size */
    if (hFile->offset >= hFile->cached_size) {
        if (pdwRead != NULL) {
            *pdwRead = 0;
        }
        return TRUE;  /* Not an error, just EOF */
    }
    
    /* Adjust read size if needed */
    off_t bytes_to_read = dwToRead;
    if (hFile->offset + bytes_to_read > hFile->cached_size) {
        bytes_to_read = hFile->cached_size - hFile->offset;
    }
    
    /* Copy from cached data */
    memcpy(lpBuffer, hFile->cached_data + hFile->offset, bytes_to_read);
    hFile->offset += bytes_to_read;
    
    if (pdwRead != NULL) {
        *pdwRead = (DWORD)bytes_to_read;
    }
    
    return TRUE;
}

/* Get file size */
DWORD MPQ_GetFileSize(MPQ_FILE hFile, LPDWORD pdwFileSizeHigh) {
    if (hFile == NULL) {
        return 0;
    }
    
    if (pdwFileSizeHigh != NULL) {
        *pdwFileSizeHigh = (DWORD)(hFile->cached_size >> 32);
    }
    
    return (DWORD)(hFile->cached_size & 0xFFFFFFFF);
}

/* Set file pointer position */
DWORD MPQ_SetFilePointer(MPQ_FILE hFile, LONG lFilePos, LONG* plFilePosHigh, DWORD dwMoveMethod) {
    if (hFile == NULL) {
        return 0xFFFFFFFF;
    }
    
    off_t new_offset = 0;
    
    switch (dwMoveMethod) {
        case FILE_BEGIN:
            new_offset = lFilePos;
            break;
        case FILE_CURRENT:
            new_offset = hFile->offset + lFilePos;
            break;
        case FILE_END:
            new_offset = hFile->cached_size + lFilePos;
            break;
        default:
            return 0xFFFFFFFF;
    }
    
    if (new_offset < 0) {
        return 0xFFFFFFFF;
    }
    
    hFile->offset = new_offset;
    
    if (plFilePosHigh != NULL) {
        *plFilePosHigh = (LONG)(new_offset >> 32);
    }
    
    return (DWORD)(new_offset & 0xFFFFFFFF);
}

/* Extract a file from the archive */
BOOL MPQ_ExtractFile(MPQ_ARCHIVE hMpq, LPCSTR szToExtract, LPCSTR szExtracted, DWORD dwSearchScope) {
    if (hMpq == NULL || szToExtract == NULL || szExtracted == NULL) {
        return FALSE;
    }
    
    int file_number = 0;
    int result = libmpq__file_number(hMpq, (char*)szToExtract, &file_number);
    
    if (result != 0) {
        return FALSE;
    }
    
    result = libmpq__file_extract(hMpq, file_number, (char*)szExtracted);
    return (result == 0) ? TRUE : FALSE;
}

/* Find first file in archive - simplified implementation */
MPQ_FIND_HANDLE MPQ_FindFirstFile(MPQ_ARCHIVE hMpq, LPCSTR szMask, LPMPQ_FIND_DATA lpFindFileData, LPCSTR szListFile) {
    /* Note: libmpq doesn't have native file enumeration like StormLib.
     * This is a simplified implementation that may not work for all cases.
     * For full functionality, you would need to read and parse the (listfile) 
     * from the MPQ archive. */
    
    if (hMpq == NULL || lpFindFileData == NULL) {
        return NULL;
    }
    
    /* Initialize find data structure */
    lpFindFileData->archive = hMpq;
    lpFindFileData->current_file = 0;
    
    /* Try to get the number of files in the archive */
    /* This is a limitation - libmpq doesn't provide easy file enumeration */
    lpFindFileData->file_count = 0;
    
    /* Return NULL to indicate file enumeration is not fully supported with libmpq */
    return NULL;
}

/* Find next file */
BOOL MPQ_FindNextFile(MPQ_FIND_HANDLE hFind, LPMPQ_FIND_DATA lpFindFileData) {
    /* Not fully implemented - libmpq limitation */
    return FALSE;
}

/* Close find handle */
BOOL MPQ_FindClose(MPQ_FIND_HANDLE hFind) {
    /* Nothing to clean up in our simplified implementation */
    return TRUE;
}

#endif /* USE_LIBMPQ */
