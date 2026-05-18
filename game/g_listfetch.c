#include "g_local.h"

#include <strings.h>

static BOOL G_IsMapFile(LPCSTR name) {
    size_t len;

    if (!name) {
        return false;
    }
    len = strlen(name);
    if (len < 4) {
        return false;
    }
    return !strcasecmp(name + len - 4, ".w3m") ||
           !strcasecmp(name + len - 4, ".w3x");
}

static void G_ListAppendRow(LPSTR text, DWORD text_size, DWORD *numRows, LPCSTR row) {
    DWORD len;
    DWORD add;

    if (!text || text_size == 0 || !numRows || !row || *numRows >= MAX_LIST_FETCH_ROWS) {
        return;
    }
    len = (DWORD)strlen(text);
    add = (DWORD)strlen(row);
    if (len && len + 1 < text_size) {
        text[len++] = '\n';
        text[len] = '\0';
    }
    if (len + add >= text_size) {
        add = text_size - len - 1;
    }
    if (add) {
        memcpy(text + len, row, add);
    }
    text[len + add] = '\0';
    (*numRows)++;
}

static void G_ListMaps(LPSTR text, DWORD text_size) {
    SFILE_FIND_DATA find;
    HANDLE handle;
    char keys[MAX_LIST_FETCH_ROWS][64];
    DWORD numRows = 0;

    if (!text || text_size == 0) {
        return;
    }

    text[0] = '\0';
    memset(keys, 0, sizeof(keys));
    handle = gi.FindFirstFile ? gi.FindFirstFile("Maps\\*", &find) : NULL;
    while (handle && numRows < MAX_LIST_FETCH_ROWS) {
        if (G_IsMapFile(find.cFileName)) {
            BOOL duplicate = false;

            FOR_LOOP(i, numRows) {
                if (!strcmp(keys[i], find.cFileName)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                snprintf(keys[numRows], sizeof(keys[0]), "%s", find.cFileName);
                G_ListAppendRow(text, text_size, &numRows, find.cFileName);
            }
        }
        if (!gi.FindNextFile || !gi.FindNextFile(handle, &find)) {
            break;
        }
    }
    if (handle && gi.FindClose) {
        gi.FindClose(handle);
    }
}

static void G_WriteListFetchResponse(LPEDICT ent, DWORD requestId, LPCSTR text) {
    gi.WriteByte(svc_list);
    gi.WriteLong((LONG)requestId);
    gi.WriteString(text ? text : "");
    gi.unicast(ent);
}

void CMD_List(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    DWORD requestId;
    char text[MAX_LIST_FETCH_TEXT];

    if (argc < 3) {
        return;
    }

    requestId = (DWORD)strtoul(argv[1], NULL, 10);
    text[0] = '\0';
    if (argc >= 3 && !strcmp(argv[2], "maps")) {
        G_ListMaps(text, sizeof(text));
    }
    G_WriteListFetchResponse(ent, requestId, text);
}
