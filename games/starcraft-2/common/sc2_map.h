#ifndef SC2_MAP_H
#define SC2_MAP_H

#include "common/common.h"

#define SC2_MAX_MAP_OBJECTS 1024
#define SC2_DEFAULT_MAP_WIDTH  96
#define SC2_DEFAULT_MAP_HEIGHT 96
#define SC2_CELL_SIZE          1.0f

typedef enum {
    SC2_OBJECT_UNIT,
    SC2_OBJECT_DOODAD,
} sc2ObjectType_t;

typedef struct {
    sc2ObjectType_t type;
    char            name[64];
    char            model[256];
    VECTOR3         position;
    FLOAT           angle;
    FLOAT           scale;
    DWORD           player;
} sc2MapObject_t;

typedef struct {
    char           map_name[128];
    DWORD          width;
    DWORD          height;
    VECTOR2        origin;
    FLOAT          cell_size;
    BOOL           generated;
    DWORD          num_objects;
    sc2MapObject_t objects[SC2_MAX_MAP_OBJECTS];
} sc2Map_t;

typedef struct {
    HANDLE (*read_file)(LPCSTR filename, LPDWORD size);
    void   (*free_file)(HANDLE file);
    HANDLE (*mem_alloc)(long size);
    void   (*mem_free)(HANDLE mem);
} sc2MapHost_t;

void          SC2_MapSetHost(sc2MapHost_t const *host);
BOOL          SC2_MapLoad(LPCSTR mapFilename);
void          SC2_MapShutdown(void);
sc2Map_t     *SC2_MapCurrent(void);
FLOAT         SC2_MapHeightAtPoint(FLOAT x, FLOAT y);
BOX2          SC2_MapBounds(void);
VECTOR2       SC2_MapNormalizedPosition(FLOAT x, FLOAT y);
VECTOR2       SC2_MapDenormalizedPosition(FLOAT x, FLOAT y);
DWORD         SC2_MapObjectClassId(sc2MapObject_t const *object);

#endif
