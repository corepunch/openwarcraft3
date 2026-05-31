/*
 * ui/screens/single_player.c — Single player menu screen.
 */

#include "../ui_local.h"
#include "../ui_screen.h"

typedef enum {
    SINGLE_PLAYER_VIEW_MAIN,
    SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT,
} singlePlayerView_t;

typedef struct {
    playerRace_t race;
    LPCSTR route;
    LPCSTR map_path;
    LPCSTR background_model;
} singlePlayerCampaign_t;

static singlePlayerCampaign_t const campaigns[] = {
    { kPlayerRaceHuman, "human", "Maps\\Campaign\\Human02.w3m",
      "UI\\Glues\\SinglePlayer\\HumanCampaign3D\\HumanCampaign3D.mdx" },
    { kPlayerRaceOrc, "orc", "Maps\\Campaign\\Orc01.w3m",
      "UI\\Glues\\SinglePlayer\\OrcCampaign3D\\OrcCampaign3D.mdx" },
    { kPlayerRaceUndead, "undead", "Maps\\Campaign\\Undead01.w3m",
      "UI\\Glues\\SinglePlayer\\UndeadCampaign3D\\UndeadCampaign3D.mdx" },
    { kPlayerRaceNightElf, "night-elf", "Maps\\Campaign\\NightElf01.w3m",
      "UI\\Glues\\SinglePlayer\\NightElfCampaign3D\\NightElfCampaign3D.mdx" },
    { kPlayerRaceNone, "tutorial", "Maps\\Campaign\\Prologue01.w3m",
      "UI\\Glues\\SinglePlayer\\TutorialCampaign3D\\TutorialCampaign3D.mdx" },
};

static LPFRAMEDEF frame_single_player = NULL;
static LPFRAMEDEF frame_single_player_main = NULL;
static LPFRAMEDEF frame_profile_panel = NULL;
static LPFRAMEDEF frame_campaign = NULL;
static LPFRAMEDEF frame_campaign_select = NULL;
static LPFRAMEDEF frame_mission_select = NULL;
static LPFRAMEDEF frame_campaign_backdrop = NULL;
static LPFRAMEDEF frame_campaign_fade = NULL;
static LPFRAMEDEF frame_campaign_logo = NULL;
static DWORD campaign_background_model = 0;
static singlePlayerView_t current_view = SINGLE_PLAYER_VIEW_MAIN;

static singlePlayerCampaign_t const *SinglePlayer_FindCampaign(LPCSTR route) {
    if (!route) {
        return NULL;
    }
    for (DWORD i = 0; i < sizeof(campaigns) / sizeof(campaigns[0]); i++) {
        if (!strcmp(route, campaigns[i].route)) {
            return &campaigns[i];
        }
    }
    return NULL;
}

static void SinglePlayer_SetHidden(LPFRAMEDEF frame, BOOL hidden) {
    if (frame) {
        UI_SetHidden(frame, hidden);
    }
}

static void SinglePlayer_SetView(singlePlayerView_t view) {
    current_view = view;

    SinglePlayer_SetHidden(frame_single_player, view != SINGLE_PLAYER_VIEW_MAIN);
    SinglePlayer_SetHidden(frame_single_player_main, false);
    SinglePlayer_SetHidden(frame_profile_panel, true);

    SinglePlayer_SetHidden(frame_campaign, view != SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT);
    SinglePlayer_SetHidden(frame_campaign_backdrop, true);
    SinglePlayer_SetHidden(frame_campaign_select, false);
    SinglePlayer_SetHidden(frame_mission_select, true);
    SinglePlayer_SetHidden(frame_campaign_fade, true);
}

static void SinglePlayer_SetCampaignBackdrop(singlePlayerCampaign_t const *campaign) {
    if (frame_campaign_backdrop && campaign && campaign->background_model) {
        campaign_background_model = UI_LoadModel(campaign->background_model, false);
        frame_campaign_backdrop->Portrait.model = campaign_background_model;
    }
}

static void SinglePlayer_DrawCampaignBackdrop(void) {
    LPRENDERER renderer = uiimport.GetRenderer ? uiimport.GetRenderer() : NULL;
    LPCMODEL model = UI_GetModel(campaign_background_model);
    RECT viewport = { 0, 0, 1, 1 };

    if (renderer && renderer->DrawPortrait && model) {
        renderer->DrawPortrait(model, &viewport, "Stand");
    }
}

