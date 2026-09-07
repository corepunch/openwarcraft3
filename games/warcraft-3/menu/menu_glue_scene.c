/*
 * menu_glue_scene.c - shared glue background/sprite-layer model cache.
 */

#include "menu_local.h"

#define UI_GLUE_ANIM_NAME 96 // chars; fits Blizzard glue sequence names and suffixes; used as animation storage.
#define UI_GLUE_BIRTH_TIME 1000 // ms; every named RoC/TFT panel Birth interval has this length.
#define UI_GLUE_DEATH_TIME 667 // ms; every named RoC/TFT panel Death interval rounds to this length.

typedef struct {
    LPCSTR left;
    LPCSTR right;
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
    uiGluePanelChanged_f changed;
} uiGlueSceneState_t;

static uiGlueSceneState_t menu_glue_scene;

static uiGluePanelDef_t const glue_panels[UI_GLUE_PANEL_COUNT] = {
    [UI_GLUE_MAIN_MENU] = { .left = "MainMenu %s", .right = "MainMenu %s" },
    [UI_GLUE_REALM_SELECTION] = { .left = "RealmSelection %s", .right = "RealmSelection %s" },
    [UI_GLUE_SINGLE_PLAYER] = { .left = "SinglePlayer %s", .right = "SinglePlayer %s" },
    [UI_GLUE_OPTIONS] = { .left = "Options %s Alternate", .right = "Options %s" },
    [UI_GLUE_SINGLE_PLAYER_SKIRMISH] = { .left = "SinglePlayerSkirmish %s", .right = "SinglePlayerSkirmish %s" },
    [UI_GLUE_MULTIPLAYER_PRE_GAME_CHAT] = { .left = "MultiplayerPreGameChat %s", .right = "MultiplayerPreGameChat %s" },
    [UI_GLUE_BATTLENET_CUSTOM] = { .left = "BattlenetCustom %s", .right = "BattlenetCustom %s" },
    [UI_GLUE_BATTLENET_CUSTOM_CREATE] = { .left = "BattlenetCustomCreate %s", .right = "BattlenetCustomCreate %s" },
};

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
    LPCSTR primary;
    DWORD duration, elapsed;

    primary = menu_glue_scene.phase == UI_GLUE_PANEL_IDLE ? "Stand" :
              menu_glue_scene.phase == UI_GLUE_PANEL_EXIT ? "Death" : "Birth";
    snprintf(anim, anim_size, format, primary);
    if (menu_glue_scene.phase == UI_GLUE_PANEL_IDLE) return anim;
    duration = menu_glue_scene.phase == UI_GLUE_PANEL_EXIT ? UI_GLUE_DEATH_TIME : UI_GLUE_BIRTH_TIME;
    elapsed = MIN(M_Time() - menu_glue_scene.phase_start, duration);
    snprintf(anim + strlen(anim), anim_size - strlen(anim), "@%.4f", (FLOAT)elapsed / (FLOAT)duration);
    return anim;
}

void UI_ResetGlueSceneModels(void) {
    memset(&menu_glue_scene, 0, sizeof(menu_glue_scene));
}

void UI_ReleaseGlueSceneModels(void) {
    LPRENDERER renderer = menuimport.GetRenderer();

    if (menu_glue_scene.background) renderer->ReleaseModel((LPMODEL)menu_glue_scene.background);
    if (menu_glue_scene.top_left_panel) renderer->ReleaseModel((LPMODEL)menu_glue_scene.top_left_panel);
    if (menu_glue_scene.top_right_panel) renderer->ReleaseModel((LPMODEL)menu_glue_scene.top_right_panel);
    UI_ResetGlueSceneModels();
}

void UI_PreloadGlueSceneModels(void) {
    LPRENDERER renderer;

    if (menu_glue_scene.loaded) return;
    renderer = menuimport.GetRenderer();
    if (!renderer || !renderer->LoadModel) return;
    menu_glue_scene.background = renderer->LoadModel(UI_GlueBackgroundPath());
    menu_glue_scene.top_left_panel = renderer->LoadModel(UI_GlueTopLeftPanelPath());
    menu_glue_scene.top_right_panel = renderer->LoadModel(UI_GlueTopRightPanelPath());
    menu_glue_scene.loaded = true;
}

/* Panel names are the public transition contract. This owns the authored
 * current Death -> target Birth sequencing, including alternate left layers. */
