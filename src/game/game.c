#include "g_local.h"

#include "Doodads/Doodads.h"
#include "Units/DestructableData.h"
#include "Units/UnitUI.h"
#include "Units/UnitBalance.h"
#include "Units/UnitWeapons.h"
#include "Units/UnitAbilities.h"
#include "Units/UnitData.h"
#include "Units/AbilityData.h"

#define MAX_ENTITIES 4096

struct game_export globals;
struct game_import gi;
struct game_state game_state;
struct game_locals game;

static void G_InitGame(void) {
    game_state.edicts = gi.MemAlloc(sizeof(edict_t) * MAX_ENTITIES);

    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;
    globals.max_clients = 16;

    game.max_clients = globals.max_clients;
    game.clients = gi.MemAlloc(game.max_clients * sizeof(gclient_t));

    InitAbilities();
    InitConfigFiles();
    InitDoodads();
    InitDestructableData();
    InitUnitUI();
    InitUnitData();
    InitUnitBalance();
    InitUnitWeapons();
    InitUnitAbilities();
    InitAbilityData();
}

static void G_ShutdownGame(void) {
    gi.MemFree(game_state.edicts);

    ShutdownConfigFiles();
    ShutdownDoodads();
    ShutdownDestructableData();
    ShutdownUnitUI();
    ShutdownUnitData();
    ShutdownUnitBalance();
    ShutdownUnitWeapons();
    ShutdownUnitAbilities();
    ShutdownAbilityData();
}

static void G_RunFrame() {
    FOR_LOOP(i, globals.num_edicts) {
        G_RunEntity(&globals.edicts[i]);
    }
    G_SolveCollisions();
}

struct game_export *GetGameAPI(struct game_import *import) {
    memset(&game_state, 0, sizeof(struct game_state));
    gi = *import;
    globals.Init = G_InitGame;
    globals.Shutdown = G_ShutdownGame;
    globals.SpawnEntities = G_SpawnEntities;
    globals.RunFrame = G_RunFrame;
    globals.ClientCommand = G_ClientCommand;
    globals.edict_size = sizeof(struct edict_s);
    return &globals;
}
