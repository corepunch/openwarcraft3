#ifndef SC2_MAP_H
#define SC2_MAP_H

#include "common/common.h"

#define SC2_MAX_MAP_OBJECTS 1024
#define SC2_DEFAULT_MAP_WIDTH  96
#define SC2_DEFAULT_MAP_HEIGHT 96
#define SC2_CELL_SIZE          1.0f
#define SC2_MAX_TERRAIN_TEXTURES 16
#define SC2_MAX_CLIFF_SETS     8
#define SC2_MAX_CLIFF_CELLS    16384
#define SC2_OBJECT_HEIGHT_ABSOLUTE 0x00000001
#define SC2_OBJECT_HEIGHT_OFFSET   0x00000002
#define SC2_OBJECT_FORCE_PLACEMENT 0x00000004

typedef enum {
    SC2_OBJECT_UNIT,
    SC2_OBJECT_DOODAD,
} sc2ObjectType_t;

typedef struct {
    sc2ObjectType_t type;
    DWORD           id;
    char            name[64];
    char            model[256];
    VECTOR3         position;
    FLOAT           angle;
    FLOAT           scale;
    DWORD           variation;
    DWORD           player;
    DWORD           flags;
} sc2MapObject_t;

typedef struct {
    char           diffuse[256];
    char           normal[256];
} sc2TerrainTexture_t;

typedef struct {
    char           name[64];
    char           mesh[64];
} sc2CliffSet_t;

typedef struct {
    DWORD          index;
    DWORD          flags;
    DWORD          cliff_set;
    DWORD          variant;
} sc2CliffCell_t;

typedef struct {
    char           map_name[128];
    char           tile_set[64];
    DWORD          width;
    DWORD          height;
    DWORD          full_width;
    DWORD          full_height;
    BOOL           has_playable_bounds;
    LONG           playable_left;
    LONG           playable_bottom;
    LONG           playable_right;
    LONG           playable_top;
    VECTOR2        origin;
    FLOAT          cell_size;
    BOOL           generated;
    DWORD          num_objects;
    sc2MapObject_t objects[SC2_MAX_MAP_OBJECTS];
    DWORD          num_terrain_textures;
    sc2TerrainTexture_t terrain_textures[SC2_MAX_TERRAIN_TEXTURES];
    DWORD          texture_mask_width;
    DWORD          texture_mask_height;
    DWORD          num_texture_masks;
    LPBYTE         texture_masks[SC2_MAX_TERRAIN_TEXTURES];
    DWORD          cell_flags_width;
    DWORD          cell_flags_height;
    LPBYTE         cell_flags;
    DWORD          cliff_level_width;
    DWORD          cliff_level_height;
    USHORT        *cliff_levels;
    DWORD          num_cliff_sets;
    sc2CliffSet_t cliff_sets[SC2_MAX_CLIFF_SETS];
    DWORD          num_cliff_cells;
    sc2CliffCell_t cliff_cells[SC2_MAX_CLIFF_CELLS];
    FLOAT          height_quantize_bias;
    FLOAT          height_quantize_scale;
    FLOAT          standard_height;
    DWORD          height_map_width;
    DWORD          height_map_height;
    FLOAT         *height_map;
    FLOAT         *height_adjust_map;
    BOOL           has_camera;
    DWORD          camera_priority;
    VECTOR3        camera_target;
    FLOAT          camera_distance;
    FLOAT          camera_pitch;
    FLOAT          camera_yaw;
    FLOAT          camera_fov;
    FLOAT          camera_znear;
    FLOAT          camera_zfar;
    FLOAT          camera_height_offset;
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
