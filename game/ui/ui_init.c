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
    UI_FRAME(MainMenuFrame);
    UI_FRAME(TopLeftPanel);
    UI_FRAME(TopRightPanel);
    UI_FRAME(RealmSelect);
    UI_FRAME(ControlLayer);
    UI_FRAME(RealmButton);
    UI_FRAME(SinglePlayerButton);
    UI_FRAME(BattleNetButton);
    UI_FRAME(LocalAreaNetworkButton);
    UI_FRAME(OptionsButton);
    UI_FRAME(CreditsButton);
    UI_FRAME(ExitButton);
    UI_FRAME(WarCraftIIILogo);
    LPFRAMEDEF RealmSelectOKButton = RealmSelect ? UI_FindChildFrame(RealmSelect, "RealmSelectOKButton") : NULL;
    LPFRAMEDEF RealmSelectCancelButton = RealmSelect ? UI_FindChildFrame(RealmSelect, "RealmSelectCancelButton") : NULL;

    if (WarCraftIIILogo) {
        UI_SetPoint(WarCraftIIILogo, FRAMEPOINT_TOPLEFT, NULL, FRAMEPOINT_TOPLEFT, 0.13f, -0.08f);
    }

    if (TopLeftPanel) {
        UI_SetHidden(TopLeftPanel, false);
    }
    if (TopRightPanel) {
        UI_SetHidden(TopRightPanel, false);
    }
    if (RealmSelect) {
        UI_SetHidden(RealmSelect, true);
    }
    if (ControlLayer) {
        UI_SetHidden(ControlLayer, false);
    }
    if (TopLeftPanel) {
        UI_SetParent(TopLeftPanel, MainMenuFrame);
    }
    if (TopRightPanel) {
        UI_SetParent(TopRightPanel, MainMenuFrame);
    }
    if (RealmSelect) {
        UI_SetParent(RealmSelect, MainMenuFrame);
    }
    if (ControlLayer) {
        UI_SetParent(ControlLayer, MainMenuFrame);
    }
    if (WarCraftIIILogo && ControlLayer) {
        UI_SetParent(WarCraftIIILogo, ControlLayer);
    }
#ifndef OW3_NO_NETWORK
    UI_SetOnClick(RealmButton, "menu /realm-select");
#else
    (void)RealmButton;
#endif
    UI_SetOnClick(RealmSelectOKButton, "menu /main");
    UI_SetOnClick(RealmSelectCancelButton, "menu /main");
    UI_SetOnClick(SinglePlayerButton, "menu /single-player");
    UI_SetOnClick(BattleNetButton, "menu /lan");
    UI_SetOnClick(LocalAreaNetworkButton, "menu /lan");
    UI_SetOnClick(OptionsButton, "menu /options");
    UI_SetOnClick(CreditsButton, "menu /credits");
    UI_SetOnClick(ExitButton, "menu /quit");
}

static void Init_SinglePlayerMenu(void) {
    UI_FRAME(SinglePlayerMenu);
    UI_FRAME(ProfilePanel);
    LPFRAMEDEF CampaignButton = UI_FindChildFrame(SinglePlayerMenu, "CampaignButton");
    LPFRAMEDEF LoadSavedButton = UI_FindChildFrame(SinglePlayerMenu, "LoadSavedButton");
    LPFRAMEDEF ViewReplayButton = UI_FindChildFrame(SinglePlayerMenu, "ViewReplayButton");
    LPFRAMEDEF SkirmishButton = UI_FindChildFrame(SinglePlayerMenu, "SkirmishButton");
    LPFRAMEDEF CancelButton = UI_FindChildFrame(SinglePlayerMenu, "CancelButton");
    if (ProfilePanel) {
        UI_SetHidden(ProfilePanel, true);
    }
    UI_SetOnClick(CampaignButton, "menu /single-player/campaign");
    UI_SetOnClick(LoadSavedButton, "menu /single-player/load-saved");
    UI_SetOnClick(ViewReplayButton, "menu /single-player/replays");
    UI_SetOnClick(SkirmishButton, "menu /single-player/skirmish");
    UI_SetOnClick(CancelButton, "menu /main");
}

