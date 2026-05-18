#include "g_local.h"

#ifndef _WIN32
#include <strings.h>
#endif

typedef struct {
    PATHSTR path;
    char name[128];
    char description[512];
    char suggestedPlayers[96];
    char mapSize[32];
    char tileset[64];
    DWORD players;
    DWORD flags;
} mapListItem_t;

typedef struct {
    const BYTE *data;
    DWORD size;
    DWORD pos;
} mapReader_t;

static void G_SanitizeListField(LPSTR text);
static void G_SanitizeInfoText(LPSTR text);

#define MAX_MAP_LIST_ITEMS 256

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

static BOOL G_IsCampaignMap(LPCSTR name) {
    return name && (!strncasecmp(name, "Maps\\Campaign\\", 14) ||
                    !strncasecmp(name, "Maps/FrozenThrone/Campaign/", 26) ||
                    !strncasecmp(name, "Maps\\FrozenThrone\\Campaign\\", 26) ||
                    !strncasecmp(name, "Maps/Campaign/", 14));
}

static BOOL G_MapRead(mapReader_t *reader, void *out, DWORD size) {
    if (!reader || !out || reader->pos > reader->size || size > reader->size - reader->pos) {
        return false;
    }
    memcpy(out, reader->data + reader->pos, size);
    reader->pos += size;
    return true;
}

static BOOL G_MapSkip(mapReader_t *reader, DWORD size) {
    if (!reader || reader->pos > reader->size || size > reader->size - reader->pos) {
        return false;
    }
    reader->pos += size;
    return true;
}

static BOOL G_MapReadU32(mapReader_t *reader, DWORD *out) {
    return G_MapRead(reader, out, sizeof(*out));
}

static BOOL G_MapReadCString(mapReader_t *reader, LPSTR out, DWORD out_size) {
    DWORD written = 0;

    if (!reader) {
        return false;
    }
    if (out && out_size > 0) {
        out[0] = '\0';
    }
    while (reader->pos < reader->size) {
        char ch = (char)reader->data[reader->pos++];
        if (ch == '\0') {
            return true;
        }
        if (out && out_size > 0 && written + 1 < out_size) {
            out[written++] = ch;
            out[written] = '\0';
        }
    }
    return false;
}

static LPCSTR G_SkipSpace(LPCSTR text) {
    while (text && (*text == ' ' || *text == '\t' || *text == '\r')) {
        text++;
    }
    return text;
}

static void G_ReadLine(LPCSTR *cursor, LPSTR out, DWORD out_size) {
    DWORD len = 0;
    LPCSTR p;

    if (!cursor || !*cursor || !out || out_size == 0) {
        return;
    }
    p = *cursor;
    while (*p && *p != '\n' && *p != '\r') {
        if (len + 1 < out_size) {
            out[len++] = *p;
        }
        p++;
    }
    out[len] = '\0';
    while (*p == '\n' || *p == '\r') {
        p++;
    }
    *cursor = p;
}

static BOOL G_WTSFindString(LPCSTR wts, DWORD stringId, LPSTR out, DWORD out_size) {
    LPCSTR cursor;
    DWORD currentId = 0;
    BOOL inTarget = false;
    BOOL readingData = false;
    char line[512];

    if (!wts || !out || out_size == 0) {
        return false;
    }
    out[0] = '\0';
    cursor = wts;
    if ((BYTE)cursor[0] == 0xEF && (BYTE)cursor[1] == 0xBB && (BYTE)cursor[2] == 0xBF) {
        cursor += 3;
    }

    while (*cursor) {
        LPCSTR trimmed;

        G_ReadLine(&cursor, line, sizeof(line));
        trimmed = G_SkipSpace(line);
        if (!readingData) {
            if (!strncmp(trimmed, "STRING ", 7)) {
                currentId = (DWORD)strtoul(trimmed + 7, NULL, 10);
                inTarget = (currentId == stringId);
            } else if (*trimmed == '{') {
                readingData = true;
            }
            continue;
        }
        if (*trimmed == '}') {
            if (inTarget) {
                return true;
            }
            inTarget = false;
            readingData = false;
            out[0] = '\0';
            continue;
        }
        if (inTarget) {
            DWORD len = (DWORD)strlen(out);
            DWORD add = (DWORD)strlen(line);
            if (len + add >= out_size) {
                add = out_size - len - 1;
            }
            if (add) {
                memcpy(out + len, line, add);
                out[len + add] = '\0';
            }
        }
    }
    return false;
}

