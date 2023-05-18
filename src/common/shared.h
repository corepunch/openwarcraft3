#ifndef shared_h
#define shared_h

#include <string.h>
#include <stdio.h>
#include <assert.h>

// Typedefs for ANSI C
typedef unsigned char  BYTE;
typedef unsigned short USHORT;
typedef int            LONG;
typedef unsigned int   DWORD;
typedef unsigned long  DWORD_PTR;
typedef long           LONG_PTR;
typedef long           INT_PTR;
typedef long long      LONGLONG;
typedef unsigned long long ULONGLONG;
typedef void         * HANDLE;
typedef void         * LPOVERLAPPED; // Unsupported on Linux and Mac
typedef char           TCHAR;
typedef unsigned int   LCID;
typedef LONG         * PLONG;
typedef DWORD        * LPDWORD;
typedef BYTE         * LPBYTE;
typedef const char   * LPCTSTR;
typedef const char   * LPCSTR;
typedef char         * LPTSTR;
typedef char         * LPSTR;

#ifndef __cplusplus
  #define bool char
  #define true 1
  #define false 0
#endif

//#define NULL 0

#endif
