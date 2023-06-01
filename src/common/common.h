#ifndef __common_h__
#define __common_h__

#include "shared.h"
#include "net.h"

//#define DEBUG_PATHFINDING

#define TMP_MAP "/tmp/map.w3m"
#define MAP_VERTEX_SIZE 7
#define MAX_SHEET_LINE 1024
#define TILESIZE 128
#define MAX_COMMAND_ENTITIES 64
#define HEIGHT_COR (TILESIZE * 2 + 5)
#define DECODE_HEIGHT(x) (((x) - 0x2000) / 4)
#define CMDARG_LEN 64
#define MAX_CMDARGS 64


#define UPDATE_BACKUP 16
#define UPDATE_MASK (UPDATE_BACKUP-1)

#define U_REMOVE 15

#define SFileReadArray(file, object, variable, elemsize, alloc) \
SFileReadFile(file, &object->num_##variable, 4, NULL, NULL); \
if (object->num_##variable > 0) {object->variable = alloc(object->num_##variable * elemsize); \
SFileReadFile(file, object->variable, object->num_##variable * elemsize, NULL, NULL); }

// server to client
enum svc_ops {
    svc_bad,
// these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
//    svc_temp_entity,
    svc_layout,
    svc_playerinfo,

// the rest are private to the client and server
//    svc_nop,
//    svc_disconnect,
//    svc_reconnect,
//    svc_sound,                    // <see code>
//    svc_print,                    // [byte] id [string] null terminated string
//    svc_stufftext,                // [string] stuffed into client's console buffer, should be \n terminated
//    svc_serverdata,                // [long] protocol ...
    svc_configstring,            // [short] [string]
    svc_spawnbaseline,
//    svc_centerprint,            // [string] to put in center of the screen
//    svc_download,                // [short] size [size bytes]
//    svc_playerinfo,                // variable
    svc_packetentities,            // [...]
//    svc_deltapacketentities,    // [...]
    svc_frame
};

// client to server
enum clc_ops {
    clc_bad,
//    clc_nop,
    clc_move,
//    clc_command,
//    clc_userinfo,            // [[userinfo string]
    clc_stringcmd            // [string] message
};

typedef enum t_attrib_id {
    attrib_position,
    attrib_color,
    attrib_texcoord,
    attrib_texcoord2,
    attrib_normal,
    attrib_skin1,
    attrib_skin2,
    attrib_boneWeight1,
    attrib_boneWeight2,
} t_attrib_id;

typedef struct {
    int x;
    int y;
} point2_t;

struct texture;

typedef struct {
    unsigned int modeltype;
    struct mdxModel_s *mdx;
} model_t;

KNOWN_AS(mdx, MODEL);
KNOWN_AS(texture, TEXTURE);
KNOWN_AS(War3MapVertex, WAR3MAPVERTEX);
KNOWN_AS(war3map, WAR3MAP);
KNOWN_AS(TerrainInfo, TERRAININFO);
KNOWN_AS(CliffInfo, CLIFFINFO);

#include "cmodel.h"

HANDLE FS_ParseSheet(LPCSTR fileName, LPCSHEETLAYOUT layout, DWORD elementSize);
void LoadMap(LPCSTR pFilename);

void FS_Init(void);
void FS_Shutdown(void);

void Com_Quit(void);
void Sys_Quit(void);

HANDLE FS_OpenFile(LPCSTR fileName);
void FS_CloseFile(HANDLE file);
bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted);
LPSHEETCELL FS_ReadSheet(LPCSTR fileName);

void CL_Init(void);
void CL_Frame(DWORD msec);
void CL_Shutdown(void);

void SV_Init(void);
void SV_Frame(DWORD msec);
void SV_Shutdown(void);

HANDLE MemAlloc(long size);
void MemFree(HANDLE mem);

void Sys_MkDir(LPCSTR directory);

handle_t CM_BuildHeatmap(LPCVECTOR2 target);

// INI
LPSTR ParserGetTokenEx(parser_t *p, bool sameLine);
LPSTR ParserGetToken(parser_t *p);
configValue_t *FS_ParseConfig(LPCSTR filename);
LPCSTR INI_FindValue(configValue_t *config, LPCSTR sectionName, LPCSTR valueName);
LPSTR FS_ReadFileIntoString(LPCSTR fileName);
void ParserError(parser_t *p);

#endif
