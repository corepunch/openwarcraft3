/*
 * scene.c - shared glue background/sprite-layer model cache.
 */

#include "menu_local.h"

#define UI_GLUE_ANIM_NAME 96 // chars; fits Blizzard glue sequence names and suffixes; used as animation storage.
#define UI_GLUE_BIRTH_TIME 1000 // ms; every named RoC/TFT panel Birth interval has this length.
#define UI_GLUE_DEATH_TIME 667 // ms; every named RoC/TFT panel Death interval rounds to this length.

typedef struct {
    LPCSTR left;
    LPCSTR right;
    LPCSTR left_stand;
} uiGluePanelDef_t;

typedef enum {
    UI_GLUE_PANEL_IDLE,
    UI_GLUE_PANEL_EXIT,
    UI_GLUE_PANEL_ENTER,
} uiGluePanelPhase_t;

typedef struct {
    BOOL loaded;
    LPCMODEL background;
    LPCMODEL top_left_panel;
    LPCMODEL top_right_panel;
    uiGluePanel_t current;
    uiGluePanel_t target;
    uiGluePanelPhase_t phase;
    DWORD phase_start;
    uiGluePanelChanged_f exited;
    uiGluePanelChanged_f changed;
} uiGlueSceneState_t;

static uiGlueSceneState_t scene;

static uiGluePanelDef_t const glue_panels[UI_GLUE_PANEL_COUNT] = {
    [UI_GLUE_MAIN_MENU] = { .left = "MainMenu %s", .right = "MainMenu %s" },
    [UI_GLUE_REALM_SELECTION] = { .left = "RealmSelection %s", .right = "RealmSelection %s" },
    [UI_GLUE_SINGLE_PLAYER] = { .left = "SinglePlayer %s", .right = "SinglePlayer %s" },
    [UI_GLUE_OPTIONS] = { .left = "Options %s", .right = "Options %s", .left_stand = "Options Stand Alternate" },
    [UI_GLUE_SINGLE_PLAYER_SKIRMISH] = { .left = "SinglePlayerSkirmish %s", .right = "SinglePlayerSkirmish %s" },
    [UI_GLUE_MULTIPLAYER_PRE_GAME_CHAT] = { .left = "MultiplayerPreGameChat %s", .right = "MultiplayerPreGameChat %s" },
    [UI_GLUE_BATTLENET_CUSTOM] = { .left = "BattlenetCustom %s", .right = "BattlenetCustom %s" },
    [UI_GLUE_BATTLENET_CUSTOM_CREATE] = { .left = "BattlenetCustomCreate %s", .right = "BattlenetCustomCreate %s" },
};

static LPCSTR const phases[] = { "Stand", "Death", "Birth" };
static DWORD const durations[] = { 0, UI_GLUE_DEATH_TIME, UI_GLUE_BIRTH_TIME };
static FLOAT const screen_offsets[] = {
    -1.000f, -1.000f, -1.000f, -0.980f, -0.940f, -0.860f, -0.740f,
    -0.600f, -0.450f, -0.310f, -0.200f, -0.120f, -0.070f, -0.035f,
    -0.015f, 0.000f, 0.000f, 0.000f, 0.000f, 0.000f, 0.000f,
};

static void UI_GlueReleaseModel(LPCMODEL model) {
    mi.GetRenderer()->ReleaseModel((LPMODEL)model);
}

static LPCSTR UI_GlueBackgroundPath(void) {
    LPCSTR model = Theme_String("GlueSpriteLayerBackground", "Default");

    if (!model || !*model || !strcmp(model, "GlueSpriteLayerBackground")) {
        model = Theme_String("MainMenu", "Default");
    }
    if (!model || !*model || !strcmp(model, "MainMenu")) {
        model = "UI\\Glues\\MainMenu\\MainMenu3d\\MainMenu3d.mdx";
    }
    return model;
}

static LPCSTR UI_GlueTopLeftPanelPath(void) {
    return Theme_String("GlueSpriteLayerTopLeft", "UI\\Glues\\SpriteLayers\\TopLeftPanel.mdx");
}

static LPCSTR UI_GlueTopRightPanelPath(void) {
    return Theme_String("GlueSpriteLayerTopRight", "UI\\Glues\\SpriteLayers\\TopRightPanel.mdx");
}

