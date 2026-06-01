#include "g_sc2_local.h"
#include <stdio.h>
#include <string.h>

struct game_import gi;
struct game_export globals;

static edict_t sc2_edicts[SC2_MAX_EDICTS];
static struct client_s sc2_clients[SC2_MAX_CLIENTS];

static void SC2_Init(void) {
    memset(sc2_edicts,  0, sizeof(sc2_edicts));
    memset(sc2_clients, 0, sizeof(sc2_clients));

    globals.edicts     = sc2_edicts;
    globals.num_edicts = 1;
    globals.max_edicts = SC2_MAX_EDICTS;
    globals.max_clients = SC2_MAX_CLIENTS;
}

static void SC2_Shutdown(void) {
    G_FreeModels();
}

static void SC2_SpawnEntities(LPCMAPINFO mapinfo, LPCDOODAD doodads) {
    (void)mapinfo; (void)doodads;
}

static void SC2_RunFrame(void) {
}

static void SC2_ClientBegin(LPEDICT ent) {
    (void)ent;
}

static void SC2_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    (void)ent; (void)argc; (void)argv;
}

static void SC2_ClientSetCameraPosition(LPEDICT ent, LPCVECTOR2 position) {
    (void)ent; (void)position;
}

static BOOL SC2_CanSeeEntity(DWORD player, LPCEDICT ent) {
    (void)player; (void)ent;
    return true;
}

static LPCSTR SC2_GetThemeValue(LPCSTR filename) {
    (void)filename;
    return NULL;
}

struct game_export *GetGameAPI(struct game_import *import) {
    gi = *import;

    globals.Init                  = SC2_Init;
    globals.Shutdown              = SC2_Shutdown;
    globals.SpawnEntities         = SC2_SpawnEntities;
    globals.RunFrame              = SC2_RunFrame;
    globals.ClientBegin           = SC2_ClientBegin;
    globals.ClientCommand         = SC2_ClientCommand;
    globals.ClientSetCameraPosition = SC2_ClientSetCameraPosition;
    globals.CanSeeEntity          = SC2_CanSeeEntity;
    globals.GetThemeValue         = SC2_GetThemeValue;
    globals.edict_size            = sizeof(edict_t);
    globals.max_clients           = SC2_MAX_CLIENTS;
    globals.max_edicts            = SC2_MAX_EDICTS;

    return &globals;
}