static LPCSTR UI_ResolveThemeModel(LPCSTR key) {
    LPCSTR model = Theme_String(key, "Default");
    if (!model || !*model || !strcmp(model, key)) {
        return NULL;
    }
    return model;
}

static void UI_WriteMenuGlueBackground(LPCSTR model_name) {
    FRAMEDEF center;
    LPCSTR center_model = model_name;
    if (center_model && *center_model) {
        LPCSTR resolved = UI_ResolveThemeModel(center_model);
        if (resolved) {
            center_model = resolved;
        }
    }
    if (!center_model || !*center_model) {
        center_model = UI_ResolveThemeModel("GlueSpriteLayerCenter");
    }
    if (!center_model) {
        center_model = UI_ResolveThemeModel("GlueSpriteLayerBackground");
    }
    if (!center_model || !*center_model) {
        return;
    }
    UI_InitFrame(&center, FT_PORTRAIT);
    strcpy(center.Name, "GlueSpriteLayerBackground");
    UI_SetAllPoints(&center);
    center.Portrait.model = gi.ModelIndex(center_model);
#ifdef DIAG_OUTPUT
    DIAGF("UI_WriteMainMenuGlueBackground: name=%s model=%s modelIndex=%u\n",
          center.Name,
          center_model,
          (unsigned)center.Portrait.model);
#endif
    UI_WriteFrame(&center);
}

static void UI_WriteMainMenuGlueBackground(void) {
    UI_WriteMenuGlueBackground(NULL);
}

static LPCSTR UI_MenuPanelAnimationForMapCategory(LPCSTR category) {
    if (category && !strcmp(category, "skirmish")) {
        return "SinglePlayerSkirmish";
    }
    if (!category || !strcmp(category, "campaign")) {
        return "Death";
    }
    return "MainCancelPanel";
}

static void UI_BuildMenuPanelAnimation(LPEDICT ent, LPCSTR target, LPSTR out, size_t out_size) {
    (void)ent;
    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!target || !*target) {
        return;
    }
    if (strchr(target, ' ') || !strcmp(target, "Death") || !strcmp(target, "Birth") || !strcmp(target, "Stand")) {
        snprintf(out, out_size, "%s", target);
    } else {
        snprintf(out, out_size, "%s Stand", target);
    }
}

static void UI_WriteMainMenuGlueTopLayers(LPCSTR animation) {
    FRAMEDEF top_right;
    FRAMEDEF top_left;
    LPCSTR top_right_model = UI_ResolveThemeModel("GlueSpriteLayerTopRight");
    LPCSTR top_left_model = UI_ResolveThemeModel("GlueSpriteLayerTopLeft");

    if (top_right_model && *top_right_model) {
        UI_InitFrame(&top_right, FT_SPRITE);
        strcpy(top_right.Name, "GlueSpriteLayerTopRight");
        UI_SetAllPoints(&top_right);
        top_right.Portrait.model = gi.ModelIndex(top_right_model);
        if (animation && *animation) {
            UI_SetText(&top_right, "%s", animation);
        }
#ifdef DIAG_OUTPUT
        DIAGF("UI_WriteMainMenuGlueTopLayers: name=%s model=%s modelIndex=%u\n",
              top_right.Name,
              top_right_model,
              (unsigned)top_right.Portrait.model);
#endif
        UI_WriteFrame(&top_right);
    }

    if (top_left_model && *top_left_model) {
        UI_InitFrame(&top_left, FT_SPRITE);
        strcpy(top_left.Name, "GlueSpriteLayerTopLeft");
        UI_SetAllPoints(&top_left);
        top_left.Portrait.model = gi.ModelIndex(top_left_model);
        if (animation && *animation) {
            UI_SetText(&top_left, "%s", animation);
        }
#ifdef DIAG_OUTPUT
        DIAGF("UI_WriteMainMenuGlueTopLayers: name=%s model=%s modelIndex=%u\n",
              top_left.Name,
              top_left_model,
              (unsigned)top_left.Portrait.model);
#endif
        UI_WriteFrame(&top_left);
    }
}