static void SinglePlayer_LaunchCampaign(singlePlayerCampaign_t const *campaign) {
    char command[256];

    if (!campaign || !campaign->map_path) {
        return;
    }
    snprintf(command, sizeof(command), "map \"%s\"", campaign->map_path);
    UI_MenuCommandLocal(command);
}

static void SinglePlayer_BindMainMenu(void) {
    LPFRAMEDEF CampaignButton;
    LPFRAMEDEF LoadSavedButton;
    LPFRAMEDEF ViewReplayButton;
    LPFRAMEDEF SkirmishButton;
    LPFRAMEDEF ProfileButton;
    LPFRAMEDEF CancelButton;
    LPFRAMEDEF ProfileNameText;

    if (!frame_single_player) {
        return;
    }

    CampaignButton = UI_FindChildFrame(frame_single_player, "CampaignButton");
    LoadSavedButton = UI_FindChildFrame(frame_single_player, "LoadSavedButton");
    ViewReplayButton = UI_FindChildFrame(frame_single_player, "ViewReplayButton");
    SkirmishButton = UI_FindChildFrame(frame_single_player, "SkirmishButton");
    ProfileButton = UI_FindChildFrame(frame_single_player, "ProfileButton");
    CancelButton = UI_FindChildFrame(frame_single_player, "CancelButton");
    ProfileNameText = UI_FindChildFrame(frame_single_player, "ProfileNameText");

    UI_SetOnClick(CampaignButton, "menu /single-player/campaign");
    UI_SetOnClick(LoadSavedButton, "");
    UI_SetOnClick(ViewReplayButton, "");
    UI_SetOnClick(SkirmishButton, "");
    UI_SetOnClick(ProfileButton, "");
    UI_SetOnClick(CancelButton, "menu_main");
    if (ProfileNameText) {
        UI_SetText(ProfileNameText, "Player");
    }
}

static void SinglePlayer_BindCampaignMenu(void) {
    LPFRAMEDEF BackButton;
    LPFRAMEDEF HumanButton;
    LPFRAMEDEF OrcButton;
    LPFRAMEDEF UndeadButton;
    LPFRAMEDEF NightElfButton;
    LPFRAMEDEF TutorialButton;
    LPFRAMEDEF DifficultySelect;
    LPFRAMEDEF DifficultyMenu;
    LPFRAMEDEF DifficultyTitle;

    if (!frame_campaign) {
        return;
    }

    BackButton = UI_FindChildFrame(frame_campaign, "BackButton");
    HumanButton = UI_FindChildFrame(frame_campaign, "HumanButton");
    OrcButton = UI_FindChildFrame(frame_campaign, "OrcButton");
    UndeadButton = UI_FindChildFrame(frame_campaign, "UndeadButton");
    NightElfButton = UI_FindChildFrame(frame_campaign, "NightElfButton");
    TutorialButton = UI_FindChildFrame(frame_campaign, "TutorialButton");
    DifficultySelect = UI_FindChildFrame(frame_campaign, "DifficultySelect");
    DifficultyMenu = DifficultySelect ? UI_FindChildFrame(DifficultySelect, "CampaignPopupMenuMenu") : NULL;
    DifficultyTitle = DifficultySelect ? UI_FindChildFrame(DifficultySelect, "CampaignPopupMenuTitleTextTemplate") : NULL;

    UI_SetOnClick(BackButton, "menu_game");
    UI_SetOnClick(HumanButton, "menu /single-player/campaign/human");
    UI_SetOnClick(OrcButton, "menu /single-player/campaign/orc");
    UI_SetOnClick(UndeadButton, "menu /single-player/campaign/undead");
    UI_SetOnClick(NightElfButton, "menu /single-player/campaign/night-elf");
    UI_SetOnClick(TutorialButton, "menu /single-player/campaign/tutorial");

    if (DifficultyMenu) {
        UI_MenuClearItems(DifficultyMenu);
        UI_MenuAddItem(DifficultyMenu, UI_GetString("EASY"), 0);
        UI_MenuAddItem(DifficultyMenu, UI_GetString("NORMAL"), 1);
        UI_MenuAddItem(DifficultyMenu, UI_GetString("HARD"), 2);
        UI_SetOnClick(DifficultyMenu, "menu /single-player/difficulty/%u");
        UI_SetHidden(DifficultyMenu, true);
    }
    if (DifficultyTitle) {
        UI_SetText(DifficultyTitle, "NORMAL");
    }
}

