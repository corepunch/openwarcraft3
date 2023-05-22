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

static DWORD Sheet_GetHeight(LPSHEETCELL sheet) {
    DWORD height = 0;
    FOR_EACH_LIST(struct SheetCell const, cell, sheet) {
        height = MAX(height, cell->row);
    }
    return height;
}

static DWORD Sheet_GetWidth(LPSHEETCELL sheet) {
    DWORD width = 0;
    FOR_EACH_LIST(struct SheetCell const, cell, sheet) {
        width = MAX(width, cell->column);
    }
    return width;
}
static LPSHEETCELL SheetCellNew(DWORD x, DWORD y, LPSTR text, LPSHEETCELL sheet) {
    LPSHEETCELL cell = MemAlloc(sizeof(struct SheetCell));
    cell->column = x;
    cell->row = y;
    cell->next = sheet;
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

LPSHEETCELL FS_ReadSheet(LPCSTR fileName) {
    HANDLE file = FS_OpenFile(fileName);
    DWORD fileSize = SFileGetFileSize(file, NULL);
    TCHAR czBuffer[MAX_SHEET_LINE];
    TCHAR ch = 0;
    DWORD X = 1, Y = 1;
    LPSHEETCELL cells = NULL;
    for (DWORD read = 0, cur = 0; read < fileSize; read++) {
        SFileReadFile(file, &ch, 1, NULL, NULL);
        if (ch == '\n') {
            DWORD const numTokens = SheetParseTokens(czBuffer);
            if (czBuffer[0] == 'C' || czBuffer[0] == 'F') {
                for (DWORD i = 1; i < numTokens; i++) {
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
    SFileCloseFile(file);
    return cells;
}

void Sheet_Release(LPSHEETCELL sheet) {
    MemFree(sheet->text);
    SAFE_DELETE(sheet->next, Sheet_Release);
}

HANDLE FS_ParseSheet(LPCSTR fileName,
                     LPCSHEETLAYOUT layout,
                     DWORD elementSize)
{
    LPSHEETCELL sheet = FS_ReadSheet(fileName);

    if (!sheet)
        return NULL;

    LPCSTR columns[MAX_SHEET_COLUMNS] = { 0 };
    DWORD sheetHeight = Sheet_GetHeight(sheet);
    HANDLE datas = MemAlloc(elementSize * (sheetHeight + 1));

    FOR_EACH_LIST(struct SheetCell const, cell, sheet) {
        if (cell->row != 1 || cell->column >= MAX_SHEET_COLUMNS)
            continue;
        columns[cell->column] = cell->text;
    }

    for (DWORD row = 2; row <= sheetHeight; row++) {
        LPSTR current = &((LPSTR)datas)[elementSize * (row - 2)];
        FOR_EACH_LIST(struct SheetCell const, cell, sheet) {
            if (cell->row != row)
                continue;
            for (LPCSHEETLAYOUT sl = layout; sl->column; sl++) {
                if (columns[cell->column] == NULL)
                    continue;
                if (!strcmp(sl->column, columns[cell->column])) {
                    HANDLE field = current + (uint64_t)sl->fofs;
                    switch (sl->type) {
                        case ST_ID: *(int *)field = *(int*)cell->text; break;
                        case ST_INT: *(int *)field = atoi(cell->text); break;
                        case ST_FLOAT: *(float *)field = atof(cell->text); break;
                        case ST_STRING: strcpy(field, cell->text); break;
                    }
                }
            }
        }
    }
    Sheet_Release(sheet);
    return datas;
}

void GetSheetName(LPCSTR fileName, LPSTR tableName) {
    DWORD index = 0;
    for (char *name = strrchr(fileName, '\\') + 1; *name && *name != '.'; name++) {
        tableName[index++] = *name;
    }
    tableName[0] = toupper(tableName[0]);
}

void PrintSheetStruct(LPCSTR fileName) {
    LPSHEETCELL sheet = FS_ReadSheet(fileName);

    if (!sheet)
        return;

    PATHSTR tableName = { 0 };
    PATHSTR tableFolder = { 0 };
    GetSheetName(fileName, tableName);
    LPCSTR folder = "game";

    memcpy(tableFolder, fileName, strstr(fileName, "\\") - fileName);

    if (strstr(fileName, "UI\\SoundInfo\\")) {
        strcpy(tableFolder, "SoundInfo");
    }

    if (strstr(fileName, "TerrainArt\\")) {
        folder = "renderer";
    }

    PATHSTR buf;

    sprintf(buf, "/Users/igor/Developer/openwarcraft3/src/%s/%s", folder, tableFolder);
    Sys_MkDir(buf);

    sprintf(buf, "/Users/igor/Developer/openwarcraft3/src/%s/%s/%s.h", folder, tableFolder, tableName);
    FILE *hHeader = fopen(buf, "w");

    sprintf(buf, "/Users/igor/Developer/openwarcraft3/src/%s/%s/%s.c", folder, tableFolder, tableName);
    FILE *hSource = fopen(buf, "w");

    PATHSTR upperStr = { 0 };
    for (int i = 0; i < strlen(tableName); i++) {
        upperStr[i] = toupper(tableName[i]);
    }

    fprintf(hHeader, "#ifndef __%s_h__\n", tableName);
    fprintf(hHeader, "#define __%s_h__\n\n", tableName);
    fprintf(hHeader, "#include \"../../common/common.h\"\n\n");
    fprintf(hHeader, "typedef char SHEETSTR[80];\n\n");
    fprintf(hHeader, "struct %s {\n", tableName);

    fprintf(hSource, "#include \"../%c_local.h\"\n", folder[0]);
    fprintf(hSource, "#include \"%s.h\"\n\n", tableName);
    fprintf(hSource, "static LP%s g_%s = NULL;\n\n", upperStr, tableName);
    fprintf(hSource, "static struct SheetLayout %s[] = {\n", tableName);

    PATHSTR indexName;
    enum SheetType indexType = ST_INT;

    for (int column = 1; column <= Sheet_GetWidth(sheet); column++) {
        LPCSTR columnName = NULL;
        typedef struct {
            bool numbers;
            bool letters;
            bool num4;
            bool comma;
        } status_t;
        status_t status = { true, true, true, false };
        FOR_EACH_LIST(struct SheetCell const, cell, sheet) {
            if (cell->column != column)
                continue;
            if (cell->row == 1) {
                columnName = cell->text;
            } else {
                for (LPCSTR s = cell->text; *s; s++) {
                    status.numbers &= isdigit(*s) || *s == ',' || *s == '-' || *s == '_' || *s == ' ' || *s == '.';
                    status.comma |= *s == ',' || *s == '.';
                }
                status.num4 &= strlen(cell->text) <= 4;
            }
        }
        if (columnName == NULL)
            continue;
        PATHSTR varName = { 0 };
        for (int i = 0, j = 0; columnName[i]; i++) {
            if (columnName[i] == '(')
                break;
            if (columnName[i] == ' ')
                varName[j++] = '_';
            else
                varName[j++] = columnName[i];
        }
        if (!strcmp(varName, "auto") || !strcmp(varName, "long")) {
            varName[strlen(varName)] = '_';
        }
        if (status.numbers) {
            fprintf(hHeader, "\t%s %s; /*", status.comma ? "float" : "DWORD", varName);
            fprintf(hSource, "\t{ \"%s\", ST_%s, FOFS(%s, %s) },\n", columnName, status.comma ? "FLOAT" : "INT", tableName, varName);
            if (column == 1) {
                strcpy(indexName, varName);
                indexType = ST_INT;
            }
        } else if (status.num4) {
            fprintf(hHeader, "\tDWORD %s; /*", varName);
            fprintf(hSource, "\t{ \"%s\", ST_ID, FOFS(%s, %s) },\n", columnName, tableName, varName);
            if (column == 1) {
                strcpy(indexName, varName);
                indexType = ST_INT;
            }
        } else {
            fprintf(hHeader, "\tSHEETSTR %s; /*", varName);
            fprintf(hSource, "\t{ \"%s\", ST_STRING, FOFS(%s, %s) },\n", columnName, tableName, varName);
            if (column == 1) {
                strcpy(indexName, varName);
                indexType = ST_STRING;
            }
        }
        FOR_EACH_LIST(struct SheetCell const, cell, sheet) {
            if (cell->column != column)
                continue;
            if (cell->row > 1 && cell->row < 10) {
                fprintf(hHeader, " %s", cell->text);
            }
        }
        fprintf(hHeader, " */\n");
    }
    fprintf(hSource, "\t{ NULL },\n");
    fprintf(hSource, "};\n\n");
    fprintf(hHeader, "};\n\n");

    fprintf(hHeader, "typedef struct %s %s;\n", tableName, upperStr);
    fprintf(hHeader, "typedef struct %s *LP%s;\n", tableName, upperStr);
    fprintf(hHeader, "typedef struct %s const *LPC%s;\n\n", tableName, upperStr);

    fprintf(hSource, "LPC%s Find%s(%s %s) {\n", upperStr, tableName, (indexType == ST_INT ? "DWORD" : "LPCSTR"), indexName);
    fprintf(hSource, "\tstruct %s *value = g_%s;\n", tableName, tableName);
    if (indexType == ST_INT) {
        fprintf(hSource, "\tfor (; value->%s != %s && value->%s; value++);\n", indexName, indexName, indexName);
        fprintf(hSource, "\tif (value->%s == 0) value = NULL;\n", indexName);
    } else {
        fprintf(hSource, "\tfor (; *value->%s && strcmp(value->%s, %s); value++);\n", indexName, indexName, indexName);
        fprintf(hSource, "\tif (*value->%s == 0) value = NULL;\n", indexName);
    }
    fprintf(hSource, "\treturn value;\n");
    fprintf(hSource, "}\n\n");

    fprintf(hSource, "void Init%s(void) {\n", tableName);
    fprintf(hSource, "\tg_%s = %ci.ParseSheet(\"", tableName, folder[0]);
    for (LPCSTR s = fileName; *s; s++) {
        fprintf(hSource, "%c", *s);
        if (*s == '\\')
            fprintf(hSource, "%c", *s);
    }
    fprintf(hSource, "\", %s, sizeof(struct %s));\n", tableName, tableName);
    fprintf(hSource, "}\n");

    fprintf(hSource, "void Shutdown%s(void) {\n", tableName);
    fprintf(hSource, "\t%ci.MemFree(g_%s);\n", folder[0], tableName);
    fprintf(hSource, "}\n");

    fprintf(hHeader, "LPC%s Find%s(%s %s);\n", upperStr, tableName, (indexType == ST_INT ? "DWORD" : "LPCSTR"), indexName);
    fprintf(hHeader, "void Init%s(void);\n", tableName);
    fprintf(hHeader, "void Shutdown%s(void);\n\n", tableName);

    fprintf(hHeader, "#endif\n");

    fclose(hHeader);
    fclose(hSource);
}

void FPrintSheetStructs(LPCSTR *fileNames) {
    for (LPCSTR* s = fileNames; *s; s++) {
        PrintSheetStruct(*s);
    }
}
