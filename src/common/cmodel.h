#ifndef war3map_h
#define war3map_h

#include "common.h"

enum MDLTEXOP {
  TEXOP_LOAD = 0x0,
  TEXOP_TRANSPARENT = 0x1,
  TEXOP_BLEND = 0x2,
  TEXOP_ADD = 0x3,
  TEXOP_ADD_ALPHA = 0x4,
  TEXOP_MODULATE = 0x5,
  TEXOP_MODULATE2X = 0x6,
  NUMTEXOPS = 0x7,
};

enum MDLGEO {
  MODEL_GEO_UNSHADED = 0x1,
  MODEL_GEO_SPHERE_ENV_MAP = 0x2,  // unused until v1500
  MODEL_GEO_WRAPWIDTH = 0x4,       // unused until v1500
  MODEL_GEO_WRAPHEIGHT = 0x8,      // unused until v1500
  MODEL_GEO_TWOSIDED = 0x10,
  MODEL_GEO_UNFOGGED = 0x20,
  MODEL_GEO_NO_DEPTH_TEST = 0x40,
  MODEL_GEO_NO_DEPTH_SET = 0x80,
  MODEL_GEO_NO_FALLBACK = 0x100,   // added in v1500. seen in ElwynnTallWaterfall01.mdx, FelwoodTallWaterfall01.mdx and LavaFallsBlackRock*.mdx
};

struct PathMapNode {
    BYTE unused:1;
    BYTE nowalk:1;
    BYTE nofly:1;
    BYTE nobuild:1;
    BYTE unused2:1;
    BYTE blight:1;
    BYTE nowater:1;
    BYTE unknown:1;
};

struct Doodad {
    DWORD doodID;
    DWORD variation;
    VECTOR3 position;
    float angle;
    VECTOR3 scale;
    BYTE nFlags;
    BYTE lifetime;
    DWORD id_num;
};

struct War3MapVertex {
    USHORT accurate_height;
    USHORT waterlevel;
    BYTE ground:4;
    BYTE ramp:1;
    BYTE blight:1;
    BYTE water:1;
    BYTE boundary:1;
    BYTE details:4;
    BYTE unknown:4;
    BYTE level:4;
    BYTE cliff:4;
};

struct war3map {
    DWORD header;
    DWORD version;
    BYTE tileset;
    DWORD custom;
    LPDWORD lpGrounds;
    LPDWORD lpCliffs;
    VECTOR2 center;
    DWORD width;
    DWORD height;
    LPWAR3MAPVERTEX vertices;
    DWORD numGrounds;
    DWORD numCliffs;
};

void CM_LoadMap(LPCSTR szMapFilename);
VECTOR3 CM_PointIntoHeightmap(LPCVECTOR3 lpPoint);
VECTOR3 CM_PointFromHeightmap(LPCVECTOR3 lpPoint);
bool CM_IntersectLineWithHeightmap(LPCLINE3 lpLine, LPVECTOR3 lpOutput);
float CM_GetHeightAtPoint(float sx, float sy);
DWORD CM_GetDoodadsArray(LPCDOODAD *lppDoodads);

#endif
