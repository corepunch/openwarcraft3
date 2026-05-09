/*
 * ui_init.c — Server-side UI initialization.
 *
 * The UI system runs entirely on the server.  At startup UI_Init() loads all
 * Warcraft III FDF (Frame Definition File) assets from the MPQ archive via
 * UI_ParseFDF().  These files describe the frame hierarchy — types, textures,
 * fonts, and layout anchors — and are stored as frameDef_t templates.
 *
 * After parsing, UI_Init() instantiates the top-level frames (ConsoleUI,
 * ResourceBar, ToolTip, UpperButtonBar, CinematicPanel) and positions them
 * using the same anchor-point system as the original Warcraft III UI.
 *
 * When a client connects, G_ClientBegin() (g_main.c) calls UI_WriteLayout()
 * (ui_write.c) to serialize the complete frame tree and send it to the client
 * as an svc_layout message.  The client stores the raw blob and hands it to
 * the renderer without interpreting the frame hierarchy itself.
 *
 * Inline FDF strings
 * ------------------
 * A frame hierarchy can also be written as a C string literal following FDF
 * syntax and registered at runtime with UI_ParseFDF_Buffer().  This is useful
 * for self-contained components that have no corresponding .fdf file on disk.
 * The Init_ToolTip() function below is a worked example:
 *
 *   static LPCSTR tooltip =
 *       "Frame \"FRAME\" \"ToolTip\" {\n"
 *       "    Frame \"TOOLTIPTEXT\" \"ToolTipText\" {\n"
 *       "        SetAllPoints,\n"
 *       "        BackdropBackground  \"ToolTipBackground\",\n"
 *       "        BackdropCornerFlags \"UL|UR|BL|BR|T|L|B|R\",\n"
 *       "        BackdropCornerSize  0.008,\n"
 *       "        FrameFont \"MasterFont\", 0.010, \"\",\n"
 *       "        FontJustificationH JUSTIFYLEFT,\n"
 *       "        FontColor 1.0 1.0 1.0 1.0,\n"
 *       "    }\n"
 *       "}\n";
 *
 *   void Init_ToolTip(LPFRAMEDEF parent) {
 *       LPSTR buffer = strdup(tooltip);
 *       UI_ParseFDF_Buffer("Tooltip", buffer);  // registers the template
 *       free(buffer);
 *
 *       // retrieve the registered frame by name and position it
 *       UI_FRAME(ToolTip);
 *       UI_SetParent(ToolTip, parent);
 *       UI_SetSize(ToolTip, 0.22, 0.10);
 *       UI_SetPoint(ToolTip, FRAMEPOINT_BOTTOMRIGHT, NULL, FRAMEPOINT_BOTTOMRIGHT, 0.0, 0.16);
 *   }
 *
 * UI_ParseFDF_Buffer() parses the string in place (it is mutated during
 * parsing, so always pass a writable copy).  After the call, frames can be
 * retrieved by name with UI_FindFrame() or the UI_FRAME() convenience macro,
 * then positioned and parented as usual.
 */
#include "../g_local.h"

#define TOOLTIP_SIZE 0.2200, 0.1000
#define TOOLTIP_POSITION 0.0, 0.1600
#define QUEST_DIALOG_POSITION 0.0, -0.0230

static void Init_ResourceBar(LPFRAMEDEF ConsoleUI) {
    UI_FRAME(ResourceBarFrame);
    UI_FRAME(ResourceBarGoldText);
    UI_FRAME(ResourceBarLumberText);
    UI_FRAME(ResourceBarSupplyText);
    UI_FRAME(ResourceBarUpkeepText);
    
    ResourceBarGoldText->Stat = PLAYERSTATE_RESOURCE_GOLD;
    ResourceBarLumberText->Stat = PLAYERSTATE_RESOURCE_LUMBER;
    ResourceBarSupplyText->Stat = PLAYERSTATE_RESOURCE_FOOD_USED;
    ResourceBarUpkeepText->Text = UI_GetString("UPKEEP_NONE");
    
    UI_SetParent(ResourceBarFrame, ConsoleUI);
    UI_SetPoint(ResourceBarFrame, FRAMEPOINT_TOPRIGHT, ConsoleUI, FRAMEPOINT_TOPRIGHT, 0, 0);
}

static void Init_UpperButtonBar(LPFRAMEDEF ConsoleUI) {
    UI_FRAME(UpperButtonBarFrame);
    UI_SetParent(UpperButtonBarFrame, ConsoleUI);
    UI_SetPoint(UpperButtonBarFrame, FRAMEPOINT_TOPLEFT, ConsoleUI, FRAMEPOINT_TOPLEFT, 0, 0);
}