static void G_ResolveTrigStr(LPCSTR text, LPCSTR wts, LPSTR out, DWORD out_size) {
    DWORD id;

    if (!out || out_size == 0) {
        return;
    }
    if (!text) {
        out[0] = '\0';
        return;
    }
    if (strncmp(text, "TRIGSTR_", 8)) {
        snprintf(out, out_size, "%s", text);
        return;
    }
    id = (DWORD)strtoul(text + 8, NULL, 10);
    if (!G_WTSFindString(wts, id, out, out_size)) {
        snprintf(out, out_size, "%s", text);
    }
}

static LPCSTR G_BaseFileName(LPCSTR path) {
    LPCSTR base = path;

    if (!path) {
        return "";
    }
    for (LPCSTR p = path; *p; p++) {
        if (*p == '\\' || *p == '/') {
            base = p + 1;
        }
    }
    return base;
}

static BOOL G_NameMatchesMapFile(LPCSTR name, LPCSTR path) {
    PATHSTR base;
    LPCSTR file;
    size_t len;

    if (!name || !path || !*name) {
        return false;
    }
    file = G_BaseFileName(path);
    snprintf(base, sizeof(base), "%s", file);
    len = strlen(base);
    if (len > 4 && (!strcasecmp(base + len - 4, ".w3m") ||
                    !strcasecmp(base + len - 4, ".w3x"))) {
        base[len - 4] = '\0';
    }
    return !strcasecmp(name, base);
}

static void G_SetMapDisplayName(mapListItem_t *item,
                                LPCSTR wts,
                                LPCSTR rawName,
                                LPCSTR rawDescription,
                                LPCSTR rawLoadingTitle,
                                LPCSTR rawLoadingSubtitle) {
    char name[128];
    char description[128];
    char loadingTitle[64];
    char loadingSubtitle[96];

    if (!item) {
        return;
    }

    G_ResolveTrigStr(rawName, wts, name, sizeof(name));
    G_SanitizeListField(name);
    snprintf(item->name, sizeof(item->name), "%s", name);

    if (strncasecmp(item->path, "Maps\\Campaign\\", 14) ||
        !G_NameMatchesMapFile(item->name, item->path)) {
        return;
    }

    G_ResolveTrigStr(rawDescription, wts, description, sizeof(description));
    G_ResolveTrigStr(rawLoadingTitle, wts, loadingTitle, sizeof(loadingTitle));
    G_ResolveTrigStr(rawLoadingSubtitle, wts, loadingSubtitle, sizeof(loadingSubtitle));
    G_SanitizeListField(description);
    G_SanitizeListField(loadingTitle);
    G_SanitizeListField(loadingSubtitle);

    if (loadingTitle[0] && loadingSubtitle[0]) {
        snprintf(item->name, sizeof(item->name), "%s: %s", loadingTitle, loadingSubtitle);
    } else if (description[0]) {
        snprintf(item->name, sizeof(item->name), "%s", description);
    } else if (loadingTitle[0]) {
        snprintf(item->name, sizeof(item->name), "%s", loadingTitle);
    } else if (loadingSubtitle[0]) {
        snprintf(item->name, sizeof(item->name), "%s", loadingSubtitle);
    }
}

static void G_SanitizeListField(LPSTR text) {
    if (!text) {
        return;
    }
    for (LPSTR p = text; *p; p++) {
        if (*p == '\n' || *p == '\r' || *p == '\t') {
            *p = ' ';
        }
    }
}

static void G_SanitizeInfoText(LPSTR text) {
    if (!text) {
        return;
    }
    for (LPSTR p = text; *p; p++) {
        if (*p == '\t') {
            *p = ' ';
        }
    }
}

