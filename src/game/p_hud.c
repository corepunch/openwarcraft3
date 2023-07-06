#include "g_local.h"

#define COMMAND_BUTTON_SIZE 0.039

static float cmd_columns[4] = { 0.2375, 0.2809, 0.3243, 0.3677 };
static float cmd_rows[3] = { 0.1136, 0.0697, 0.0258 };

static LPCSTR GetBuildCommand(unitRace_t race) {
    switch (race) {
        case RACE_HUMAN: return STR_CmdBuildHuman;
        case RACE_ORC: return STR_CmdBuildOrc;
        case RACE_UNDEAD: return STR_CmdBuildUndead;
        case RACE_NIGHTELF: return STR_CmdBuildNightElf;
        default: return STR_CmdBuild;
    }
}

void Add_CommandButtonCoded(uiFrameDef_t *root, DWORD code, LPCSTR art, LPCSTR buttonpos) {
    DWORD x, y;
    sscanf(buttonpos, "%d,%d", &x, &y);
    uiFrameDef_t *button = UI_Spawn(FT_COMMANDBUTTON, root);
    button->f.tex.coord[1] = 0xff;
    button->f.tex.coord[3] = 0xff;
    button->f.tex.index = gi.ImageIndex(art);
    button->f.size.width = UI_SCALE(COMMAND_BUTTON_SIZE);
    button->f.size.height = UI_SCALE(COMMAND_BUTTON_SIZE);
    button->f.code = code;
    UI_SetPoint(button, FRAMEPOINT_CENTER, root, FRAMEPOINT_BOTTOM, UI_SCALE(cmd_columns[x]), UI_SCALE(cmd_rows[y]));
}

void UI_AddAbilityButton(uiFrameDef_t *root, LPCSTR ability) {
    LPCSTR art = FindConfigValue(ability, STR_ART);
    LPCSTR buttonpos = FindConfigValue(ability, STR_BUTTONPOS);
    DWORD ability_id = FindAbilityIndex(ability);
    if (!art || !buttonpos)
        return;
    if (!ability_id) {
        gi.error("Ability %s not found", ability);
        return;
    }
    Add_CommandButtonCoded(root, ability_id, art, buttonpos);
}

void Get_Portrait_f(edict_t *edict) {
    edict_t *ent = G_GetMainSelectedEntity(edict->client);
    uiFrameDef_t *layer = UI_EmptyScreen();
    uiFrameDef_t *portrait = UI_Spawn(FT_PORTRAIT, layer);
    portrait->f.tex.index = ent->s.model;
    portrait->f.size.width = 800;
    portrait->f.size.height = 800;
    UI_SetPoint(portrait, FRAMEPOINT_BOTTOMLEFT, layer, FRAMEPOINT_BOTTOMLEFT, 2150, 300);
    UI_WriteLayout(edict, layer, LAYER_PORTRAIT);
}

#define MULTISELECT_X 0.326
#define MULTISELECT_Y 0.082
#define MULTISELECT_GRID_X 0.031
#define MULTISELECT_GRID_Y 0.05
#define MULTISELECT_SIZE1 0.024
#define MULTISELECT_SIZE2 0.032
#define MULTISELECT_COLUMNS 6
#define HP_BAR_HEIGHT_RATIO 0.175f
#define HP_BAR_SPACING_RATIO 0.02f
#define HP_BAR_COLOR MAKE(COLOR32,0,255,0,255)
#define MANA_BAR_COLOR MAKE(COLOR32,0,128,255,255)

static uiFrameDef_t *HUD_MultiselectIcon(uiFrameDef_t *layer, edict_t *ent, DWORD i) {
    float x = UI_SCALE((i%MULTISELECT_COLUMNS)*MULTISELECT_GRID_X+MULTISELECT_X);
    float y = UI_SCALE(MULTISELECT_Y-(i/MULTISELECT_COLUMNS)*MULTISELECT_GRID_Y);
    float s = UI_SCALE(i == 0 ? MULTISELECT_SIZE2 : MULTISELECT_SIZE1);
    LPCSTR art = FindConfigValue(GetClassName(ent->class_id), STR_ART);
    uiFrameDef_t *button = UI_Spawn(FT_TEXTURE, layer);
    button->f.tex.index = gi.ImageIndex(art);
    button->f.size.width = s;
    button->f.size.height = s;
    UI_SetPoint(button, FRAMEPOINT_CENTER, layer, FRAMEPOINT_BOTTOMLEFT, x, y);
    return button;
}

