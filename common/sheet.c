#include <stdio.h>
#include <StormLib.h>

#include "../common/common.h"

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

static struct SheetCell *SheetCellNew(int x, int y, char *text, struct SheetCell *sheet) {
    struct SheetCell *cell = MemAlloc(sizeof(struct SheetCell));
    cell->x = x;
    cell->y = y;
    cell->lpNext = sheet;
    if (*text == '"') {
        cell->text = strdup(text+1);
        cell->text[strlen(cell->text)-2] = 0;
    } else {
        cell->text = strdup(text);
    }
//    if (y==163) printf("%s\n", text);
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
