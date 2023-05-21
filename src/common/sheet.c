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

static DWORD Sheet_GetHeight(LPSHEETCELL lpSheet) {
    DWORD dwHeight = 0;
    FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
        dwHeight = MAX(dwHeight, lpCell->row);
    }
    return dwHeight;
}

static DWORD Sheet_GetWidth(LPSHEETCELL lpSheet) {
    DWORD dwWidth = 0;
    FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
        dwWidth = MAX(dwWidth, lpCell->column);
    }
    return dwWidth;
}
static LPSHEETCELL SheetCellNew(DWORD x, DWORD y, LPSTR text, LPSHEETCELL sheet) {
    LPSHEETCELL cell = MemAlloc(sizeof(struct SheetCell));
    cell->column = x;
    cell->row = y;
    cell->lpNext = sheet;
    cell->text = MemAlloc(strlen(text + 1));
    for (char *instr = text, *outstr = cell->text; *instr; instr++) {
        if (*instr == '"' || *instr == '\r' || *instr == '\n')
            continue;
        *(outstr++) = *instr;
    }
//    if (y == 2) {
//    if (!strcmp(cell->text, "Peon")) {
//        printf("\t%d %s\n", x, cell->text);
//    }
    return cell;
}

LPSHEETCELL FS_ReadSheet(LPCSTR szFileName) {
    HANDLE hFile = FS_OpenFile(szFileName);
    DWORD dwFileSize = SFileGetFileSize(hFile, NULL);
    TCHAR czBuffer[MAX_SHEET_LINE];
    TCHAR ch = 0;
    DWORD X = 1, Y = 1;
    LPSHEETCELL cells = NULL;
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

void Sheet_Release(LPSHEETCELL lpSheet) {
    MemFree(lpSheet->text);
    SAFE_DELETE(lpSheet->lpNext, Sheet_Release);
}

HANDLE FS_ParseSheet(LPCSTR szFileName,
                     LPCSHEETLAYOUT lpLayout,
                     DWORD dwElementSize)
{
    LPSHEETCELL lpSheet = FS_ReadSheet(szFileName);

    if (!lpSheet)
        return NULL;

    LPCSTR columns[MAX_SHEET_COLUMNS] = { 0 };
    DWORD dwSheetHeight = Sheet_GetHeight(lpSheet);
    HANDLE lpDatas = MemAlloc(dwElementSize * (dwSheetHeight + 1));

    FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
        if (lpCell->row != 1 || lpCell->column >= MAX_SHEET_COLUMNS)
            continue;
        columns[lpCell->column] = lpCell->text;
    }

    for (DWORD dwRow = 2; dwRow <= dwSheetHeight; dwRow++) {
        LPSTR lpCurrent = &((LPSTR)lpDatas)[dwElementSize * (dwRow - 2)];
        FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
            if (lpCell->row != dwRow)
                continue;
            for (LPCSHEETLAYOUT sl = lpLayout; sl->column; sl++) {
                if (columns[lpCell->column] == NULL)
                    continue;
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
    }
    Sheet_Release(lpSheet);
    return lpDatas;
}

void GetSheetName(LPCSTR szFileName, LPSTR szTableName) {
    DWORD index = 0;
    for (char *name = strrchr(szFileName, '\\') + 1; *name && *name != '.'; name++) {
        szTableName[index++] = *name;
    }
    szTableName[0] = toupper(szTableName[0]);
}

void PrintSheetStruct(LPCSTR szFileName) {
    LPSHEETCELL lpSheet = FS_ReadSheet(szFileName);

    if (!lpSheet)
        return;
    
    PATHSTR szTableName = { 0 };
    PATHSTR szTableFolder = { 0 };
    GetSheetName(szFileName, szTableName);
    LPCSTR szFolder = "game";

    memcpy(szTableFolder, szFileName, strstr(szFileName, "\\") - szFileName);

    if (strstr(szFileName, "UI\\SoundInfo\\")) {
        strcpy(szTableFolder, "SoundInfo");
    }

    if (strstr(szFileName, "TerrainArt\\")) {
        szFolder = "renderer";
    }
    
    PATHSTR buf;
    
    sprintf(buf, "/Users/igor/Developer/openwarcraft3/src/%s/%s", szFolder, szTableFolder);
    Sys_MkDir(buf);

    sprintf(buf, "/Users/igor/Developer/openwarcraft3/src/%s/%s/%s.h", szFolder, szTableFolder, szTableName);
    FILE *hHeader = fopen(buf, "w");

    sprintf(buf, "/Users/igor/Developer/openwarcraft3/src/%s/%s/%s.c", szFolder, szTableFolder, szTableName);
    FILE *hSource = fopen(buf, "w");
    
    fprintf(hHeader, "#ifndef __%s_h__\n", szTableName);
    fprintf(hHeader, "#define __%s_h__\n\n", szTableName);
    fprintf(hHeader, "#include \"../../common/common.h\"\n\n");
    fprintf(hHeader, "typedef char SHEETSTR[80];\n\n");
    fprintf(hHeader, "struct %s {\n", szTableName);

    fprintf(hSource, "#include \"../%c_local.h\"\n", szFolder[0]);
    fprintf(hSource, "#include \"%s.h\"\n\n", szTableName);
    fprintf(hSource, "static struct %s *g_%s = NULL;\n\n", szTableName, szTableName);
    fprintf(hSource, "static struct SheetLayout %s[] = {\n", szTableName);

    PATHSTR indexName;
    enum SheetType indexType = ST_INT;
    
    for (int dwColumn = 1; dwColumn <= Sheet_GetWidth(lpSheet); dwColumn++) {
        LPCSTR szColumnName = NULL;
        typedef struct {
            bool numbers;
            bool letters;
            bool num4;
            bool comma;
        } status_t;
        status_t status = { true, true, true, false };
        FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
            if (lpCell->column != dwColumn)
                continue;
            if (lpCell->row == 1) {
                szColumnName = lpCell->text;
            } else {
                for (LPCSTR s = lpCell->text; *s; s++) {
                    status.numbers &= isdigit(*s) || *s == ',' || *s == '-' || *s == '_' || *s == ' ' || *s == '.';
                    status.comma |= *s == ',' || *s == '.';
                }
                status.num4 &= strlen(lpCell->text) <= 4;
            }
        }
        if (szColumnName == NULL)
            continue;
        PATHSTR szVarName = { 0 };
        for (int i = 0, j = 0; szColumnName[i]; i++) {
            if (szColumnName[i] == '(')
                break;
            if (szColumnName[i] == ' ')
                szVarName[j++] = '_';
            else
                szVarName[j++] = szColumnName[i];
        }
        if (!strcmp(szVarName, "auto") || !strcmp(szVarName, "long")) {
            szVarName[strlen(szVarName)] = '_';
        }
        if (status.numbers) {
            fprintf(hHeader, "\t%s %s; /*", status.comma ? "float" : "DWORD", szVarName);
            fprintf(hSource, "\t{ \"%s\", ST_%s, FOFS(%s, %s) },\n", szColumnName, status.comma ? "FLOAT" : "INT", szTableName, szVarName);
            if (dwColumn == 1) {
                strcpy(indexName, szVarName);
                indexType = ST_INT;
            }
        } else if (status.num4) {
            fprintf(hHeader, "\tDWORD %s; /*", szVarName);
            fprintf(hSource, "\t{ \"%s\", ST_ID, FOFS(%s, %s) },\n", szColumnName, szTableName, szVarName);
            if (dwColumn == 1) {
                strcpy(indexName, szVarName);
                indexType = ST_INT;
            }
        } else {
            fprintf(hHeader, "\tSHEETSTR %s; /*", szVarName);
            fprintf(hSource, "\t{ \"%s\", ST_STRING, FOFS(%s, %s) },\n", szColumnName, szTableName, szVarName);
            if (dwColumn == 1) {
                strcpy(indexName, szVarName);
                indexType = ST_STRING;
            }
        }
        FOR_EACH_LIST(struct SheetCell const, lpCell, lpSheet) {
            if (lpCell->column != dwColumn)
                continue;
            if (lpCell->row > 1 && lpCell->row < 10) {
                fprintf(hHeader, " %s", lpCell->text);
            }
        }
        fprintf(hHeader, " */\n");
    }
    fprintf(hSource, "\t{ NULL },\n");
    fprintf(hSource, "};\n\n");
    fprintf(hHeader, "};\n\n");
    
    fprintf(hSource, "struct %s *Find%s(%s %s) {\n", szTableName, szTableName, (indexType == ST_INT ? "DWORD" : "LPCSTR"), indexName);
    fprintf(hSource, "\tstruct %s *lpValue = g_%s;\n", szTableName, szTableName);
    if (indexType == ST_INT) {
        fprintf(hSource, "\tfor (; lpValue->%s != %s && lpValue->%s; lpValue++);\n", indexName, indexName, indexName);
        fprintf(hSource, "\tif (lpValue->%s == 0) lpValue = NULL;\n", indexName);
    } else {
        fprintf(hSource, "\tfor (; *lpValue->%s && strcmp(lpValue->%s, %s); lpValue++);\n", indexName, indexName, indexName);
        fprintf(hSource, "\tif (*lpValue->%s == 0) lpValue = NULL;\n", indexName);
    }
    fprintf(hSource, "\treturn lpValue;\n");
    fprintf(hSource, "}\n\n");

    fprintf(hSource, "void Init%s(void) {\n", szTableName);
    fprintf(hSource, "\tg_%s = %ci.ParseSheet(\"", szTableName, szFolder[0]);
    for (LPCSTR s = szFileName; *s; s++) {
        fprintf(hSource, "%c", *s);
        if (*s == '\\')
            fprintf(hSource, "%c", *s);
    }
    fprintf(hSource, "\", %s, sizeof(struct %s));\n", szTableName, szTableName);
    fprintf(hSource, "}\n");

    fprintf(hSource, "void Shutdown%s(void) {\n", szTableName);
    fprintf(hSource, "\t%ci.MemFree(g_%s);\n", szFolder[0], szTableName);
    fprintf(hSource, "}\n");

    fprintf(hHeader, "struct %s *Find%s(%s %s);\n", szTableName, szTableName, (indexType == ST_INT ? "DWORD" : "LPCSTR"), indexName);
    fprintf(hHeader, "void Init%s(void);\n", szTableName);
    fprintf(hHeader, "void Shutdown%s(void);\n\n", szTableName);

    fprintf(hHeader, "#endif\n");
    
    fclose(hHeader);
    fclose(hSource);
}

void FPrintSheetStructs(LPCSTR *szFileNames) {
    for (LPCSTR* s = szFileNames; *s; s++) {
        PrintSheetStruct(*s);
    }
}
