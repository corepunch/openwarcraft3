#include "g_local.h"

#include "tables/Doodads.h"
#include "tables/DestructableData.h"
#include "tables/UnitUI.h"

#define MAX_ENTITIES 4096

struct game_export globals;
struct game_import gi;
struct game_state game_state;
struct game_locals game;

static void G_Init(void) {
    game_state.edicts = gi.MemAlloc(sizeof(struct edict) * MAX_ENTITIES);
    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;

    InitDoodads();
    InitDestructableData();
    InitUnitUI();
}

static void G_Shutdown(void) {
    gi.MemFree(game_state.edicts);
}

static void G_ClientCommand(LPCCLIENTMESSAGE lpClientMessage) {
    LPEDICT lpEdict = &game_state.edicts[lpClientMessage->entity];
    switch (lpClientMessage->cmd) {
        case CMD_MOVE:
            lpEdict->goalentity = Waypoint_add(lpClientMessage->location);
            break;
        case CMD_ATTACK:
            lpEdict->goalentity = &game_state.edicts[lpClientMessage->targetentity];
            lpEdict->enemy = &game_state.edicts[lpClientMessage->targetentity];
            break;
        default:
            break;
    }
}

static void G_RunFrame() {
    FOR_LOOP(i, globals.num_edicts) {
        G_RunEntity(&globals.edicts[i]);
    }
}

struct game_export *GetGameAPI(struct game_import *import) {
    memset(&game_state, 0, sizeof(struct game_state));
    gi = *import;
    globals.Init = G_Init;
    globals.Shutdown = G_Shutdown;
    globals.SpawnEntities = G_SpawnEntities;
    globals.RunFrame = G_RunFrame;
    globals.ClientCommand = G_ClientCommand;
    globals.edict_size = sizeof(struct edict);
    return &globals;
}
