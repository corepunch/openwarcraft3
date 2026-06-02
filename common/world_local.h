#ifndef WORLD_LOCAL_H
#define WORLD_LOCAL_H

#include "common.h"
#include "mpq.h"

#ifdef GAME_WORLD
extern struct game_import gi;
static inline HANDLE G_WorldReadFile(LPCSTR filename, LPDWORD size) {
    return gi.ReadFile(filename, size);
}
static inline HANDLE G_WorldMemAlloc(long size) {
    return gi.MemAlloc(size);
}
static inline void G_WorldMemFree(HANDLE mem) {
    gi.MemFree(mem);
}
static inline BOMStatus G_WorldTextRemoveBom(LPSTR buffer) {
    unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
    unsigned char utf16le_bom[] = { 0xFF, 0xFE };
    unsigned char utf16be_bom[] = { 0xFE, 0xFF };
    size_t len;

    if (!buffer) {
        return INVALID_BOM;
    }
    len = strlen(buffer);
    if (len >= 3 && memcmp(buffer, utf8_bom, 3) == 0) {
        memmove(buffer, buffer + 3, len - 3 + 1);
        return UTF8_BOM_FOUND;
    }
    if (len >= 2 && memcmp(buffer, utf16le_bom, 2) == 0) {
        memmove(buffer, buffer + 2, len - 2 + 1);
        return UTF16LE_BOM_FOUND;
    }
    if (len >= 2 && memcmp(buffer, utf16be_bom, 2) == 0) {
        memmove(buffer, buffer + 2, len - 2 + 1);
        return UTF16BE_BOM_FOUND;
    }
    return NO_BOM;
}
#define FS_ReadFile G_WorldReadFile
#define FS_FreeFile G_WorldMemFree
#define MemAlloc G_WorldMemAlloc
#define MemFree G_WorldMemFree
#define PF_TextRemoveBom G_WorldTextRemoveBom
#define Com_Error(code, ...) gi.error(__VA_ARGS__)
#endif

struct world_state {
    LPWAR3MAP map;
    MAPINFO info;
    struct Doodad *doodads;
};

extern struct world_state world;

/* Implemented in world_w3.c / world_wow.c respectively */
bool     CM_LoadMapFormat(LPCSTR mapFilename);
FLOAT    CM_GetHeightAtPoint(FLOAT sx, FLOAT sy);
VECTOR2  CM_GetNormalizedMapPosition(FLOAT x, FLOAT y);
VECTOR2  CM_GetDenormalizedMapPosition(FLOAT x, FLOAT y);
BOX2     CM_GetWorldBounds(void);

#endif
