/* Loading must survive the real FDF loader and the initial-layout serialization path. */
#ifdef BZ_TESTS
#include "shared/test.h"
#include "../hud/hud_local.h"

typedef struct {
    DWORD sprites, bar, back, texts, sent, bytes;
    LONG opcode, layer;
    UIFRAME progress;
    char anim[32];
} LOADCAP;
static LOADCAP loadcap;

/* Capture what a connecting client receives, rather than inspecting manually constructed frames. */
static void loading_write(pfWriteType_t type, void const *data) {
    if (type == PF_BYTE) {
        if (!loadcap.bytes++) loadcap.opcode = *(LONG const *)data;
        else loadcap.layer = *(LONG const *)data;
    }
    if (type != PF_UIFRAME) return;
    LPCUIFRAME frame = data;
    if (frame->flags.type == FT_SPRITE) {
        loadcap.sprites++;
        if (frame->stat == UI_STAT_LOADING_PROGRESS) {
            loadcap.bar = frame->tex.index;
            loadcap.progress = *frame;
            T_STREQ(frame->text, "#0");
        } else {
            loadcap.back = frame->tex.index;
            strlcpy(loadcap.anim, frame->text, sizeof(loadcap.anim));
        }
    }
    if (frame->flags.type == FT_TEXT && frame->text) {
        if (!strcmp(frame->text, "Chapter")) loadcap.texts |= 1;
        if (!strcmp(frame->text, "Subtitle")) loadcap.texts |= 2;
        if (!strcmp(frame->text, "Description")) loadcap.texts |= 4;
    }
}

static void loading_unicast(LPEDICT ent) { (void)ent; loadcap.sent++; }

/* tests.mpq carries the native FDF plus ROC/TFT WorldEditData rows and decorated skin keys. */
TEST(wc3_loading, initial_layout_resolves_campaign_custom_and_melee_art) {
    static const struct { DWORD row; LPSTR custom; LPCSTR model, anim; } cases[] = {
        { 0, NULL, "UI\\Glues\\Loading\\Backgrounds\\Campaigns\\LordaeronBackground.mdx", "#!0" },
        { 1, NULL, "UI\\Glues\\Loading\\Backgrounds\\Campaigns\\LordaeronExpansionBackground.mdx", "#!6" },
        { 1, "war3mapImported\\LoadingScreen.mdx", "war3mapImported\\LoadingScreen.mdx", "#!0" },
        { (DWORD)-1, NULL, "UI\\Glues\\Loading\\Multiplayer\\Load-Multiplayer-Random.mdx", "#!0" },
    };
    __typeof__(gi.Write) old_write = gi.Write;
    __typeof__(gi.unicast) old_send = gi.unicast;
    LPCMAPINFO old_info = level.mapinfo;
    MAPINFO info = { .mapName = "Chapter", .loadingScreenTitle = "Chapter",
                    .loadingScreenSubtitle = "Subtitle", .loadingScreenText = "Description" };

    UI_ResetHud(); UI_LoadHudLoading();
    T_NOT_NULL(hud.loading.Loading); T_NOT_NULL(hud.loading.LoadingBar);
    T_NOT_NULL(hud.loading.LoadingBackground);
    if (!hud.loading.Loading || !hud.loading.LoadingBar || !hud.loading.LoadingBackground) return;
    gi.Write = loading_write; gi.unicast = loading_unicast; level.mapinfo = &info;
    FOR_LOOP(i, sizeof(cases) / sizeof(cases[0])) {
        info.campaignBackgroundNumber = cases[i].row;
        info.loadingScreenModel = cases[i].custom;
        /* Empty title must resolve the authored map name; custom art must override the campaign row. */
        info.loadingScreenTitle = i == 3 ? "" : "Chapter";
        memset(&loadcap, 0, sizeof(loadcap));
        UI_WriteLoadingLayout(g_edicts);
        T_EQ(loadcap.opcode, svc_layout); T_EQ(loadcap.layer, LAYER_LOADING); T_EQ(loadcap.sent, 1);
        T_EQ(loadcap.sprites, 2); T_NE(loadcap.bar, 0); T_NE(loadcap.back, 0); T_EQ(loadcap.texts, 7);
        T_STREQ(gi.GetConfigstring(CS_MODELS + loadcap.back), cases[i].model);
        T_STREQ(gi.GetConfigstring(CS_MODELS + loadcap.bar), "UI\\Glues\\Loading\\LoadBar\\LoadBar.mdx");
        T_STREQ(loadcap.anim, cases[i].anim);
        T_FEQ(loadcap.progress.size.width, 0, 0.00001f); T_FEQ(loadcap.progress.size.height, 0, 0.00001f);
        T_EQ(loadcap.progress.points.y[FPP_MAX].offset, (SHORT)(0.0025f * UI_FRAMEPOINT_SCALE));
    }
    gi.Write = old_write; gi.unicast = old_send; level.mapinfo = old_info;
    UI_ResetHud();
}
#endif