static LPCSTR G_TilesetName(BYTE tileset) {
    switch (tileset) {
        case 'A': return "Ashenvale";
        case 'B': return "Barrens";
        case 'C': return "Felwood";
        case 'D': return "Dungeon";
        case 'F': return "Lordaeron Fall";
        case 'G': return "Underground";
        case 'I': return "Icecrown Glacier";
        case 'J': return "Dalaran";
        case 'K': return "Black Citadel";
        case 'L': return "Lordaeron Summer";
        case 'N': return "Northrend";
        case 'O': return "Outland";
        case 'Q': return "Village Fall";
        case 'V': return "Village";
        case 'W': return "Lordaeron Winter";
        case 'X': return "Dalaran Ruins";
        case 'Y': return "Cityscape";
        case 'Z': return "Sunken Ruins";
        default: return "UNKNOWNMAP_TILESET";
    }
}

static LPCSTR G_MapSizeName(DWORD width, DWORD height) {
    DWORD largest = MAX(width, height);

    if (largest <= 96) {
        return "Small";
    }
    if (largest <= 128) {
        return "Medium";
    }
    if (largest <= 160) {
        return "Large";
    }
    return "Huge";
}

static BOOL G_ReadArchiveFile(HANDLE archive, LPCSTR name, LPBYTE *out, LPDWORD out_size) {
    HANDLE file;
    DWORD size;
    DWORD bytesRead = 0;
    LPBYTE buffer;

    if (!archive || !name || !out || !out_size ||
        !gi.OpenFileEx || !gi.GetArchiveFileSize || !gi.ReadArchiveFile || !gi.CloseFile) {
        return false;
    }
    *out = NULL;
    *out_size = 0;
    if (!gi.OpenFileEx(archive, name, SFILE_OPEN_FROM_MPQ, &file)) {
        return false;
    }
    size = gi.GetArchiveFileSize(file, NULL);
    if (size == 0) {
        gi.CloseFile(file);
        return false;
    }
    buffer = gi.MemAlloc(size + 1);
    if (!buffer) {
        gi.CloseFile(file);
        return false;
    }
    if (!gi.ReadArchiveFile(file, buffer, size, &bytesRead, NULL) || bytesRead != size) {
        gi.MemFree(buffer);
        gi.CloseFile(file);
        return false;
    }
    buffer[size] = '\0';
    gi.CloseFile(file);
    *out = buffer;
    *out_size = size;
    return true;
}