void UI_GotoGluePanel(uiGluePanel_t panel, uiGluePanelChanged_f changed) {
    LPRENDERER renderer = menuimport.GetRenderer();

    UI_PreloadGlueSceneModels();
    if (!renderer) return;
    if (panel <= UI_GLUE_NONE || panel >= UI_GLUE_PANEL_COUNT) {
        fprintf(stderr, "UI: unknown glue panel %u\n", (unsigned)panel);
        return;
    }
    if (!menu_glue_scene.current) {
        menu_glue_scene.current = panel;
        menu_glue_scene.phase = UI_GLUE_PANEL_ENTER;
        menu_glue_scene.phase_start = M_Time();
        menu_glue_scene.changed = changed;
        return;
    }
    if (panel == menu_glue_scene.current || panel == menu_glue_scene.target) return;
    menu_glue_scene.target = panel;
    menu_glue_scene.phase = UI_GLUE_PANEL_EXIT;
    menu_glue_scene.phase_start = M_Time();
    menu_glue_scene.changed = changed;
}

void UI_CloseGluePanel(uiGluePanelChanged_f changed) {
    LPRENDERER renderer = menuimport.GetRenderer();

    if (!renderer) return;
    if (!menu_glue_scene.current) {
        if (changed) changed();
        return;
    }
    if (menu_glue_scene.phase == UI_GLUE_PANEL_EXIT && !menu_glue_scene.target) return;
    menu_glue_scene.target = UI_GLUE_NONE;
    menu_glue_scene.phase = UI_GLUE_PANEL_EXIT;
    menu_glue_scene.phase_start = M_Time();
    menu_glue_scene.changed = changed;
}

static void UI_GlueFinishExit(void) {
    if (menu_glue_scene.target) {
        menu_glue_scene.current = menu_glue_scene.target;
        menu_glue_scene.target = UI_GLUE_NONE;
        menu_glue_scene.phase = UI_GLUE_PANEL_ENTER;
    } else {
        uiGluePanelChanged_f changed = menu_glue_scene.changed;

        menu_glue_scene.current = UI_GLUE_NONE;
        menu_glue_scene.phase = UI_GLUE_PANEL_IDLE;
        menu_glue_scene.changed = NULL;
        if (changed) changed();
    }
    menu_glue_scene.phase_start = M_Time();
}

static void UI_GlueAdvanceTransition(void) {
    DWORD elapsed = M_Time() - menu_glue_scene.phase_start;

    if (menu_glue_scene.phase == UI_GLUE_PANEL_EXIT && elapsed >= UI_GLUE_DEATH_TIME)
        UI_GlueFinishExit();
    else if (menu_glue_scene.phase == UI_GLUE_PANEL_ENTER && elapsed >= UI_GLUE_BIRTH_TIME) {
        uiGluePanelChanged_f changed = menu_glue_scene.changed;

        menu_glue_scene.phase = UI_GLUE_PANEL_IDLE;
        menu_glue_scene.changed = NULL;
        if (changed) changed();
    }
}

void UI_DrawGlueScene(void) {
    LPRENDERER renderer = menuimport.GetRenderer();
    uiGluePanelDef_t const *panel;
    FLOAT right_offset;
    char left_anim[UI_GLUE_ANIM_NAME];
    char right_anim[UI_GLUE_ANIM_NAME];

    if (!renderer || !menu_glue_scene.current) return;
    UI_PreloadGlueSceneModels();
    UI_GlueAdvanceTransition();
    if (!menu_glue_scene.current) return;
    panel = &glue_panels[menu_glue_scene.current];
    right_offset = UI_GlueRightPanelOffset(renderer);

    if (renderer->RenderFrame && menu_glue_scene.background) {
        renderEntity_t entity = {0};
        entity.model = menu_glue_scene.background;
        entity.scale = 1.0f;
        entity.flags = RF_NO_SHADOW | RF_NO_FOGOFWAR | RF_PORTRAIT_LIGHTING;
        renderer->SetEntityAnimFrame(menu_glue_scene.background, "Stand", &entity);

        viewDef_t viewdef = {0};
        viewdef.viewport = (RECT){0, 0, 1, 1};
        viewdef.rdflags = RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL | RDF_NOFOG | RDF_USE_ENTITY_CAMERA;
        viewdef.num_entities = 1;
        viewdef.entities = &entity;
        renderer->RenderFrame(&viewdef);
    }

    if (renderer->DrawSprite && menu_glue_scene.top_left_panel) {
        LPCSTR anim = UI_GluePanelAnimation(panel->left, left_anim, sizeof(left_anim));
        renderer->DrawSprite(menu_glue_scene.top_left_panel, anim, 0.0f, UI_BASE_HEIGHT);
    }
    if (renderer->DrawSprite && menu_glue_scene.top_right_panel) {
        LPCSTR anim = UI_GluePanelAnimation(panel->right, right_anim, sizeof(right_anim));
        renderer->DrawSprite(menu_glue_scene.top_right_panel, anim, right_offset, UI_BASE_HEIGHT);
    }
}