/* The stock sprite layers are authored as a 4:3 pair: the left layer remains
 * at the scene origin and the right layer's origin must follow the extra
 * widescreen canvas width. */
static FLOAT UI_GlueRightPanelOffset(LPRENDERER renderer) {
    size2_t win = renderer->GetWindowSize();
    FLOAT aspect;

    if (win.height <= 0) return 0.0f;
    aspect = (FLOAT)win.width / (FLOAT)win.height;
    return aspect > UI_MIN_ASPECT ? UI_BASE_HEIGHT * aspect - UI_BASE_WIDTH : 0.0f;
}

/* Both panel models use the same fixed intervals, so one phase clock drives
 * both layers and the sequence name is derived only when drawing. */
static LPCSTR UI_GluePanelAnimation(LPCSTR format, LPSTR anim, DWORD anim_size) {
    DWORD duration, elapsed;

    snprintf(anim, anim_size, format, phases[scene.phase]);
    if (scene.phase == UI_GLUE_PANEL_IDLE) return anim;
    duration = durations[scene.phase];
    elapsed = MIN(M_Time() - scene.phase_start, duration);
    snprintf(anim + strlen(anim), anim_size - strlen(anim), "@%.4f", (FLOAT)elapsed / (FLOAT)duration);
    return anim;
}

static LPCSTR UI_GlueLeftAnimation(uiGluePanelDef_t const *panel, LPSTR anim, DWORD anim_size) {
    if (scene.phase == UI_GLUE_PANEL_IDLE && panel->left_stand) {
        snprintf(anim, anim_size, "%s", panel->left_stand);
        return anim;
    }
    return UI_GluePanelAnimation(panel->left, anim, anim_size);
}

void UI_ResetGlueSceneModels(void) {
    memset(&scene, 0, sizeof(scene));
}

void UI_ReleaseGlueSceneModels(void) {
    SAFE_DELETE(scene.background, UI_GlueReleaseModel);
    SAFE_DELETE(scene.top_left_panel, UI_GlueReleaseModel);
    SAFE_DELETE(scene.top_right_panel, UI_GlueReleaseModel);
    UI_ResetGlueSceneModels();
}

void UI_PreloadGlueSceneModels(void) {
    LPRENDERER renderer;

    if (scene.loaded) return;
    renderer = mi.GetRenderer();
    if (!renderer || !renderer->LoadModel) return;
    scene.background = renderer->LoadModel(UI_GlueBackgroundPath());
    scene.top_left_panel = renderer->LoadModel(UI_GlueTopLeftPanelPath());
    scene.top_right_panel = renderer->LoadModel(UI_GlueTopRightPanelPath());
    scene.loaded = true;
}

/* Panel names are the public transition contract. This owns the authored
 * current Death -> target Birth sequencing, including alternate left layers. */
void UI_GotoGluePanel(uiGluePanel_t panel, uiGluePanelChanged_f changed) {
    UI_GotoGluePanelTransition(panel, NULL, changed);
}

void UI_GotoGluePanelTransition(uiGluePanel_t panel, uiGluePanelChanged_f exited,
                                uiGluePanelChanged_f changed) {
    LPRENDERER renderer = mi.GetRenderer();

    UI_PreloadGlueSceneModels();
    if (!renderer) return;
    if (panel <= UI_GLUE_NONE || panel >= UI_GLUE_PANEL_COUNT) {
        fprintf(stderr, "UI: unknown glue panel %u\n", (unsigned)panel);
        return;
    }
    if (!scene.current) {
        scene.current = panel;
        scene.phase = UI_GLUE_PANEL_ENTER;
        scene.phase_start = M_Time();
        if (exited) exited();
        scene.exited = NULL;
        scene.changed = changed;
        return;
    }
    if (panel == scene.current || panel == scene.target) return;
    scene.target = panel;
    scene.phase = UI_GLUE_PANEL_EXIT;
    scene.phase_start = M_Time();
    scene.exited = exited;
    scene.changed = changed;
}

void UI_CloseGluePanel(uiGluePanelChanged_f changed) {
    LPRENDERER renderer = mi.GetRenderer();

    if (!renderer) return;
    if (!scene.current) {
        if (changed) changed();
        return;
    }
    if (scene.phase == UI_GLUE_PANEL_EXIT && !scene.target) return;
    scene.target = UI_GLUE_NONE;
    scene.phase = UI_GLUE_PANEL_EXIT;
    scene.phase_start = M_Time();
    scene.changed = changed;
}