static BOOL G_ParseMapInfo(mapListItem_t *item) {
    HANDLE mapData;
    DWORD mapSize = 0;
    HANDLE archive;
    LPBYTE w3i = NULL;
    DWORD w3iSize = 0;
    LPBYTE wts = NULL;
    DWORD wtsSize = 0;
    mapReader_t reader;
    char rawName[128];
    char rawDescription[128];
    char rawSuggestedPlayers[96];
    char rawLoadingTitle[64];
    char rawLoadingSubtitle[96];
    DWORD version = 0;
    DWORD mapWidth = 0;
    DWORD mapHeight = 0;
    DWORD playerCount = 0;
    BYTE tileset = 0;

    if (!item || !gi.ReadFile || !gi.MemFree ||
        !gi.OpenArchiveFromMemory || !gi.CloseArchive) {
        return false;
    }

    mapData = gi.ReadFile(item->path, &mapSize);
    if (!mapData || mapSize == 0) {
        return false;
    }
    if (!gi.OpenArchiveFromMemory(mapData, mapSize, 0, &archive)) {
        gi.MemFree(mapData);
        return false;
    }
    if (!G_ReadArchiveFile(archive, "war3map.w3i", &w3i, &w3iSize)) {
        gi.CloseArchive(archive);
        gi.MemFree(mapData);
        return false;
    }
    G_ReadArchiveFile(archive, "war3map.wts", &wts, &wtsSize);

    reader = MAKE(mapReader_t, .data = w3i, .size = w3iSize, .pos = 0);
    rawName[0] = '\0';
    rawDescription[0] = '\0';
    rawSuggestedPlayers[0] = '\0';
    rawLoadingTitle[0] = '\0';
    rawLoadingSubtitle[0] = '\0';

    if (!G_MapReadU32(&reader, &version) ||
        !G_MapSkip(&reader, sizeof(DWORD) * 2)) {
        goto fail;
    }
    if (version > 27 && !G_MapSkip(&reader, sizeof(DWORD) * 4)) {
        goto fail;
    }
    if (!G_MapReadCString(&reader, rawName, sizeof(rawName)) ||
        !G_MapReadCString(&reader, NULL, 0) ||
        !G_MapReadCString(&reader, rawDescription, sizeof(rawDescription)) ||
        !G_MapReadCString(&reader, rawSuggestedPlayers, sizeof(rawSuggestedPlayers)) ||
        !G_MapSkip(&reader, sizeof(FLOAT) * 8) ||
        !G_MapSkip(&reader, sizeof(DWORD) * 4) ||
        !G_MapReadU32(&reader, &mapWidth) ||
        !G_MapReadU32(&reader, &mapHeight) ||
        !G_MapReadU32(&reader, &item->flags) ||
        !G_MapRead(&reader, &tileset, sizeof(tileset)) ||
        !G_MapSkip(&reader, sizeof(DWORD))) {
        goto fail;
    }
    if (version > 24 && !G_MapReadCString(&reader, NULL, 0)) {
        goto fail;
    }
    if (!G_MapReadCString(&reader, NULL, 0) ||
        !G_MapReadCString(&reader, rawLoadingTitle, sizeof(rawLoadingTitle)) ||
        !G_MapReadCString(&reader, rawLoadingSubtitle, sizeof(rawLoadingSubtitle)) ||
        !G_MapSkip(&reader, sizeof(DWORD))) {
        goto fail;
    }
    if (version > 24 && !G_MapReadCString(&reader, NULL, 0)) {
        goto fail;
    }
    if (!G_MapReadCString(&reader, NULL, 0) ||
        !G_MapReadCString(&reader, NULL, 0) ||
        !G_MapReadCString(&reader, NULL, 0)) {
        goto fail;
    }
    if (version > 24) {
        if (!G_MapSkip(&reader, sizeof(DWORD) + sizeof(FLOAT) * 2 + sizeof(FLOAT) + sizeof(BYTE) * 4 + sizeof(DWORD)) ||
            !G_MapReadCString(&reader, NULL, 0) ||
            !G_MapSkip(&reader, sizeof(BYTE) + sizeof(BYTE) * 4)) {
            goto fail;
        }
    }
    if (version > 27 && !G_MapSkip(&reader, sizeof(BYTE) * 4)) {
        goto fail;
    }
    if (version > 30 && !G_MapSkip(&reader, sizeof(DWORD) * 2)) {
        goto fail;
    }
    if (!G_MapReadU32(&reader, &playerCount)) {
        goto fail;
    }

    G_SetMapDisplayName(item,
                        (LPCSTR)wts,
                        rawName,
                        rawDescription,
                        rawLoadingTitle,
                        rawLoadingSubtitle);
    G_ResolveTrigStr(rawDescription, (LPCSTR)wts, item->description, sizeof(item->description));
    G_ResolveTrigStr(rawSuggestedPlayers, (LPCSTR)wts, item->suggestedPlayers, sizeof(item->suggestedPlayers));
    G_SanitizeInfoText(item->description);
    G_SanitizeInfoText(item->suggestedPlayers);
    snprintf(item->mapSize, sizeof(item->mapSize), "%s", G_MapSizeName(mapWidth, mapHeight));
    snprintf(item->tileset, sizeof(item->tileset), "%s", G_TilesetName(tileset));
    item->players = MIN(playerCount, MAX_PLAYERS);
    if (wts) {
        gi.MemFree(wts);
    }
    gi.MemFree(w3i);
    gi.CloseArchive(archive);
    gi.MemFree(mapData);
    return true;

fail:
    if (wts) {
        gi.MemFree(wts);
    }
    if (w3i) {
        gi.MemFree(w3i);
    }
    gi.CloseArchive(archive);
    gi.MemFree(mapData);
    return false;
}

