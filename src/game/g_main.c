#include "g_local.h"

#define MAX_ENTITIES 4096

struct game_export globals;
struct game_import gi;
struct game_state game_state;
struct game_locals game;

void G_Init(void) {
    game_state.edicts = gi.MemAlloc(sizeof(struct edict) * MAX_ENTITIES);
    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;

    G_InitUnits();
    G_InitDestructables();
    G_InitDoodads();
}

void G_Shutdown(void) {
    gi.MemFree(game_state.edicts);
}

static void G_RunEntity(LPEDICT lpEdict, DWORD msec) {
    if (lpEdict->think) {
        lpEdict->think(lpEdict, msec);
    }
}

void G_RunFrame(DWORD msec) {
    FOR_LOOP(i, globals.num_edicts) {
        G_RunEntity(&globals.edicts[i], msec);
    }
}

struct game_export *GetGameAPI(struct game_import *import) {
    memset(&game_state, 0, sizeof(struct game_state));
    gi = *import;
    globals.Init = G_Init;
    globals.Shutdown = G_Shutdown;
    globals.SpawnEntities = G_SpawnEntities;
    globals.RunFrame = G_RunFrame;
    globals.edict_size = sizeof(struct edict);
    return &globals;
}
