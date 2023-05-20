#include <stdio.h>
#include <StormLib.h>

#include "../common/common.h"

#define MAX_SHEET_COLUMNS 256

static DWORD SheetParseTokens(TCHAR *buffer) {
    DWORD numtokens = 1;
    for (TCHAR *it = buffer; *it != '\0'; it++) {
        if (*it == ';') {
            numtokens++;
            *it = '\0';
        }
    }
    return numtokens;
}

static TCHAR *GetToken(TCHAR *buffer, DWORD index) {
    while (index > 0) {
        buffer += strlen(buffer) + 1;
        index--;
    }
    return buffer;
}

static struct SheetCell *SheetCellNew(DWORD x, DWORD y, LPSTR text, struct SheetCell *sheet) {
    struct SheetCell *cell = MemAlloc(sizeof(struct SheetCell));
    cell->column = x;
    cell->row = y;
    cell->lpNext = sheet;
    if (*text == '"') {
        cell->text = strdup(text+1);
        cell->text[strlen(cell->text)-2] = 0;
    } else {
        cell->text = strdup(text);
    }
//    if (y == 1) {
    if (strstr(cell->text, "DPEO")) {
        printf("  %d %s\n", x, cell->text);
    }
    return cell;
}

struct SheetCell *FS_ReadSheet(LPCSTR szFileName) {
    HANDLE hFile = FS_OpenFile(szFileName);
    DWORD dwFileSize = SFileGetFileSize(hFile, NULL);
    TCHAR czBuffer[MAX_SHEET_LINE];
    TCHAR ch = 0;
    DWORD X = 1, Y = 1;
    struct SheetCell *cells = NULL;
    for (DWORD read = 0, cur = 0; read < dwFileSize; read++) {
        SFileReadFile(hFile, &ch, 1, NULL, NULL);
        if (ch == '\n') {
            DWORD const dwNumTokens = SheetParseTokens(czBuffer);
            if (czBuffer[0] == 'C' || czBuffer[0] == 'F') {
                for (DWORD i = 1; i < dwNumTokens; i++) {
                    TCHAR *token = GetToken(czBuffer, i);
                    switch (*token) {
                        case 'X':
                            X = atoi(token+1);
                            break;
                        case 'Y':
                            Y = atoi(token+1);
                            break;
                        case 'K':
                            cells = SheetCellNew(X, Y, token+1, cells);
                            break;
                    }
                }
            }
            
            memset(czBuffer, 0, MAX_SHEET_LINE);
            cur = 0;
        } else {
            czBuffer[cur++] = ch;
        }
    }
    SFileCloseFile(hFile);
    return cells;
}

void Sheet_Release(struct SheetCell *lpSheet) {
    MemFree(lpSheet->text);
    SAFE_DELETE(lpSheet->lpNext, Sheet_Release);
}

HANDLE FS_ParseSheet(LPCSTR szFileName,
                    LPCSHEETLAYOUT lpLayout,
                    DWORD dwElementSize,
                    HANDLE lpNextFieldOffset)
{
    struct SheetCell *lpSheet = FS_ReadSheet(szFileName);
    if (!lpSheet)
        return NULL;
    LPCSTR columns[MAX_SHEET_COLUMNS] = { 0 };
    HANDLE lpList = NULL;
    
    FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
        if (lpCell->row != 1 || lpCell->column >= MAX_SHEET_COLUMNS)
            continue;
        columns[lpCell->column] = lpCell->text;
    }

    for (int row = 2;; row++) {
        LPSTR lpCurrent = MemAlloc(dwElementSize);
        int filled = 0;
        FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
            if (lpCell->row != row)
                continue;
            filled = 1;
            for (LPCSHEETLAYOUT sl = lpLayout; sl->column; sl++) {
                if (!strcmp(sl->column, columns[lpCell->column])) {
                    HANDLE field = lpCurrent + (uint64_t)sl->fofs;
                    switch (sl->type) {
                        case ST_ID: *(int *)field = *(int*)lpCell->text; break;
                        case ST_INT: *(int *)field = atoi(lpCell->text); break;
                        case ST_FLOAT: *(float *)field = atof(lpCell->text); break;
                        case ST_STRING: strcpy(field, lpCell->text); break;
                    }
                }
            }
        }
        if (filled) {
            HANDLE *pnext = (HANDLE *)(lpCurrent + (uint64_t)lpNextFieldOffset);
            *pnext = lpList;
            lpList = lpCurrent;
        } else {
            MemFree(lpCurrent);
            break;
        }
    }
    Sheet_Release(lpSheet);
    return lpList;
}
