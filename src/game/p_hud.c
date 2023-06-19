#include "g_local.h"

#define COMMAND_BUTTON_SIZE 0.039

static float cmd_columns[4] = { 0.2375, 0.2809, 0.3243, 0.3677 };
static float cmd_rows[3] = { 0.1136, 0.0697, 0.0258 };

edict_t *GetMainSelectedEntity(gclient_t *client) {
    FOR_LOOP(i, globals.num_edicts) {
        if (client_isentityselected(client, &globals.edicts[i])) {
            return &globals.edicts[i];
        }
    }
    return NULL;
}

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
    UI_SetPoint(button, FRAMEPOINT_CENTER, root, FRAMEPOINT_BOTTOM, cmd_columns[x], cmd_rows[y]);
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

void Get_Commands_f(edict_t *edict) {
    edict_t *ent = GetMainSelectedEntity(edict->client);
    uiFrameDef_t *layer = UI_Clear();
    if (ent) {
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
        
        LPCSTR abilities = UNIT_ABILITIES_NORMAL(ent->class_id);
        if (abilities) {
            PARSE_LIST(abilities, abil, gi.ParserGetToken) {
                LPCSTR code = gi.FindSheetCell(AbilityData, abil, "code");
                if (code && FindAbilityByClassname(code)) {
                    UI_AddAbilityButton(layer, code);
                } else {
                    gi.error("Can't find ability %s", abil);
                }
            }
        }
    }
    UI_WriteLayout(edict, layer, UI_COMMANDBAR);
    edict->client->menu.mouseup = NULL;
    edict->client->menu.cmdbutton = NULL;
}

void UI_AddCancelButton(edict_t *ent) {
    uiFrameDef_t *layer = UI_Clear();
    UI_AddAbilityButton(layer, STR_CmdCancel);
    UI_WriteLayout(ent, layer, UI_COMMANDBAR);
    ent->client->menu.cmdbutton = NULL;
}
