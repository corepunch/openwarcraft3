#ifndef war3map_h
#define war3map_h

#include "common.h"

struct War3MapVertex {
    USHORT accurate_height;
    USHORT waterlevel;
    BYTE mapedge;
    BYTE ground;
    BYTE ramp;
    BYTE blight;
    BYTE water;
    BYTE boundary;
    BYTE groundVariation;
    BYTE cliffVariation; // used also to mark mid-ramp
    BYTE level;
    BYTE cliff;
};

struct war3map {
    DWORD header;
    DWORD version;
    BYTE tileset;
    DWORD custom;
    LPDWORD grounds;
    LPDWORD cliffs;
    VECTOR2 center;
    DWORD width;
    DWORD height;
    HANDLE vertices;
    DWORD num_grounds;
    DWORD num_cliffs;
};

bool CM_LoadMap(LPCSTR mapFilename);
BOOL CM_ReadMapInfo(LPCSTR mapFilename, LPMAPINFO info);
BOOL CM_FindMapPreviewTexture(LPCSTR mapFilename, LPSTR out, DWORD out_size);
void CM_FreeMapInfo(LPMAPINFO info);
void CM_DefaultMapName(LPCSTR path, LPSTR out, DWORD out_size);
void CM_ResolveMapInfoString(LPCMAPINFO info, LPCSTR text, LPSTR out, DWORD out_size);
BOOL CM_MapNameMatchesFile(LPCSTR name, LPCSTR path);
LPCSTR CM_TilesetName(BYTE tileset);
LPCSTR CM_MapSizeName(DWORD width, DWORD height);
void CM_SanitizeMapListField(LPSTR text);
void CM_SanitizeMapInfoText(LPSTR text);
float CM_GetHeightAtPoint(float sx, float sy);
LPDOODAD CM_GetDoodads(void);
//LPCMAPPLAYER CM_GetPlayer(DWORD index);
DWORD CM_GetLocalPlayerNumber(void);
LPCMAPINFO CM_GetMapInfo(void);
VECTOR2 CM_GetNormalizedMapPosition(float x, float y);
VECTOR2 CM_GetDenormalizedMapPosition(float x, float y);
BOOL CM_ClosestPathablePoint(LPCVECTOR2 location, LPVECTOR2 out);
BOOL CM_ClosestPathablePointForRadius(LPCVECTOR2 location, FLOAT radius, LPVECTOR2 out);
BOX2 CM_GetWorldBounds(void);

#endif