static uiFrameDef_t *HUD_MultiselectHealth(uiFrameDef_t *button, edict_t *ent, DWORD index) {
    float y = button->f.size.height * HP_BAR_SPACING_RATIO;
    uiFrameDef_t *statusbar = UI_Spawn(FT_SIMPLESTATUSBAR, button);
    statusbar->f.tex.index = gi.ImageIndex("SimpleHpBarConsole");
    statusbar->f.color = HP_BAR_COLOR;
    statusbar->f.size.width = button->f.size.width;
    statusbar->f.size.height = button->f.size.height * HP_BAR_HEIGHT_RATIO;
    statusbar->f.stat = index;
    UI_SetPoint(statusbar, FRAMEPOINT_TOPLEFT, button, FRAMEPOINT_BOTTOMLEFT, 0, -y);
    return statusbar;
}

static uiFrameDef_t *HUD_MultiselectMana(uiFrameDef_t *button, edict_t *ent, DWORD index) {
    float y = button->f.size.height * (HP_BAR_HEIGHT_RATIO + HP_BAR_SPACING_RATIO * 2);
    uiFrameDef_t *statusbar = UI_Spawn(FT_SIMPLESTATUSBAR, button);
    statusbar->f.color = MANA_BAR_COLOR;
    statusbar->f.tex.index = gi.ImageIndex("SimpleManaBarConsole");
    statusbar->f.size.width = button->f.size.width;
    statusbar->f.size.height = button->f.size.height * HP_BAR_HEIGHT_RATIO;
    statusbar->f.stat = index;
    UI_SetPoint(statusbar, FRAMEPOINT_TOPLEFT, button, FRAMEPOINT_BOTTOMLEFT, 0, -y);
    return statusbar;
}

void Get_Commands_f(edict_t *edict) {
    edict_t *ent = G_GetMainSelectedEntity(edict->client);
    uiFrameDef_t *layer = UI_EmptyScreen();
    if (ent) {
        DWORD selent = 0, statindex = 0;
        FOR_SELECTED_UNITS(edict->client, ent) {
//        for (; selent < 12;) {
            uiFrameDef_t *button = HUD_MultiselectIcon(layer, ent, selent);
            /*uiFrameDef_t *hpbar = */HUD_MultiselectHealth(button, ent, statindex++);
            /*uiFrameDef_t *manabar = */HUD_MultiselectMana(button, ent, statindex++);
            selent++;
        }
        
        LPCSTR abilities = UNIT_ABILITIES_NORMAL(ent->class_id);
        LPCSTR trains = UNIT_TRAINS(ent->class_id);
        if (UNIT_SPEED(ent->class_id) > 0) {
            UI_AddAbilityButton(layer, STR_CmdMove);
            UI_AddAbilityButton(layer, STR_CmdHoldPos);
            UI_AddAbilityButton(layer, STR_CmdPatrol);
            UI_AddAbilityButton(layer, STR_CmdStop);
        }
        if (UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(ent->class_id) != 0) {
            UI_AddAbilityButton(layer, STR_CmdAttack);
        }
        if (UNIT_BUILDS(ent->class_id)) {
            UI_AddAbilityButton(layer, GetBuildCommand(RACE_HUMAN));
        }
        if (abilities) {
            PARSE_LIST(abilities, abil, getNextSegment) {
                LPCSTR code = gi.FindSheetCell(game.config.abilities, abil, "code");
                if (code && FindAbilityByClassname(code)) {
                    UI_AddAbilityButton(layer, code);
                } else {
                    gi.error("Ability not implemented: %s - %s", abil, gi.FindSheetCell(game.config.abilities, abil, "comments"));
                }
            }
        }
        if (trains) {
            PARSE_LIST(trains, unit, getNextSegment) {
                LPCSTR art = FindConfigValue(unit, STR_ART);
                LPCSTR buttonpos = FindConfigValue(unit, STR_BUTTONPOS);
                if (!art || !buttonpos)
                    return;
                Add_CommandButtonCoded(layer, *((DWORD *)unit), art, buttonpos);
            }
        }
    }
    UI_WriteLayout(edict, layer, LAYER_COMMANDBAR);
    memset(&edict->client->menu, 0, sizeof(menu_t));
}

void UI_AddCancelButton(edict_t *ent) {
    uiFrameDef_t *layer = UI_EmptyScreen();
    UI_AddAbilityButton(layer, STR_CmdCancel);
    UI_WriteLayout(ent, layer, LAYER_COMMANDBAR);
    memset(&ent->client->menu, 0, sizeof(menu_t));
}
