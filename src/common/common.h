#ifndef __common_h__
#define __common_h__

#include "shared.h"
#include "net.h"

#define MAP_VERTEX_SIZE 7
#define MAX_SHEET_LINE 1024
#define MAX_COMMAND_ENTITIES 64
#define HEIGHT_COR (TILESIZE * 2 + 5)
#define WATER_HEIGHT_COR 80
#define DECODE_HEIGHT(x) (((x) - 0x2000) / 4)
#define CMDARG_LEN 64
#define MAX_CMDARGS 64
#define UPDATE_BACKUP 16
#define UPDATE_MASK (UPDATE_BACKUP-1)
#define U_REMOVE 15
#define NUM_THREADS 16

#define SFileReadArray(file, object, variable, elemsize, alloc) \
SFileReadFile(file, &object->num_##variable, 4, NULL, NULL); \
if (object->num_##variable > 0) {object->variable = alloc(object->num_##variable * elemsize); \
SFileReadFile(file, object->variable, object->num_##variable * elemsize, NULL, NULL); }

typedef enum {
    ERR_FATAL,        // exit the entire game with a popup window
    ERR_DROP,         // print to console and disconnect from game
    ERR_QUIT,         // not an error, just a normal exit
} errorCode_t;

// server to client
enum svc_ops {
    svc_bad,
// these ops are known to the game dll
//    svc_muzzleflash,
//    svc_muzzleflash2,
    svc_temp_entity,
    svc_layout,
    svc_playerinfo,
    svc_cursor,

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
    svc_frame,
    svc_mirror
};

// client to server
enum clc_ops {
    clc_bad,
//    clc_nop,
    clc_move,
//    clc_userinfo,            // [[userinfo string]
    clc_stringcmd            // [string] message
};

typedef enum t_attrib_id {
    attrib_position,
    attrib_color,
    attrib_texcoord,
    attrib_normal,
    attrib_skin1,
    attrib_skin2,
    attrib_boneWeight1,
    attrib_boneWeight2,
    attrib_particleAxis,
    attrib_particleSize,
} t_attrib_id;

struct texture;
struct font;

typedef void (*xcommand_t)(void);

typedef struct model {
    unsigned int modeltype;
    struct mdxModel_s *mdx;
    struct m3Model_s *m3;
} model_t;

KNOWN_AS(model, MODEL);
KNOWN_AS(texture, TEXTURE);
KNOWN_AS(font, FONT);
KNOWN_AS(War3MapVertex, WAR3MAPVERTEX);
KNOWN_AS(war3map, WAR3MAP);
KNOWN_AS(TerrainInfo, TERRAININFO);
KNOWN_AS(CliffInfo, CLIFFINFO);

#include "cmodel.h"

// common.c
void Com_Init(void);
void Com_Error(errorCode_t code, LPCSTR fmt, ...);

void LoadMap(LPCSTR pFilename);

void FS_Init(void);
void FS_Shutdown(void);

void Com_Quit(void);
void Sys_Quit(void);

HANDLE FS_OpenFile(LPCSTR fileName);
void FS_CloseFile(HANDLE file);
bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted);
bool FS_FileExists(LPCSTR fileName);
HANDLE FS_ReadFile(LPCSTR filename, LPDWORD size);

sheetRow_t *FS_ParseINI(LPCSTR fileName);
sheetRow_t *FS_ParseSLK(LPCSTR fileName);
LPCSTR FS_FindSheetCell(sheetRow_t *sheet, LPCSTR row, LPCSTR column);

void CL_Init(void);
void CL_Frame(DWORD msec);
void CL_Shutdown(void);

void SV_Init(void);
void SV_Frame(DWORD msec);
void SV_Shutdown(void);

HANDLE MemAlloc(long size);
void MemFree(HANDLE mem);

void Sys_MkDir(LPCSTR directory);

struct edict_s;
DWORD CM_BuildHeatmap(struct edict_s *goalentity);

// parser.c
LPSTR ParserGetTokenEx(parser_t *p, bool sameLine);
LPSTR ParserGetToken(parser_t *p);
LPSTR FS_ReadFileIntoString(LPCSTR fileName);
void ParserError(parser_t *p);

// cmd.c
void Cbuf_Init(void);
void Cbuf_AddText(LPCSTR text);
void Cbuf_Execute(void);
void Cmd_AddCommand(LPCSTR cmd_name, xcommand_t function);
void Cmd_RemoveCommand(LPCSTR cmd_name);
bool Cmd_Exists(LPCSTR cmd_name);
void Cmd_ExecuteString(LPCSTR text);
void Cmd_ForwardToServer(LPCSTR text);

#endif