LPCSTR tooltip = \
"Frame \"FRAME\" \"ToolTip\" {\n"
"    Frame \"TOOLTIPTEXT\" \"ToolTipText\" {\n"
"        SetAllPoints,\n"
"        DecorateFileNames,\n"
"        BackdropTileBackground,\n"
"        BackdropBackground  \"ToolTipBackground\",\n"
"        BackdropCornerFlags \"UL|UR|BL|BR|T|L|B|R\",\n"
"        BackdropCornerSize  0.008,\n"
"        BackdropBackgroundSize  0.036,\n"
"        BackdropBackgroundInsets 0.0025 0.0025 0.0025 0.0025,\n"
"        BackdropEdgeFile  \"ToolTipBorder\",\n"
"        BackdropBlendAll,\n"
"        FrameFont \"MasterFont\", 0.010, \"\",\n"
"        FontJustificationH JUSTIFYLEFT,\n"
"        FontJustificationV JUSTIFYTOP,\n"
"        FontFlags \"FIXEDSIZE\",\n"
"        FontColor 1.0 1.0 1.0 1.0,\n"
"    }\n"
"}\n";

void Init_ToolTip(LPFRAMEDEF parent) {
    LPSTR buffer = strdup(tooltip);
    UI_ParseFDF_Buffer("Tooltip", buffer);
    free(buffer);
    
//    DWORD ToolTipBackground = UI_LoadTexture("ToolTipBackground", true);
//    DWORD ToolTipBorder = UI_LoadTexture("ToolTipBorder", true);
//    DWORD ToolTipGoldIcon = UI_LoadTexture("ToolTipGoldIcon", true);
//    DWORD ToolTipLumberIcon = UI_LoadTexture("ToolTipLumberIcon", true);
//    DWORD ToolTipStonesIcon = UI_LoadTexture("ToolTipStonesIcon", true);
//    DWORD ToolTipManaIcon = UI_LoadTexture("ToolTipManaIcon", true);
//    DWORD ToolTipSupplyIcon = UI_LoadTexture("ToolTipSupplyIcon", true);
    
    UI_FRAME(ToolTip);
    UI_SetParent(ToolTip, parent);
    UI_SetSize(ToolTip, TOOLTIP_SIZE);
    UI_SetPoint(ToolTip, FRAMEPOINT_BOTTOMRIGHT, NULL, FRAMEPOINT_BOTTOMRIGHT, TOOLTIP_POSITION);
}

void Init_CinematicPanel(void) {
    UI_FRAME(CinematicSpeakerText);
    UI_FRAME(CinematicDialogueText);
    CinematicSpeakerText->Stat = MAX_STATS + PLAYERTEXT_SPEAKER;
    CinematicDialogueText->Stat = MAX_STATS + PLAYERTEXT_DIALOGUE;
}

static void Init_MainMenu(void) {
    UI_FRAME(WarCraftIIILogo);
    UI_FRAME(SinglePlayerButton);
    UI_FRAME(BattleNetButton);
    UI_FRAME(LocalAreaNetworkButton);
    UI_FRAME(OptionsButton);
    UI_FRAME(CreditsButton);
    UI_FRAME(ExitButton);

    if (WarCraftIIILogo) {
        UI_SetSize(WarCraftIIILogo, 0.34, 0.23);
    }
    UI_SetOnClick(SinglePlayerButton, "menu singleplayer");
    UI_SetOnClick(BattleNetButton, "menu multiplayer");
    UI_SetOnClick(LocalAreaNetworkButton, "menu multiplayer");
    UI_SetOnClick(OptionsButton, "menu options");
    UI_SetOnClick(CreditsButton, "menu credits");
    UI_SetOnClick(ExitButton, "menu quit");
}

static void Init_SinglePlayerMenu(void) {
    UI_FRAME(CampaignButton);
    UI_FRAME(LoadSavedButton);
    UI_FRAME(ViewReplayButton);
    UI_FRAME(SkirmishButton);
    UI_FRAME(CancelButton);
    UI_SetOnClick(CampaignButton, "menu mapselect campaign");
    UI_SetOnClick(LoadSavedButton, "menu main");
    UI_SetOnClick(ViewReplayButton, "menu main");
    UI_SetOnClick(SkirmishButton, "menu mapselect skirmish");
    UI_SetOnClick(CancelButton, "menu main");
}

