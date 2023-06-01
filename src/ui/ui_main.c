#include "ui_local.h"

uiPortrait_t portrait;
uiImport_t imp;
uiLocal_t ui;

void UI_Init(void) {
    UI_InitConfigFiles();
    
    ui.theme = imp.ParseConfig("UI\\war3skins.txt");
    ui.simpleConsole = FDF_ParseFile("UI\\FrameDef\\UI\\ConsoleUI.fdf");
    UI_FrameAddChild(ui.simpleConsole, UI_MakeCommandBar(ui.simpleConsole));
    memset(&portrait, 0, sizeof(uiPortrait_t));
    
    ui.btnCancel = imp.LoadTexture(imp.FindConfigValue(ui.theme, "Default", "CommandCancel"));
}

void UI_Shutdown(void) {
    UI_ShutdownConfigFiles();
}

void UI_DrawPortrait(uiPortrait_t *pt) {
    entityState_t const *selected = imp.GetSelectedEntity();
    RECT viewport = { 215/800.0, 30/600.0, 80/800.0, 80/600.0 };
    if (selected != pt->current) {
        if (selected) {
            LPCSTR string = imp.GetConfigString(selected->model + CS_MODELS);
            PATHSTR buffer = { 0 };
            PATHSTR path = { 0 };
            memcpy(buffer, string, strstr(string, ".mdx") - string);
            sprintf(path, "%s_Portrait.mdx", buffer);
            pt->portraitModel = imp.LoadModel(path);
        }
        pt->current = selected;
    }
    if (pt->portraitModel) {
        imp.DrawPortrait(pt->portraitModel, &viewport);
    }
}

void UI_Draw(void) {
    ui.simpleConsole->Width = 0.8;
    ui.simpleConsole->Height = 0.6;
    UI_DrawPortrait(&portrait);
    UI_UpdateFrame(ui.simpleConsole);
    UI_DrawFrame(ui.simpleConsole);
}

void UI_HandleEvent(entityState_t const *ent, uiEvent_t event) {
    switch (event) {
        case UI_REFRESH_CONSOLE:
            ui.selectedEntities[0] = ent;
            CommandBar_SetMode(CBAR_SHOW_ABILITIES);
            break;
    }
}

uiExport_t UI_GetAPI(uiImport_t import) {
    imp = import;
    return (uiExport_t) {
        .Init = UI_Init,
        .Shutdown = UI_Shutdown,
        .Draw = UI_Draw,
        .ServerCommand = UI_ServerCommand,
        .HandleEvent = UI_HandleEvent,
        .MouseEvent = UI_MouseEvent,
    };
}
