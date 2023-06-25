#include "g_local.h"
#include "g_unitdata.h"

#define MAX_ENTITIES 4096

struct game_export globals;
struct game_import gi;
struct game_state game_state;
struct game_locals game;

LPCSTR miscdata_files[] = {
    "UI\\MiscData.txt",
    "Units\\MiscData.txt",
    "Units\\MiscGame.txt",
    "UI\\MiscUI.txt",
    "UI\\SoundInfo\\MiscData.txt",
    "war3mapMisc.txt",
    NULL
};

static void InitMiscValue(LPCSTR name, float *dest) {
    LPCSTR strvalue = gi.FindSheetCell(game.config.misc, "Misc", name);
    *dest = strvalue ? atof(strvalue) : 0;
}

static void InitConstants(void) {
    for (LPCSTR *config = miscdata_files; *config; config++) {
        sheetRow_t *current = gi.ReadConfig(*config);
        if (current) {
            PUSH_BACK(sheetRow_t, current, game.config.misc);
        }
    }
    InitMiscValue("AttackHalfAngle", &game.constants.attackHalfAngle);
    InitMiscValue("MaxCollisionRadius", &game.constants.maxCollisionRadius);
    InitMiscValue("DecayTime", &game.constants.decayTime);
    InitMiscValue("BoneDecayTime", &game.constants.boneDecayTime);
    InitMiscValue("DissipateTime", &game.constants.dissipateTime);
    InitMiscValue("StructureDecayTime", &game.constants.structureDecayTime);
    InitMiscValue("BulletDeathTime", &game.constants.bulletDeathTime);
    InitMiscValue("CloseEnoughRange", &game.constants.closeEnoughRange);
    InitMiscValue("Dawn", &game.constants.dawnTimeGameHours);
    InitMiscValue("Dusk", &game.constants.duskTimeGameHours);
    InitMiscValue("DayHours", &game.constants.gameDayHours);
    InitMiscValue("DayLength", &game.constants.gameDayLength);
    InitMiscValue("BuildingAngle", &game.constants.buildingAngle);
    InitMiscValue("RootAngle", &game.constants.rootAngle);
}

static void G_InitGame(void) {
    game_state.edicts = gi.MemAlloc(sizeof(edict_t) * MAX_ENTITIES);

    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;
    globals.max_clients = 16;

    game.max_clients = globals.max_clients;
    game.clients = gi.MemAlloc(game.max_clients * sizeof(gclient_t));
    game.config.theme = gi.ReadConfig("UI\\war3skins.txt");
    game.config.splats = gi.ReadSheet("Splats\\SplatData.slk");
    game.config.uberSplats = gi.ReadSheet("Splats\\UberSplatData.slk");
    game.config.abilities = gi.ReadSheet("Units\\AbilityData.slk");
    
    InitConstants();
    InitUnitData();
    InitAbilities();
}

static void G_ShutdownGame(void) {
    gi.MemFree(game_state.edicts);

    ShutdownUnitData();
}

static void G_RunFrame(void) {
    FOR_LOOP(i, globals.num_edicts) {
        G_RunEntity(&globals.edicts[i]);
    }
    G_SolveCollisions();
}

static LPCSTR G_GetThemeValue(LPCSTR filename) {
    LPCSTR skinned = NULL;
    if (!strstr(filename, "\\")) {
        skinned = gi.FindSheetCell(game.config.theme, "Default", filename);
    }
    return skinned ? skinned : filename;
}

playerState_t *G_GetPlayerByNumber(DWORD number) {
    FOR_LOOP(i, game.max_clients) {
        if (game.clients[i].ps.number == number) {
            return &game.clients[i].ps;
        }
    }
    return NULL;
}

void SetPoint(parser_t *parser, uiFrameDef_t *frame);

static void G_ClientBegin(edict_t *edict) {
    uiFrameDef_t *root = UI_Clear();
    uiFrameDef_t *consoleUI = FDF_ParseFile(root, "UI\\FrameDef\\UI\\ConsoleUI.fdf");
    uiFrameDef_t *resourceBar = FDF_ParseFile(consoleUI, "UI\\FrameDef\\UI\\ResourceBar.fdf");
    resourceBar->SetPoint.type = FRAMEPOINT_TOPRIGHT;
    resourceBar->SetPoint.target = FRAMEPOINT_TOPRIGHT;
    resourceBar->SetPoint.relativeTo = consoleUI->f.number;
    uiFrameDef_t *gold = UI_FindFrameByName(resourceBar, "ResourceBarGoldText");
    uiFrameDef_t *lumber = UI_FindFrameByName(resourceBar, "ResourceBarLumberText");
    uiFrameDef_t *supply = UI_FindFrameByName(resourceBar, "ResourceBarSupplyText");
    if (gold) gold->f.stat = STAT_GOLD;
    if (lumber) lumber->f.stat = STAT_LUMBER;
    if (supply) supply->f.stat = STAT_FOOD;
    SetPoint(NULL, resourceBar);
    UI_WriteLayout(edict, root, LAYER_CONSOLE);

    FILTER_EDICTS(ent, edict->client->ps.number == ent->s.player) {
        edict->client->ps.stats[STAT_FOOD_MADE] += UNIT_FOOD_MADE(ent->class_id);
        edict->client->ps.stats[STAT_FOOD_USED] += UNIT_FOOD_USED(ent->class_id);
    }
}

struct game_export *GetGameAPI(struct game_import *import) {
    memset(&game_state, 0, sizeof(struct game_state));
    gi = *import;
    globals.Init = G_InitGame;
    globals.Shutdown = G_ShutdownGame;
    globals.SpawnEntities = G_SpawnEntities;
    globals.RunFrame = G_RunFrame;
    globals.ClientCommand = G_ClientCommand;
    globals.ClientBegin = G_ClientBegin;
    globals.GetThemeValue = G_GetThemeValue;
    globals.edict_size = sizeof(struct edict_s);
    return &globals;
}