static void Init_MapSelectMenu(void) {
    UI_FRAME(BackButton);
    UI_FRAME(Mission13Button);
    UI_FRAME(Mission12Button);
    UI_FRAME(Mission11Button);
    UI_FRAME(Mission10Button);
    UI_FRAME(Mission9Button);
    UI_FRAME(Mission8Button);
    UI_FRAME(Mission7Button);
    UI_FRAME(Mission6Button);
    UI_FRAME(Mission5Button);
    UI_FRAME(Mission4Button);
    UI_FRAME(Mission3Button);
    UI_FRAME(Mission2Button);
    UI_FRAME(Mission1Button);
    UI_FRAME(Mission0Button);
    (void)Mission9Button;
    (void)Mission8Button;
    (void)Mission7Button;
    (void)Mission6Button;
    (void)Mission5Button;
    (void)Mission4Button;
    (void)Mission3Button;
    (void)Mission2Button;
    (void)Mission1Button;
    (void)Mission0Button;

    UI_SetOnClick(BackButton, "menu singleplayer");
    UI_SetOnClick(Mission13Button, "menu load Maps\\Campaign\\Human02.w3m");
    UI_SetOnClick(Mission12Button, "menu load Maps\\Campaign\\Human01.w3m");
    UI_SetOnClick(Mission11Button, "menu load Maps\\Campaign\\Orc01.w3m");
    UI_SetOnClick(Mission10Button, "menu load Maps\\Campaign\\Undead01.w3m");
}

static void Init_MultiplayerJoinMenu(void) {
    UI_FRAME(CreateButton);
    UI_FRAME(LoadButton);
    UI_FRAME(JoinButton);
    UI_FRAME(CancelButton);
    UI_SetOnClick(CreateButton, "menu multiplayer create");
    UI_SetOnClick(LoadButton, "menu multiplayer join");
    UI_SetOnClick(JoinButton, "menu multiplayer join");
    UI_SetOnClick(CancelButton, "menu main");
}

static void Init_MultiplayerCreateMenu(void) {
    UI_FRAME(MapInfoButton);
    UI_FRAME(AdvancedOptionsButton);
    UI_FRAME(PlayButton);
    UI_FRAME(CancelButton);
    UI_SetOnClick(MapInfoButton, "menu main");
    UI_SetOnClick(AdvancedOptionsButton, "menu main");
    UI_SetOnClick(PlayButton, "menu main");
    UI_SetOnClick(CancelButton, "menu multiplayer join");
}

/* Parse all FDF assets and build the initial UI frame hierarchy.
 * Must be called once at game startup before any client connects. */
void UI_Init(void) {
    fprintf(stderr, "UI_Init: begin\n");
    UI_ClearTemplates();

    UI_ParseFDF("UI\\FrameDef\\GlobalStrings.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\CinematicPanel.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\ConsoleUI.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\ResourceBar.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\SimpleInfoPanel.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\UpperButtonBar.fdf");
    UI_ParseFDF("UI\\FrameDef\\UI\\QuestDialog.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\StandardTemplates.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\MainMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\SinglePlayerMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\CampaignMenu.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\LocalMultiplayerJoin.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\LocalMultiplayerCreate.fdf");

    UI_FRAME(ConsoleUI);
    UI_SetAllPoints(ConsoleUI);
    
    UI_FRAME(QuestDialog);
    UI_SetPoint(QuestDialog, FRAMEPOINT_TOP, NULL, FRAMEPOINT_TOP, QUEST_DIALOG_POSITION);

    Init_ResourceBar(ConsoleUI);
    Init_ToolTip(ConsoleUI);
    Init_UpperButtonBar(ConsoleUI);
    Init_CinematicPanel();
    Init_MainMenu();
    Init_SinglePlayerMenu();
    Init_MapSelectMenu();
    Init_MultiplayerJoinMenu();
    Init_MultiplayerCreateMenu();
    fprintf(stderr, "UI_Init: complete\n");

//    UI_PrintClasses();
//    UI_FRAME(SimpleHeroLevelBar);
//    SimpleHeroLevelBar->f.size.width = INFO_PANEL_UNIT_DETAIL_WIDTH;
//    SimpleProgressIndicator->f.tex.index = UI_LoadTexture("SimpleProgressBarBorder", true);
//    UI_WriteLayout(edict, CinematicPanel, LAYER_CONSOLE);
}

void UI_ShowMainMenu(LPEDICT ent) {
    UI_FRAME(MainMenuFrame);
    UI_WriteLayout(ent, MainMenuFrame, LAYER_CONSOLE);
}

void UI_ShowSinglePlayerMenu(LPEDICT ent) {
    UI_FRAME(SinglePlayerMenu);
    UI_WriteLayout(ent, SinglePlayerMenu, LAYER_CONSOLE);
}

void UI_ShowMultiplayerMenu(LPEDICT ent) {
    UI_FRAME(LocalMultiplayerJoin);
    UI_WriteLayout(ent, LocalMultiplayerJoin, LAYER_CONSOLE);
}

void UI_ShowMapSelectMenu(LPEDICT ent, LPCSTR category) {
    (void)category;
    UI_FRAME(CampaignMenu);
    UI_WriteLayout(ent, CampaignMenu, LAYER_CONSOLE);
}