static void UI_PrepareMenuFrameState(LPCFRAMEDEF root, BOOL realm_select_visible) {
    UI_FRAME(MainMenuFrame);
    UI_FRAME(ControlLayer);
    UI_FRAME(RealmSelect);

    if (!MainMenuFrame) {
        return;
    }
    if (ControlLayer) {
        UI_SetHidden(ControlLayer, root != MainMenuFrame);
    }
    if (RealmSelect) {
        UI_SetHidden(RealmSelect, !realm_select_visible);
    }
}

static void UI_WriteMenuWithMainFrameAnimationAndBackground(LPEDICT ent,
                                                           LPCFRAMEDEF root,
                                                           LPCSTR panel_animation,
                                                           LPCSTR background_model) {
    UI_FRAME(MainMenuFrame);
    UINAME animation;

    if (!MainMenuFrame || !root) {
        return;
    }
    UI_BuildMenuPanelAnimation(ent, panel_animation, animation, sizeof(animation));

    UI_WriteStart(LAYER_BACKGROUND);
    if (background_model && *background_model) {
        UI_WriteMenuGlueBackground(background_model);
    } else {
        UI_WriteMainMenuGlueBackground();
    }
    gi.WriteLong(0); // end of list
    gi.unicast(ent);

    UI_WriteStart(LAYER_CONSOLE);
    UI_WriteFrameWithChildren(MainMenuFrame, NULL);
    if (root != MainMenuFrame) {
        UI_WriteFrameWithChildren(root, NULL);
    }
    UI_WriteMainMenuGlueTopLayers(animation);
    gi.WriteLong(0); // end of list
    gi.unicast(ent);
}

static void UI_WriteMenuWithMainFrameAnimation(LPEDICT ent, LPCFRAMEDEF root, LPCSTR panel_animation) {
    UI_WriteMenuWithMainFrameAnimationAndBackground(ent, root, panel_animation, NULL);
}

static void Init_MapSelectMenu(void) {
    UI_FRAME(SlidingDoors);
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

    if (SlidingDoors) {
        UI_SetHidden(SlidingDoors, true);
    }
    UI_SetOnClick(BackButton, "menu /single-player");
    UI_SetOnClick(Mission13Button, "menu /load Maps\\Campaign\\Human02.w3m");
    UI_SetOnClick(Mission12Button, "menu /load Maps\\Campaign\\Human01.w3m");
    UI_SetOnClick(Mission11Button, "menu /load Maps\\Campaign\\Orc01.w3m");
    UI_SetOnClick(Mission10Button, "menu /load Maps\\Campaign\\Undead01.w3m");
}

