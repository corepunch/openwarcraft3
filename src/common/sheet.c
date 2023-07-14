#include <stdio.h>
#include <StormLib.h>

#include "../common/common.h"

#define MAX_INI_LINE 1024
#define MAX_SHEET_COLUMNS 256

typedef struct SheetCell {
    LPSTR text;
    USHORT column;
    USHORT row;
    LPSHEET next;
} sheetCell_t;

static sheetCell_t cells[256 * 1024] = { 0 };
static sheetRow_t rows[32 * 1024] = { 0 };
static sheetField_t fields[256 * 1024] = { 0 };
static char text_buffer[8 * 1024 * 1024] = { 0 };
LPSTR current_text = text_buffer;
LPSHEET current_cell = cells;
LPSHEET previous_cell = cells;

sheetRow_t *current_row = rows;
sheetField_t *current_field = fields;

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

//int text_size = 0;

static void FS_FillSheetCell(DWORD x, DWORD y, LPSTR text) {
    current_cell->column = x;
    current_cell->row = y;
    current_cell->next = current_cell + 1;
    current_cell->text = current_text;
    for (char *instr = text; *instr; instr++) {
        if (*instr == '"' || *instr == '\r' || *instr == '\n')
            continue;
        *(current_text++) = *instr;
    }
    *(current_text++) = '\0';
    previous_cell = current_cell;
    current_cell++;
//    if (y == 2) {
//    if (strstr(cell->text, "hhou")) {
//        printf("\t%d %s\n", x, cell->text);
//    }
}

sheetRow_t *FS_MakeRowsFromSheet(LPSHEET sheet) {
    LPCSTR columns[256] = { 0 };
    DWORD num_rows = 0;
    sheetRow_t *start = current_row;
    FOR_EACH_LIST(SHEET, cell, sheet) {
        if (cell->row == 1) {
            columns[cell->column] = cell->text;
        }
        num_rows = MAX(num_rows, cell->row);
    }
    sheetRow_t *previous_row = NULL;
    FOR_LOOP(row, num_rows) {
        FOR_EACH_LIST(SHEET, cell, sheet) {
            if (cell->row != row + 1)
                continue;
            if (cell->column == 1) {
                current_row->name = cell->text;
            } else if (columns[cell->column]) {
                current_field->name = columns[cell->column];
                current_field->value = cell->text;
                ADD_TO_LIST(current_field, current_row->fields);
                current_field++;
            }
        }
        if (current_row->name) {
            previous_row = current_row;
            previous_row->next = ++current_row;
        }
    }
    if (previous_row) {
        previous_row->next = NULL;
        return start;
    } else {
        return start;
    }
}


sheetRow_t *FS_ParseSLK(LPCSTR fileName) {
    HANDLE file = FS_OpenFile(fileName);
    DWORD fileSize = SFileGetFileSize(file, NULL);
    TCHAR czBuffer[MAX_SHEET_LINE];
    TCHAR ch = 0;
    DWORD X = 1, Y = 1;
    LPSHEET start = current_cell;
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
                            FS_FillSheetCell(X, Y, token+1);
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
    if (start != current_cell) { // close the table
        previous_cell->next = NULL;
        return FS_MakeRowsFromSheet(start);
    } else {
        return NULL;
    }
}

LPCSTR FS_FindSheetCell(sheetRow_t *sheet, LPCSTR row, LPCSTR column) {
    FOR_EACH_LIST(sheetRow_t const, srow, sheet) {
        if (strcmp(srow->name, row))
            continue;
        FOR_EACH_LIST(sheetField_t const, scolumn, srow->fields) {
            if (strcasecmp(scolumn->name, column))
                continue;
            return scolumn->value;
        }
    }
    return NULL;
}

static sheetRow_t *FS_ParseINI_Buffer(LPCSTR buffer) {
    LPCSTR p = buffer;
    sheetRow_t *start = current_row;
    sheetRow_t *section = NULL;
    while (true) {
        while (*p && isspace(*p)) p++;
        if (!*p)
            break;
        if (p[0] == '/' && p[1] == '/') {
            for (; *p != '\n' && *p != '\0'; p++);
        } else if (*p == '[') {
            p++;
            if (section) {
                section->next = current_row;
            }
            section = current_row++;
            section->name = current_text;
            for (; *p != ']'; current_text++, p++) {
                *current_text = *p;
            }
            *(current_text++) = '\0';
            p++;
        } else {
            char line[MAX_INI_LINE] = { 0 };
            for (LPSTR w = line; *p != '\n' && *p != '\r' && *p != '\0'; p++, w++) {
                *w = *p;
            }
            LPSTR eq = strstr(line, "=");
            if (eq && section) {
                *eq = '\0';
                for (LPSTR s = eq; s > line; s--) {
                    if (*s == ' ') *s = '\0';
                    else break;
                }
                eq++;
                for (; *eq == ' '; eq++);
//                printf("%s.%s %s\n", currentSec, line, eq);
                sheetField_t *field = current_field++;
                field->name = current_text;
                strcpy(current_text, line);
                current_text += strlen(line) + 1;
                field->value = current_text;
                strcpy(current_text, eq);
                current_text += strlen(eq) + 1;
                ADD_TO_LIST(field, section->fields);
            }
        }
    }
    return current_row != start ? start : NULL;
}

sheetRow_t *FS_ParseINI(LPCSTR fileName) {
    LPSTR buffer = FS_ReadFileIntoString(fileName);
    if (!buffer) {
        return NULL;
    }
//    printf("%")
    sheetRow_t *config = FS_ParseINI_Buffer(buffer);
    if (!config) {
        fprintf(stderr, "Failed to parse %s\n", fileName);
    }
    MemFree(buffer);
    return config;
}