static void SinglePlayerMenu_Init(void) {
    uiimport.Printf("SinglePlayerMenu_Init\n");
    UI_PreloadGlueSceneModels();

    frame_single_player = UI_FindFrame("SinglePlayerMenu");
    frame_single_player_main = frame_single_player ? UI_FindChildFrame(frame_single_player, "MainPanel") : NULL;
    frame_profile_panel = frame_single_player ? UI_FindChildFrame(frame_single_player, "ProfilePanel") : NULL;
    frame_campaign = UI_FindFrame("CampaignMenu");
    frame_campaign_select = frame_campaign ? UI_FindChildFrame(frame_campaign, "CampaignSelectFrame") : NULL;
    frame_mission_select = frame_campaign ? UI_FindChildFrame(frame_campaign, "MissionSelectFrame") : NULL;
    frame_campaign_backdrop = frame_campaign ? UI_FindChildFrame(frame_campaign, "CampaignBackdrop") : NULL;
    frame_campaign_fade = frame_campaign ? UI_FindChildFrame(frame_campaign, "SlidingDoors") : NULL;
    frame_campaign_logo = frame_campaign ? UI_FindChildFrame(frame_campaign, "WarCraftIIILogo") : NULL;

    if (frame_campaign_logo) {
        frame_campaign_logo->Portrait.model = UI_LoadModel("CampaignLogo", true);
    }

    SinglePlayer_BindMainMenu();
    SinglePlayer_BindCampaignMenu();
    SinglePlayer_SetCampaignBackdrop(&campaigns[0]);
    SinglePlayer_SetView(SINGLE_PLAYER_VIEW_MAIN);
}

static void SinglePlayerMenu_Shutdown(void) {
}

static void SinglePlayerMenu_Refresh(int msec) {
    (void)msec;
}

static void SinglePlayerMenu_Draw(void) {
    if (current_view == SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT) {
        SinglePlayer_DrawCampaignBackdrop();
        if (frame_campaign) {
            UI_DrawFrame(frame_campaign);
        }
        return;
    }

    UI_DrawGlueScene("SinglePlayer Stand");
    if (frame_single_player) {
        UI_DrawFrame(frame_single_player);
    }
}

static void SinglePlayerMenu_KeyEvent(int key, BOOL down) {
    (void)key;
    (void)down;
}

static void SinglePlayerMenu_MouseEvent(int x, int y, int buttons) {
    (void)x;
    (void)y;
    (void)buttons;
}

static void SinglePlayerMenu_Route(LPCSTR path) {
    if (!path || !strcmp(path, "/") || !strcmp(path, "/main")) {
        SinglePlayer_SetView(SINGLE_PLAYER_VIEW_MAIN);
        return;
    }

    if (!strcmp(path, "/campaign")) {
        SinglePlayer_SetCampaignBackdrop(&campaigns[0]);
        SinglePlayer_SetView(SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT);
        return;
    }

    if (!strncmp(path, "/campaign/", 10)) {
        singlePlayerCampaign_t const *campaign = SinglePlayer_FindCampaign(path + 10);
        SinglePlayer_SetCampaignBackdrop(campaign);
        SinglePlayer_LaunchCampaign(campaign);
        return;
    }

    if (!strncmp(path, "/difficulty/", 12)) {
        return;
    }
}

uiScreen_t singlePlayerMenuScreen = {
    .name = "single-player",
    .init = SinglePlayerMenu_Init,
    .shutdown = SinglePlayerMenu_Shutdown,
    .refresh = SinglePlayerMenu_Refresh,
    .draw = SinglePlayerMenu_Draw,
    .key_event = SinglePlayerMenu_KeyEvent,
    .mouse_event = SinglePlayerMenu_MouseEvent,
    .route = SinglePlayerMenu_Route,
};