static void Init_MultiplayerJoinMenu(void) {
    UI_FRAME(LocalMultiplayerJoin);
    LPFRAMEDEF PlayerNameEditBox = UI_FindChildFrame(LocalMultiplayerJoin, "PlayerNameEditBox");
    LPFRAMEDEF GameListLabel = UI_FindChildFrame(LocalMultiplayerJoin, "GameListLabel");
    LPFRAMEDEF GameListContainer = UI_FindChildFrame(LocalMultiplayerJoin, "GameListContainer");
    LPFRAMEDEF CreateButton = UI_FindChildFrame(LocalMultiplayerJoin, "CreateButton");
    LPFRAMEDEF LoadButton = UI_FindChildFrame(LocalMultiplayerJoin, "LoadButton");
    LPFRAMEDEF JoinButton = UI_FindChildFrame(LocalMultiplayerJoin, "JoinButton");
    LPFRAMEDEF CancelButton = UI_FindChildFrame(LocalMultiplayerJoin, "CancelButton");

    if (PlayerNameEditBox) {
        LPFRAMEDEF PlayerNameEditText = UI_FindChildFrame(PlayerNameEditBox, "PlayerNameEditBoxText");
        if (!PlayerNameEditText) {
            PlayerNameEditText = UI_Spawn(FT_TEXT, PlayerNameEditBox);
            if (PlayerNameEditText) {
                strcpy(PlayerNameEditText->Name, "PlayerNameEditBoxText");
                UI_InheritFrom(PlayerNameEditText, "StandardEditBoxTextTemplate");
                PlayerNameEditText->Parent = PlayerNameEditBox;
                UI_SetAllPoints(PlayerNameEditText);
            }
        }
        if (PlayerNameEditText) {
            strcpy(PlayerNameEditBox->Edit.TextFrame, PlayerNameEditText->Name);
        }
        PlayerNameEditBox->Edit.MaxChars = 15;
        UI_SetText(PlayerNameEditBox, "Player");
    }
    if (GameListContainer && !UI_FindChildFrame(GameListContainer, "GameListBox")) {
        LPFRAMEDEF GameListBox = UI_Spawn(FT_LISTBOX, GameListContainer);
        if (GameListBox) {
            LPFRAMEDEF GameListBackdrop;

            strcpy(GameListBox->Name, "GameListBox");
            UI_InheritFrom(GameListBox, "StandardListBoxTemplate");
            GameListBox->Parent = GameListContainer;
            GameListBackdrop = UI_Spawn(FT_BACKDROP, GameListBox);
            if (GameListBackdrop) {
                strcpy(GameListBackdrop->Name, "GameListBackdrop");
                UI_InheritFrom(GameListBackdrop, "StandardEditBoxBackdropTemplate");
                GameListBackdrop->Parent = GameListBox;
                strcpy(GameListBox->Control.Backdrop.Normal, GameListBackdrop->Name);
                UI_SetAllPoints(GameListBackdrop);
            }
            if (GameListLabel) {
                GameListBox->Font = GameListLabel->Font;
            }
            GameListBox->Color = COLOR32_WHITE;
            UI_SetAllPoints(GameListBox);
        }
    }

    UI_SetOnClick(CreateButton, "menu /lan/create");
    UI_SetOnClick(LoadButton, "menu /lan");
    UI_SetOnClick(JoinButton, "menu /lan");
    UI_SetOnClick(CancelButton, "menu /main");
}

static void Init_MultiplayerCreateMenu(void) {
    UI_FRAME(LocalMultiplayerCreate);
    LPFRAMEDEF MapInfoButton = UI_FindChildFrame(LocalMultiplayerCreate, "MapInfoButton");
    LPFRAMEDEF AdvancedOptionsButton = UI_FindChildFrame(LocalMultiplayerCreate, "AdvancedOptionsButton");
    LPFRAMEDEF PlayButton = UI_FindChildFrame(LocalMultiplayerCreate, "PlayButton");
    LPFRAMEDEF CancelButton = UI_FindChildFrame(LocalMultiplayerCreate, "CancelButton");
    UI_SetOnClick(MapInfoButton, "menu /main");
    UI_SetOnClick(AdvancedOptionsButton, "menu /main");
    UI_SetOnClick(PlayButton, "menu /main");
    UI_SetOnClick(CancelButton, "menu /lan");
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
    UI_ParseFDF("UI\\FrameDef\\Glue\\LoadSavedGameScreen.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\ViewReplayScreen.fdf");
    UI_ParseFDF("UI\\FrameDef\\Glue\\Skirmish.fdf");
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
    UI_PrepareMenuFrameState(MainMenuFrame, false);
    UI_WriteMenuWithMainFrameAnimation(ent, MainMenuFrame, "MainMenu");
}

void UI_ShowRealmSelect(LPEDICT ent, BOOL visible) {
    UI_FRAME(MainMenuFrame);
    UI_PrepareMenuFrameState(MainMenuFrame, visible);
    UI_WriteMenuWithMainFrameAnimation(ent, MainMenuFrame, "MainMenu");
}

void UI_ShowSinglePlayerMenu(LPEDICT ent) {
    UI_FRAME(SinglePlayerMenu);
    UI_PrepareMenuFrameState(SinglePlayerMenu, false);
    UI_WriteMenuWithMainFrameAnimation(ent, SinglePlayerMenu, "SinglePlayer");
}

void UI_ShowMultiplayerMenu(LPEDICT ent) {
    UI_FRAME(LocalMultiplayerJoin);
    UI_PrepareMenuFrameState(LocalMultiplayerJoin, false);
    UI_WriteMenuWithMainFrameAnimation(ent, LocalMultiplayerJoin, "BattlenetCustom");
}

