#include "ui_local.h"

DWORD x_grid[] = { 182, 139, 96, 52 };
DWORD y_grid[] = { 132, 89, 45 };

static float columns[COMMANDBAR_COLUMNS] = { 0.6375, 0.6809, 0.7243, 0.7677 };
static float rows[COMMANDBAR_ROWS] = { 0.1136, 0.0697, 0.0258 };

static uiFrameDef_t *commandBar = NULL;

entityState_t const *UI_MainSelectedEntity(void) {
    return ui.selectedEntities[0];
}

void CommandBar_Clear(void) {
    FOR_LOOP(index, NUM_COMMANDBAR_CELLS) {
        UI_GetCommandButton(index)->Texture = NULL;
        UI_GetCommandButton(index)->click = NULL;
    }
}

static void CommandBar_AddButton(LPCSTR class_name, void (*callback)(uiCommandButton_t const *)) {
    BYTE buttonpos = UI_CommandButtonPosition(class_name);
    if (buttonpos >= NUM_COMMANDBAR_CELLS) {
        imp.error("No BUTTONPOS for %s", class_name);
        return;
    }
    uiCommandButton_t *cmd = UI_GetCommandButton(buttonpos);
    strcpy(cmd->Command, class_name);
    cmd->Texture = UI_GetItemTexture(class_name);
    cmd->click = callback;
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

static void CommandBar_Abilities() {
    entityState_t const *ent = UI_MainSelectedEntity();
    if (ent->flags & EF_CAN_MOVE) {
        CommandBar_AddButton(STR_CmdMove, CommandButton_SelectTarget);
        CommandBar_AddButton(STR_CmdHoldPos, CommandButton_SelectTarget);
        CommandBar_AddButton(STR_CmdPatrol, CommandButton_SelectTarget);
        CommandBar_AddButton(STR_CmdStop, CommandButton_Stop);
    }
    if (ent->flags & EF_CAN_ATTACK) {
        CommandBar_AddButton(STR_CmdAttack, CommandButton_SelectTarget);
    }
    if (UI_FindConfigValue(UI_GetClassName(ent->class_id), STR_BUILDS)) {
        CommandBar_AddButton(GetBuildCommand(UNIT_RACE(ent)), CommandButton_Build);
    }
    PARSE_LIST(ui.selected.abilities, ability, imp.ParserGetToken) {
        CommandBar_AddButton(ability, CommandButton_SelectTarget);
    }
}

static void CommandBar_Cancel() {
    uiCommandButton_t *cmd = UI_GetCommandButton(NUM_COMMANDBAR_CELLS-1);
    cmd->Texture = ui.btnCancel;
    cmd->click = CommandButton_Cancel;
}

static void CommandBar_Build() {
    entityState_t const *ent = UI_MainSelectedEntity();
    LPCSTR builds = UI_FindConfigValue(UI_GetClassName(ent->class_id), STR_BUILDS);
    PARSE_LIST(builds, item, imp.ParserGetToken) {
        BYTE buttonpos = UI_CommandButtonPosition(item);
        if (buttonpos >= NUM_COMMANDBAR_CELLS)
            continue;
        uiCommandButton_t *cmd = UI_GetCommandButton(buttonpos);
        strcpy(cmd->Command, item);
        cmd->Texture = UI_GetItemTexture(item);
        cmd->click = CommandButton_SelectBuildLocation;
    }
    CommandBar_Cancel();
}

void CommandBar_SetMode(uiCommandBarMode_t mode) {
    CommandBar_Clear();

    ui.commandBarMode = mode;

    if (UI_MainSelectedEntity() == NULL) {
        return;
    }
    
    switch (mode) {
        case CBAR_SHOW_ABILITIES:
            CommandBar_Abilities();
            break;
        case CBAR_SELECT_TARGET:
            CommandBar_Cancel();
            break;
        case CBAR_SHOW_BUILDS:
            CommandBar_Build();
            break;
        case CBAR_SELECT_BUILD_LOCATION:
            CommandBar_Cancel();
            break;
    }
}


uiFrameDef_t *UI_MakeCommandBar(uiFrameDef_t *parent) {
    commandBar = imp.MemAlloc(sizeof(uiFrameDef_t));
    commandBar->Width = COMMANDBAR_FRAME_WIDTH;
    commandBar->Height = COMMANDBAR_FRAME_HEIGHT;
    strcpy(commandBar->Name, "CommandBarFrame");
    FOR_LOOP(index, NUM_COMMANDBAR_CELLS) {
        uiFrameDef_t *button = UI_MakeCommandButton();
//        uiCommandButton_t *commandButton = button->typedata;
        float x = columns[index % COMMANDBAR_COLUMNS];
        float y = rows[index / COMMANDBAR_COLUMNS];
        SetFramePoint(button, FRAMEPOINT_CENTER, parent, FRAMEPOINT_BOTTOMLEFT, x, y);
        button->Width = COMMAND_BUTTON_SIZE;
        button->Height = COMMAND_BUTTON_SIZE;
        strcpy(button->Name, "CommandButton");
        UI_FrameAddChild(commandBar, button);
    }
    return commandBar;
}

uiCommandButton_t *UI_GetCommandButton(DWORD index) {
    FOR_EACH_LIST(uiFrameDef_t, button, commandBar->children) {
        uiCommandButton_t *cmdbutton = button->typedata;
        if (index == 0) {
            return cmdbutton;
        } else {
            index--;
        }
    }
    return NULL;
}
