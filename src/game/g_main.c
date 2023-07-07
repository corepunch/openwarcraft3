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
    FOR_LOOP(i, game.max_clients) {
        DWORD barnum = 0;
        FOR_SELECTED_UNITS(game.clients+i, ent) {
            game.clients[i].ps.unit_stats[barnum++] = ent->s.stats[ENT_HEALTH];
            game.clients[i].ps.unit_stats[barnum++] = ent->s.stats[ENT_MANA];
        }
    }
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

#define INFO_PANEL_UNIT_DETAIL_WIDTH UI_SCALE(0.180f)
#define INFO_PANEL_UNIT_DETAIL_HEIGHT UI_SCALE(0.120f)

static void Init_SimpleProgressIndicator(void) {
    UI_FRAME(SimpleProgressIndicator);
    SimpleProgressIndicator->f.size.width = INFO_PANEL_UNIT_DETAIL_WIDTH;
    SimpleProgressIndicator->f.tex.index = UI_LoadTexture("SimpleXpBarConsole", true);
    SimpleProgressIndicator->f.tex.index2 = UI_LoadTexture("SimpleXpBarBorder", true);
    SimpleProgressIndicator->f.color = MAKE(COLOR32,160,0,160,255);
}

static void Init_ResourceBar(uiFrameDef_t *ConsoleUI) {
    UI_FRAME(ResourceBarFrame);
    UI_FRAME(ResourceBarGoldText);
    UI_FRAME(ResourceBarLumberText);
    UI_FRAME(ResourceBarSupplyText);
    
    if (ResourceBarGoldText) ResourceBarGoldText->f.stat = STAT_GOLD;
    if (ResourceBarLumberText) ResourceBarLumberText->f.stat = STAT_LUMBER;
    if (ResourceBarSupplyText) ResourceBarSupplyText->f.stat = STAT_FOOD;
    
    UI_SetParent(ResourceBarFrame, ConsoleUI);
    UI_SetPoint(ResourceBarFrame, FRAMEPOINT_TOPRIGHT, ConsoleUI, FRAMEPOINT_TOPRIGHT, 0, 0);
}

static void Init_SimpleInfoPanelIconDamage(uiFrameDef_t *parent) {
    UI_FRAME(SimpleInfoPanelIconDamage);
    UI_CHILD_FRAME(InfoPanelIconBackdrop, SimpleInfoPanelIconDamage);
    UI_CHILD_FRAME(InfoPanelIconLevel, SimpleInfoPanelIconDamage);
    UI_CHILD_FRAME(InfoPanelIconValue, SimpleInfoPanelIconDamage);
    UI_SetParent(SimpleInfoPanelIconDamage, parent);
    UI_SetPoint(SimpleInfoPanelIconDamage, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, UI_SCALE(-0.040));
    UI_SetText(InfoPanelIconLevel, "5");
    UI_SetText(InfoPanelIconValue, "3 - 7");
    UI_SetTexture(InfoPanelIconBackdrop, "InfoPanelIconDamagePierce", true);
}

static void Init_SimpleInfoPanelIconArmor(uiFrameDef_t *parent) {
    UI_FRAME(SimpleInfoPanelIconArmor);
    UI_CHILD_FRAME(InfoPanelIconBackdrop, SimpleInfoPanelIconArmor);
    UI_CHILD_FRAME(InfoPanelIconLevel, SimpleInfoPanelIconArmor);
    UI_CHILD_FRAME(InfoPanelIconValue, SimpleInfoPanelIconArmor);
    UI_SetParent(SimpleInfoPanelIconArmor, parent);
    UI_SetPoint(SimpleInfoPanelIconArmor, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, UI_SCALE(-0.0745));
    UI_SetText(InfoPanelIconLevel, "5");
    UI_SetText(InfoPanelIconValue, "3 - 7");
    UI_SetTexture(InfoPanelIconBackdrop, "InfoPanelIconArmorLarge", true);
}

static void Init_SimpleInfoPanelIconHero(uiFrameDef_t *parent) {
    UI_FRAME(SimpleInfoPanelIconHero);
    UI_CHILD_FRAME(InfoPanelIconHeroIcon, SimpleInfoPanelIconHero);
    UI_SetParent(SimpleInfoPanelIconHero, parent);
    UI_SetPoint(SimpleInfoPanelIconHero, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, UI_SCALE(0.1), UI_SCALE(-0.037));
    UI_SetTexture(InfoPanelIconHeroIcon, "InfoPanelIconHeroIconSTR", true);
}

static void Init_SimpleInfoPanelUnitDetail(uiFrameDef_t *ConsoleUI, uiFrameDef_t *bottom) {
    UI_FRAME(SimpleInfoPanelUnitDetail);
    UI_CHILD_FRAME(SimpleNameValue, SimpleInfoPanelUnitDetail);
    UI_CHILD_FRAME(SimpleClassValue, SimpleInfoPanelUnitDetail);
    
    UI_SetParent(SimpleInfoPanelUnitDetail, bottom);
    UI_SetAllPoints(SimpleInfoPanelUnitDetail);
    UI_SetText(SimpleNameValue, "Thrall");
    UI_SetText(SimpleClassValue, "Level 1 Far Seer");

    Init_SimpleInfoPanelIconDamage(SimpleInfoPanelUnitDetail);
    Init_SimpleInfoPanelIconArmor(SimpleInfoPanelUnitDetail);
    Init_SimpleInfoPanelIconHero(SimpleInfoPanelUnitDetail);
}

static uiFrameDef_t *InitBottomPanel(uiFrameDef_t *ConsoleUI) {
    uiFrameDef_t *bottom = UI_Spawn(FT_SIMPLEFRAME, ConsoleUI);
    UI_SetSize(bottom, INFO_PANEL_UNIT_DETAIL_WIDTH, INFO_PANEL_UNIT_DETAIL_HEIGHT);
    UI_SetPoint(bottom, FRAMEPOINT_BOTTOM, ConsoleUI, FRAMEPOINT_BOTTOM, 0, 0);
    return bottom;
}

static void G_ClientBegin(edict_t *edict) {
    UI_Clear();
    
    UI_ParseFDF("UI\\FrameDef\\GlobalStrings.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\ConsoleUI.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\ResourceBar.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\SimpleInfoPanel.fdf");

    UI_FRAME(ConsoleUI);
    UI_SetAllPoints(ConsoleUI);

    uiFrameDef_t *bottom = InitBottomPanel(ConsoleUI);

    Init_ResourceBar(ConsoleUI);
    Init_SimpleInfoPanelUnitDetail(ConsoleUI, bottom);
    Init_SimpleProgressIndicator();
    
    UI_PrintClasses();
    
//    UI_FRAME(SimpleHeroLevelBar);
//    SimpleHeroLevelBar->f.size.width = INFO_PANEL_UNIT_DETAIL_WIDTH;
//    SimpleProgressIndicator->f.tex.index = UI_LoadTexture("SimpleProgressBarBorder", true);
    
    UI_WriteLayout(edict, ConsoleUI, LAYER_CONSOLE);

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
