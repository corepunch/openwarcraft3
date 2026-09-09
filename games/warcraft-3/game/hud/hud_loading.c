/* Serializes the stock Loading.fdf tree before gameplay UI exists. */
#include "hud_local.h"

/* The native zero-size loading bar gets its geometry from its MDX, not a portrait viewport. */
void UI_LoadHudLoading(void) {
    if (!LoadingScreen_Load(&hud.loading)) {
        fprintf(stderr, "UI_LoadHudLoading: missing Loading.fdf\n");
        return;
    }
    if (hud.loading.LoadingCustomPanel) UI_SetHidden(hud.loading.LoadingCustomPanel, false);
    if (hud.loading.LoadingMeleePanel) UI_SetHidden(hud.loading.LoadingMeleePanel, true);
    if (hud.loading.LoadingBar) {
        hud.loading.LoadingBar->Portrait.model = UI_LoadModel("LoadingProgressBar", true);
        hud.loading.LoadingBar->Stat = UI_STAT_LOADING_PROGRESS;
        UI_SetText(hud.loading.LoadingBar, "#0");
    }
}

static LPCSTR loading_text(LPCMAPINFO info, LPCSTR text) { return text && *text ? G_MapString(info, text) : " "; }

/* Resolve W3I presentation before the server publishes the loading-only media table. */
void UI_WriteLoadingLayout(LPEDICT ent, LPCMAPINFO info) {
    LPCSTR title = info && info->loadingScreenTitle && *info->loadingScreenTitle ? info->loadingScreenTitle :
                   info ? info->mapName : NULL;
    DWORD model = 0, seq = 0;

    if (!hud.loading.Loading) return;
    if (hud.loading.LoadingTitleText)
        UI_SetText(hud.loading.LoadingTitleText, "%s", loading_text(info, title));
    if (hud.loading.LoadingSubtitleText)
        UI_SetText(hud.loading.LoadingSubtitleText, "%s", loading_text(info, info ? info->loadingScreenSubtitle : NULL));
    if (hud.loading.LoadingText)
        UI_SetText(hud.loading.LoadingText, "%s", loading_text(info, info ? info->loadingScreenText : NULL));
    /* Loading.fdf authors screen-space sprites; portrait conversion discarded their native geometry. */
    if (info && info->loadingScreenModel && *info->loadingScreenModel) {
        model = UI_LoadModel(info->loadingScreenModel, false);
    } else if (info && info->campaignBackgroundNumber != (DWORD)-1) {
        stbIniCache_t data = { 0 };
        PATHSTR path;
        char key[16];
        Stb_IniCacheLoad(&data, "UI\\WorldEditData.txt");
        snprintf(key, sizeof(key), "%02u", (unsigned)info->campaignBackgroundNumber);
        LPCSTR row = Stb_IniCacheFind(&data, "LoadingScreens", key);
        if (UI_ParseLoadingRow(row, &seq, path)) model = UI_LoadModel(path, false);
        else fprintf(stderr, "UI_WriteLoadingLayout: invalid LoadingScreens[%s]: %s\n", key, row ? row : "(missing)");
        Stb_IniCacheFree(&data);
    } else {
        model = UI_LoadModel("LoadingMeleeBackground", true);
    }
    if (hud.loading.LoadingBackground) {
        hud.loading.LoadingBackground->Portrait.model = model;
        UI_SetText(hud.loading.LoadingBackground, "#!%u", (unsigned)seq);
    }
    UI_WriteLayout(ent, hud.loading.Loading, LAYER_LOADING);
}
