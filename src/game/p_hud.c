#include "g_local.h"

#define COMMAND_BUTTON_SIZE 0.039
#define COMMAND_BUTTON_START 1

static DWORD cmd_columns[4] = { UI_SCALE(0.2375), UI_SCALE(0.2809), UI_SCALE(0.3243), UI_SCALE(0.3677) };
static DWORD cmd_rows[3] = { UI_SCALE(0.1136), UI_SCALE(0.0697), UI_SCALE(0.0258) };

static LPCSTR GetBuildCommand(unitRace_t race) {
    switch (race) {
        case RACE_HUMAN: return STR_CmdBuildHuman;
        case RACE_ORC: return STR_CmdBuildOrc;
        case RACE_UNDEAD: return STR_CmdBuildUndead;
        case RACE_NIGHTELF: return STR_CmdBuildNightElf;
        default: return STR_CmdBuild;
    }
}

void Add_CommandButtonCoded(DWORD code, LPCSTR art, LPCSTR buttonpos) {
    DWORD x, y;
    sscanf(buttonpos, "%d,%d", &x, &y);
    uiFrameDef_t button;
    UI_InitFrame(&button, x + (y << 2) + COMMAND_BUTTON_START, FT_COMMANDBUTTON);
    button.f.tex.index = gi.ImageIndex(art);
    button.f.size.width = UI_SCALE(COMMAND_BUTTON_SIZE);
    button.f.size.height = UI_SCALE(COMMAND_BUTTON_SIZE);
    button.f.code = code;
    UI_SetPointByNumber(&button, FRAMEPOINT_CENTER, UI_PARENT, FRAMEPOINT_BOTTOM, cmd_columns[x], cmd_rows[y]);
    gi.WriteUIFrame(&button.f);
}

void UI_AddAbilityButton(LPCSTR ability) {
    LPCSTR art = FindConfigValue(ability, STR_ART);
    LPCSTR buttonpos = FindConfigValue(ability, STR_BUTTONPOS);
    DWORD ability_id = FindAbilityIndex(ability);
    if (!art || !buttonpos)
        return;
    if (!ability_id) {
        gi.error("Ability %s not found", ability);
        return;
    }
    Add_CommandButtonCoded(ability_id, art, buttonpos);
}

void ui_portrait(gclient_t *client) {
    edict_t *ent = G_GetMainSelectedEntity(client);
    uiFrameDef_t portrait;
    UI_InitFrame(&portrait, COMMAND_BUTTON_START, FT_PORTRAIT);
    portrait.f.tex.index = ent->s.model;
    portrait.f.size.width = 800;
    portrait.f.size.height = 800;
    UI_SetPointByNumber(&portrait, FRAMEPOINT_BOTTOMLEFT, UI_PARENT, FRAMEPOINT_BOTTOMLEFT, 2150, 300);
    gi.WriteUIFrame(&portrait.f);
}

