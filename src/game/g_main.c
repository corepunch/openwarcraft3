#include "g_local.h"

#include "Doodads/Doodads.h"
#include "Units/DestructableData.h"
#include "Units/UnitUI.h"
#include "Units/UnitBalance.h"
#include "Units/UnitWeapons.h"
#include "Units/UnitAbilities.h"
#include "Units/AbilityData.h"

#define MAX_ENTITIES 4096

struct game_export globals;
struct game_import gi;
struct game_state game_state;
struct game_locals game;

LPCSTR configs[] = {
    "Units\\CampaignAbilityFunc.txt",
    "Units\\CampaignAbilityStrings.txt",
    "Units\\CommonAbilityFunc.txt",
    "Units\\CommonAbilityStrings.txt",
    "Units\\HumanAbilityFunc.txt",
    "Units\\HumanAbilityStrings.txt",
    "Units\\NeutralAbilityFunc.txt",
    "Units\\NeutralAbilityStrings.txt",
    "Units\\NightElfAbilityFunc.txt",
    "Units\\NightElfAbilityStrings.txt",
    "Units\\OrcAbilityFunc.txt",
    "Units\\OrcAbilityStrings.txt",
    "Units\\UndeadAbilityFunc.txt",
    "Units\\UndeadAbilityStrings.txt",
    "Units\\ItemAbilityFunc.txt",
    "Units\\ItemAbilityStrings.txt",
    NULL
};

configValue_t *abilityConfigs = NULL;

static void InitAbilityConfigs(void) {
    for (LPCSTR *config = configs; *config; config++) {
        configValue_t *current = gi.ParseConfig(*config);
        if (current) {
            PUSH_BACK(configValue_t, current, abilityConfigs);
        }
    }
}

static void G_Init(void) {
    game_state.edicts = gi.MemAlloc(sizeof(struct edict) * MAX_ENTITIES);
    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;

    InitAbilityConfigs();
    
    InitDoodads();
    InitDestructableData();
    InitUnitUI();
    InitUnitBalance();
    InitUnitWeapons();
    InitUnitAbilities();
    InitAbilityData();
}

static void G_Shutdown(void) {
    gi.MemFree(game_state.edicts);

    ShutdownDoodads();
    ShutdownDestructableData();
    ShutdownUnitUI();
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
    globals.Init = G_Init;
    globals.Shutdown = G_Shutdown;
    globals.SpawnDoodads = G_SpawnDoodads;
    globals.RunFrame = G_RunFrame;
    globals.ClientCommand = G_ClientCommand;
    globals.edict_size = sizeof(struct edict);
    return &globals;
}