static void UI_GlueFinishExit(void) {
    uiGluePanelChanged_f exited = scene.exited;

    scene.exited = NULL;
    if (scene.target) {
        scene.current = scene.target;
        scene.target = UI_GLUE_NONE;
        scene.phase = UI_GLUE_PANEL_ENTER;
    } else {
        uiGluePanelChanged_f changed = scene.changed;

        scene.current = UI_GLUE_NONE;
        scene.phase = UI_GLUE_PANEL_IDLE;
        scene.changed = NULL;
        if (changed) changed();
    }
    scene.phase_start = M_Time();
    if (exited) exited();
}

static void UI_GlueAdvanceTransition(void) {
    DWORD elapsed = M_Time() - scene.phase_start;

    if (scene.phase == UI_GLUE_PANEL_EXIT && elapsed >= UI_GLUE_DEATH_TIME)
        UI_GlueFinishExit();
    else if (scene.phase == UI_GLUE_PANEL_ENTER && elapsed >= UI_GLUE_BIRTH_TIME) {
        uiGluePanelChanged_f changed = scene.changed;

        scene.phase = UI_GLUE_PANEL_IDLE;
        scene.changed = NULL;
        if (changed) changed();
    }
}

void UI_DrawGlueScene(void) {
    LPRENDERER renderer = mi.GetRenderer();
    uiGluePanelDef_t const *panel;
    FLOAT right_offset;
    char left_anim[UI_GLUE_ANIM_NAME];
    char right_anim[UI_GLUE_ANIM_NAME];

    if (!renderer || !scene.current) return;
    UI_PreloadGlueSceneModels();
    UI_GlueAdvanceTransition();
    if (!scene.current) return;
    panel = &glue_panels[scene.current];
    right_offset = UI_GlueRightPanelOffset(renderer);

    if (renderer->RenderFrame && scene.background) {
        renderEntity_t entity = {0};
        entity.model = scene.background;
        entity.scale = 1.0f;
        entity.flags = RF_NO_SHADOW | RF_NO_FOGOFWAR | RF_PORTRAIT_LIGHTING;
        renderer->SetEntityAnimFrame(scene.background, "Stand", &entity);

        viewDef_t viewdef = {0};
        viewdef.viewport = (RECT){0, 0, 1, 1};
        viewdef.rdflags = RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL | RDF_NOFOG | RDF_USE_ENTITY_CAMERA;
        viewdef.num_entities = 1;
        viewdef.entities = &entity;
        renderer->RenderFrame(&viewdef);
    }

    if (renderer->DrawSprite && scene.top_left_panel) {
        LPCSTR anim = UI_GlueLeftAnimation(panel, left_anim, sizeof(left_anim));
        renderer->DrawSprite(scene.top_left_panel, anim, 0.0f, UI_BASE_HEIGHT);
    }
    if (renderer->DrawSprite && scene.top_right_panel) {
        LPCSTR anim = UI_GluePanelAnimation(panel->right, right_anim, sizeof(right_anim));
        renderer->DrawSprite(scene.top_right_panel, anim, right_offset, UI_BASE_HEIGHT);
    }
}

BOOL UI_GetGlueScreenOffset(LPVECTOR2 offset) {
    DWORD const count = sizeof(screen_offsets) / sizeof(screen_offsets[0]);
    DWORD duration, elapsed, index;
    FLOAT sample, position;

    if (!offset || scene.phase == UI_GLUE_PANEL_IDLE) return false;
    duration = durations[scene.phase];
    elapsed = MIN(M_Time() - scene.phase_start, duration);
    position = (FLOAT)elapsed * (FLOAT)(count - 1) / (FLOAT)duration;
    if (scene.phase == UI_GLUE_PANEL_EXIT)
        position = (FLOAT)(count - 1) - position;
    index = (DWORD)position;
    sample = screen_offsets[index];
    if (index + 1 < count)
        sample += (screen_offsets[index + 1] - sample) * (position - (FLOAT)index);
    offset->x = 0.0f;
    offset->y = sample * UI_BASE_HEIGHT;
    return true;
}
