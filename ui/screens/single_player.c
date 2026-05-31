/*
 * ui/screens/single_player.c — Single player menu screen.
 */

#include "../ui_local.h"
#include "../ui_screen.h"
#include "../generated/single_player_menu.h"

typedef enum {
    SINGLE_PLAYER_VIEW_MAIN,
    SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT,
} singlePlayerView_t;

typedef struct {
    playerRace_t race;
    LPCSTR name;
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

static SinglePlayerMenu_t single_player;
static DWORD campaign_background_model = 0;
static singlePlayerView_t current_view = SINGLE_PLAYER_VIEW_MAIN;

static BOOL SinglePlayerMenu_LoadScreen(void) {
    return SinglePlayerMenu_Load(&single_player);
}

static singlePlayerCampaign_t const *SinglePlayer_FindCampaign(LPCSTR name) {
    if (!name) {
        return NULL;
    }
    for (DWORD i = 0; i < sizeof(campaigns) / sizeof(campaigns[0]); i++) {
        if (!strcmp(name, campaigns[i].name)) {
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

    SinglePlayer_SetHidden(single_player.SinglePlayerMenu, view != SINGLE_PLAYER_VIEW_MAIN);
    SinglePlayer_SetHidden(single_player.MainPanel, false);
    SinglePlayer_SetHidden(single_player.ProfilePanel, true);

    SinglePlayer_SetHidden(single_player.CampaignMenu, view != SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT);
    SinglePlayer_SetHidden(single_player.CampaignBackdrop_2, true);
    SinglePlayer_SetHidden(single_player.CampaignSelectFrame, false);
    SinglePlayer_SetHidden(single_player.MissionSelectFrame, true);
    SinglePlayer_SetHidden(single_player.SlidingDoors, true);
}

static void SinglePlayer_SetCampaignBackdrop(singlePlayerCampaign_t const *campaign) {
    if (single_player.CampaignBackdrop_2 && campaign && campaign->background_model) {
        campaign_background_model = UI_LoadModel(campaign->background_model, false);
        single_player.CampaignBackdrop_2->Portrait.model = campaign_background_model;
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
    if (!single_player.SinglePlayerMenu) {
        return;
    }

    UI_SetOnClick(single_player.CampaignButton, "menu_single_player_campaign");
    UI_SetOnClick(single_player.LoadSavedButton, "");
    UI_SetOnClick(single_player.ViewReplayButton, "");
    UI_SetOnClick(single_player.SkirmishButton, "");
    UI_SetOnClick(single_player.ProfileButton, "");
    UI_SetOnClick(single_player.CancelButton, "menu_main");
    if (single_player.ProfileNameText) {
        UI_SetText(single_player.ProfileNameText, "Player");
    }
}

static void SinglePlayer_BindCampaignMenu(void) {
    LPFRAMEDEF DifficultyMenu;
    LPFRAMEDEF DifficultyTitle;

    if (!single_player.CampaignMenu) {
        return;
    }

    UI_SetOnClick(single_player.BackButton, "menu_game");
    UI_SetOnClick(single_player.HumanButton, "menu_single_player_campaign_human");
    UI_SetOnClick(single_player.OrcButton, "menu_single_player_campaign_orc");
    UI_SetOnClick(single_player.UndeadButton, "menu_single_player_campaign_undead");
    UI_SetOnClick(single_player.NightElfButton, "menu_single_player_campaign_night_elf");
    UI_SetOnClick(single_player.TutorialButton, "menu_single_player_campaign_tutorial");

    DifficultyMenu = single_player.DifficultySelect
        ? UI_FindChildFrame(single_player.DifficultySelect, "CampaignPopupMenuMenu")
        : NULL;
    DifficultyTitle = single_player.DifficultySelect
        ? UI_FindChildFrame(single_player.DifficultySelect, "CampaignPopupMenuTitleTextTemplate")
        : NULL;

    if (DifficultyMenu) {
        UI_MenuClearItems(DifficultyMenu);
        UI_MenuAddItem(DifficultyMenu, UI_GetString("EASY"), 0);
        UI_MenuAddItem(DifficultyMenu, UI_GetString("NORMAL"), 1);
        UI_MenuAddItem(DifficultyMenu, UI_GetString("HARD"), 2);
        UI_SetOnClick(DifficultyMenu, "menu_single_player_difficulty %u");
        UI_SetHidden(DifficultyMenu, true);
    }
    if (DifficultyTitle) {
        UI_SetText(DifficultyTitle, "NORMAL");
    }
}

static void SinglePlayerMenu_Init(void) {
    uiimport.Printf("SinglePlayerMenu_Init\n");
    UI_PreloadGlueSceneModels();

    if (single_player.WarCraftIIILogo) {
        single_player.WarCraftIIILogo->Portrait.model = UI_LoadModel("CampaignLogo", true);
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
        if (single_player.CampaignMenu) {
            UI_DrawFrame(single_player.CampaignMenu);
        }
        return;
    }

    UI_DrawGlueScene("SinglePlayer Stand");
    if (single_player.SinglePlayerMenu) {
        UI_DrawFrame(single_player.SinglePlayerMenu);
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

void SinglePlayerMenu_ShowMain(void) {
    SinglePlayer_SetView(SINGLE_PLAYER_VIEW_MAIN);
}

void SinglePlayerMenu_ShowCampaign(void) {
    SinglePlayer_SetCampaignBackdrop(&campaigns[0]);
    SinglePlayer_SetView(SINGLE_PLAYER_VIEW_CAMPAIGN_SELECT);
}

void SinglePlayerMenu_LaunchCampaign(LPCSTR name) {
    singlePlayerCampaign_t const *campaign = SinglePlayer_FindCampaign(name);
    SinglePlayer_SetCampaignBackdrop(campaign);
    SinglePlayer_LaunchCampaign(campaign);
}

uiScreen_t singlePlayerMenuScreen = {
    .name = "single-player",
    .load = SinglePlayerMenu_LoadScreen,
    .init = SinglePlayerMenu_Init,
    .shutdown = SinglePlayerMenu_Shutdown,
    .refresh = SinglePlayerMenu_Refresh,
    .draw = SinglePlayerMenu_Draw,
    .key_event = SinglePlayerMenu_KeyEvent,
    .mouse_event = SinglePlayerMenu_MouseEvent,
};