static int G_CompareMapListItems(const void *a, const void *b) {
    const mapListItem_t *ma = a;
    const mapListItem_t *mb = b;

    if (ma->players != mb->players) {
        return (ma->players < mb->players) ? -1 : 1;
    }
    return strcasecmp(ma->name, mb->name);
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

static BOOL G_AddMapListItem(mapListItem_t *items,
                             DWORD *itemCount,
                             LPCSTR path)
{
    if (!items || !itemCount || !path || !*path || *itemCount >= MAX_MAP_LIST_ITEMS) {
        return false;
    }
    FOR_LOOP(i, *itemCount) {
        if (!strcasecmp(items[i].path, path)) {
            return false;
        }
    }

    mapListItem_t *item = &items[(*itemCount)++];
    memset(item, 0, sizeof(*item));
    snprintf(item->path, sizeof(item->path), "%s", path);
    snprintf(item->name, sizeof(item->name), "%s", path);
    return true;
}

static DWORD G_CollectMapList(mapListItem_t *items, DWORD maxItems) {
    SFILE_FIND_DATA find;
    HANDLE handle;
    DWORD itemCount = 0;

    if (!items || maxItems == 0) {
        return 0;
    }
    memset(items, 0, sizeof(mapListItem_t) * maxItems);
    handle = gi.FindFirstFile ? gi.FindFirstFile("Maps\\*", &find) : NULL;
    while (handle && itemCount < maxItems) {
        if (G_IsMapFile(find.cFileName) && !G_IsCampaignMap(find.cFileName)) {
            G_AddMapListItem(items, &itemCount, find.cFileName);
        }
        if (!gi.FindNextFile || !gi.FindNextFile(handle, &find)) {
            break;
        }
    }
    if (handle && gi.FindClose) {
        gi.FindClose(handle);
    }

    DWORD parsedCount = 0;
    FOR_LOOP(i, itemCount) {
        BOOL parsed = G_ParseMapInfo(&items[i]);

        if (!parsed) {
            continue;
        }
        if (parsedCount != i) {
            items[parsedCount] = items[i];
        }
        parsedCount++;
    }
    itemCount = parsedCount;
    qsort(items, itemCount, sizeof(items[0]), G_CompareMapListItems);
    return itemCount;
}

static void G_ListMaps(LPSTR text, DWORD text_size) {
    mapListItem_t items[MAX_MAP_LIST_ITEMS];
    DWORD itemCount;
    DWORD numRows = 0;

    if (!text || text_size == 0) {
        return;
    }

    text[0] = '\0';
    itemCount = G_CollectMapList(items, MAX_MAP_LIST_ITEMS);
    numRows = 0;
    FOR_LOOP(i, itemCount) {
        char row[256];
        mapListItem_t *item = &items[i];

        snprintf(row,
                 sizeof(row),
                 "%s\t%s\t%u\t%s",
                 item->name[0] ? item->name : item->path,
                 item->path,
                 (unsigned)item->players,
                 (item->flags & melee_map) ? "melee" : "custom");
        G_ListAppendRow(text, text_size, &numRows, row);
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

void CMD_ListSelect(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    mapListItem_t items[MAX_MAP_LIST_ITEMS];
    DWORD itemCount;
    DWORD rowIndex;
    mapListItem_t *item;

    if (argc < 3 || strcmp(argv[1], "maps")) {
        return;
    }

    rowIndex = (DWORD)strtoul(argv[2], NULL, 10);
    itemCount = G_CollectMapList(items, MAX_MAP_LIST_ITEMS);
    item = (rowIndex < itemCount && rowIndex < MAX_LIST_FETCH_ROWS) ? &items[rowIndex] : NULL;

    G_SetPlayerText(ent->client, PLAYERTEXT_MAP_TITLE, item ? item->name : " ");
    G_SetPlayerText(ent->client, PLAYERTEXT_MAP_SUGGESTED_PLAYERS,
                    item ? item->suggestedPlayers : UI_GetString("UNKNOWNMAP_SUGGESTEDPLAYERS"));
    G_SetPlayerText(ent->client, PLAYERTEXT_MAP_SIZE,
                    item ? item->mapSize : UI_GetString("UNKNOWNMAP_MAPSIZE"));
    G_SetPlayerText(ent->client, PLAYERTEXT_MAP_TILESET,
                    item ? item->tileset : UI_GetString("UNKNOWNMAP_TILESET"));
    G_SetPlayerText(ent->client, PLAYERTEXT_MAP_DESCRIPTION,
                    item ? item->description : UI_GetString("UNKNOWNMAP_DESCRIPTION"));
}