void Get_Portrait_f(edict_t *edict) {
    UI_WriteLayout2(edict, ui_portrait, LAYER_PORTRAIT);
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

#define MULTISELECT_UI_NUMBER(i) (1 + i * 3)

static void HUD_MultiselectHealth(uiFrameDef_t *button, edict_t *ent, DWORD i) {
    float y = button->f.size.height * HP_BAR_SPACING_RATIO;
    uiFrameDef_t statusbar;
    UI_InitFrame(&statusbar, MULTISELECT_UI_NUMBER(i)+1, FT_SIMPLESTATUSBAR);
    statusbar.f.tex.index = gi.ImageIndex("SimpleHpBarConsole");
    statusbar.f.color = HP_BAR_COLOR;
    statusbar.f.size.width = button->f.size.width;
    statusbar.f.size.height = button->f.size.height * HP_BAR_HEIGHT_RATIO;
    statusbar.f.stat = i*2+0;
    UI_SetPointByNumber(&statusbar, FRAMEPOINT_TOPLEFT, button->f.number, FRAMEPOINT_BOTTOMLEFT, 0, -y);
    gi.WriteUIFrame(&statusbar.f);
}

static void HUD_MultiselectMana(uiFrameDef_t *button, edict_t *ent, DWORD i) {
    float y = button->f.size.height * (HP_BAR_HEIGHT_RATIO + HP_BAR_SPACING_RATIO * 2);
    uiFrameDef_t statusbar;
    UI_InitFrame(&statusbar, MULTISELECT_UI_NUMBER(i)+1, FT_SIMPLESTATUSBAR);
    statusbar.f.color = MANA_BAR_COLOR;
    statusbar.f.tex.index = gi.ImageIndex("SimpleManaBarConsole");
    statusbar.f.size.width = button->f.size.width;
    statusbar.f.size.height = button->f.size.height * HP_BAR_HEIGHT_RATIO;
    statusbar.f.stat = i*2+1;
    UI_SetPointByNumber(&statusbar, FRAMEPOINT_TOPLEFT, button->f.number, FRAMEPOINT_BOTTOMLEFT, 0, -y);
    gi.WriteUIFrame(&statusbar.f);
}

static void HUD_MultiselectIcon(edict_t *ent, DWORD i) {
    float x = UI_SCALE((i%MULTISELECT_COLUMNS)*MULTISELECT_GRID_X+MULTISELECT_X);
    float y = UI_SCALE(MULTISELECT_Y-(i/MULTISELECT_COLUMNS)*MULTISELECT_GRID_Y);
    float s = UI_SCALE(i == 0 ? MULTISELECT_SIZE2 : MULTISELECT_SIZE1);
    LPCSTR art = FindConfigValue(GetClassName(ent->class_id), STR_ART);
    uiFrameDef_t button;
    UI_InitFrame(&button, MULTISELECT_UI_NUMBER(i), FT_TEXTURE);
    button.f.tex.index = gi.ImageIndex(art);
    button.f.size.width = s;
    button.f.size.height = s;
    UI_SetPointByNumber(&button, FRAMEPOINT_CENTER, UI_PARENT, FRAMEPOINT_BOTTOMLEFT, x, y);
    gi.WriteUIFrame(&button.f);
    HUD_MultiselectHealth(&button, ent, i);
    HUD_MultiselectMana(&button, ent, i);
}

void ui_unit_commands(gclient_t *client) {
    edict_t *ent = G_GetMainSelectedEntity(client);
    if (!ent) return;
    LPCSTR abilities = UNIT_ABILITIES_NORMAL(ent->class_id);
    LPCSTR trains = UNIT_TRAINS(ent->class_id);
    if (UNIT_SPEED(ent->class_id) > 0) {
        UI_AddAbilityButton(STR_CmdMove);
        UI_AddAbilityButton(STR_CmdHoldPos);
        UI_AddAbilityButton(STR_CmdPatrol);
        UI_AddAbilityButton(STR_CmdStop);
    }
    if (UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(ent->class_id) != 0) {
        UI_AddAbilityButton(STR_CmdAttack);
    }
    if (UNIT_BUILDS(ent->class_id)) {
        UI_AddAbilityButton(GetBuildCommand(RACE_HUMAN));
    }
    if (abilities) {
        PARSE_LIST(abilities, abil, getNextSegment) {
            LPCSTR code = gi.FindSheetCell(game.config.abilities, abil, "code");
            if (code && FindAbilityByClassname(code)) {
                UI_AddAbilityButton(code);
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
            Add_CommandButtonCoded(*((DWORD *)unit), art, buttonpos);
        }
    }
}

void ui_unit_info(gclient_t *client) {
    DWORD selent = 0, statindex = 0;
    FOR_SELECTED_UNITS(client, ent) {
        HUD_MultiselectIcon(ent, selent++);
    }
}

void ui_cancel_only(gclient_t *client) {
    UI_AddAbilityButton(STR_CmdCancel);
}

void Get_Commands_f(edict_t *edict) {
    UI_WriteLayout2(edict, ui_unit_commands, LAYER_COMMANDBAR);
    UI_WriteLayout2(edict, ui_unit_info, LAYER_INFOPANEL);
    memset(&edict->client->menu, 0, sizeof(menu_t));
}

void UI_AddCancelButton(edict_t *ent) {
    UI_WriteLayout2(ent, ui_cancel_only, LAYER_COMMANDBAR);
    memset(&ent->client->menu, 0, sizeof(menu_t));
}