void UI_ShowGameInterface(LPEDICT ent) {
    UI_FRAME(ConsoleUI);
    UI_FRAME(CinematicPanel);

    UI_WriteLayout(ent, ConsoleUI, LAYER_CONSOLE);
    UI_WriteLayout(ent, CinematicPanel, LAYER_CINEMATIC);
}

void UI_ShowMapSelectMenu(LPEDICT ent, LPCSTR category) {
    if (category && !strcmp(category, "skirmish")) {
        UI_FRAME(Skirmish);
        UI_PrepareMenuFrameState(Skirmish, false);
        UI_WriteMenuWithMainFrameAnimation(ent, Skirmish, UI_MenuPanelAnimationForMapCategory(category));
    } else if (category && (!strcmp(category, "loadsaved") || !strcmp(category, "load-saved"))) {
        UI_FRAME(LoadSavedGameScreen);
        UI_PrepareMenuFrameState(LoadSavedGameScreen, false);
        UI_WriteMenuWithMainFrameAnimation(ent, LoadSavedGameScreen, UI_MenuPanelAnimationForMapCategory(category));
    } else if (category && (!strcmp(category, "replay") || !strcmp(category, "replays"))) {
        UI_FRAME(ViewReplayScreen);
        UI_PrepareMenuFrameState(ViewReplayScreen, false);
        UI_WriteMenuWithMainFrameAnimation(ent, ViewReplayScreen, UI_MenuPanelAnimationForMapCategory(category));
    } else {
        UI_FRAME(CampaignMenu);
        UI_PrepareMenuFrameState(CampaignMenu, false);
        UI_WriteMenuWithMainFrameAnimationAndBackground(ent,
                                                        CampaignMenu,
                                                        UI_MenuPanelAnimationForMapCategory(category),
                                                        "HumanBackdrop");
    }
}

typedef struct {
    LPCSTR route;
    LPCSTR category;
} uiMapRoute_t;

static uiMapRoute_t const map_routes[] = {
    { "/single-player/campaign", "campaign" },
    { "mapselect", "campaign" },
    { "mapselect/campaign", "campaign" },
    { "/single-player/load-saved", "load-saved" },
    { "mapselect/loadsaved", "loadsaved" },
    { "/single-player/replays", "replays" },
    { "mapselect/replay", "replay" },
    { "/single-player/skirmish", "skirmish" },
    { "mapselect/skirmish", "skirmish" },
};

static BOOL UI_RouteEquals(LPCSTR route, LPCSTR expected) {
    return route && expected && !strcmp(route, expected);
}

void UI_RenderRoute(LPEDICT ent, LPCSTR route) {
    if (!route || !*route || UI_RouteEquals(route, "/main") || UI_RouteEquals(route, "main")) {
        UI_ShowMainMenu(ent);
        return;
    }
    if (UI_RouteEquals(route, "/game") || UI_RouteEquals(route, "game")) {
        UI_ShowGameInterface(ent);
        return;
    }
    if (UI_RouteEquals(route, "/realm-select") || UI_RouteEquals(route, "realmselect")) {
        UI_ShowRealmSelect(ent, true);
        return;
    }
    if (UI_RouteEquals(route, "/single-player") || UI_RouteEquals(route, "singleplayer")) {
        UI_ShowSinglePlayerMenu(ent);
        return;
    }
    if (UI_RouteEquals(route, "/lan") || UI_RouteEquals(route, "multiplayer") || UI_RouteEquals(route, "multiplayer/join")) {
        UI_ShowMultiplayerMenu(ent);
        return;
    }
    if (UI_RouteEquals(route, "/lan/create") || UI_RouteEquals(route, "multiplayer/create")) {
        UI_FRAME(LocalMultiplayerCreate);
        UI_PrepareMenuFrameState(LocalMultiplayerCreate, false);
        UI_WriteMenuWithMainFrameAnimation(ent, LocalMultiplayerCreate, "BattlenetCustomCreate");
        return;
    }
    FOR_LOOP(i, sizeof(map_routes) / sizeof(*map_routes)) {
        if (UI_RouteEquals(route, map_routes[i].route)) {
            UI_ShowMapSelectMenu(ent, map_routes[i].category);
            return;
        }
    }
    UI_ShowMainMenu(ent);
}
