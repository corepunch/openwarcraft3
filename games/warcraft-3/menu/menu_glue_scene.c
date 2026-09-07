/*
 * menu_glue_scene.c - shared glue background/sprite-layer model cache.
 */

#include "menu_local.h"

#define UI_GLUE_ANIM_NAME 96 // chars; fits Blizzard glue sequence names and suffixes; used as animation storage.

typedef struct {
    char active[UI_GLUE_ANIM_NAME];
    char next[UI_GLUE_ANIM_NAME];
    DWORD start_time;
    DWORD duration;
} uiGlueAnimState_t;

typedef struct {
    BOOL loaded;
    LPCMODEL background;
    LPCMODEL top_left_panel;
    LPCMODEL top_right_panel;
    uiGlueAnimState_t background_anim;
    uiGlueAnimState_t left_anim;
    uiGlueAnimState_t right_anim;
    uiGlueAnimationFinished_f finished;
    void *finished_params;
} uiGlueSceneState_t;

static uiGlueSceneState_t menu_glue_scene;

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

static BOOL UI_GlueSequenceDuration(LPRENDERER renderer, LPCMODEL model, LPCSTR anim, LPDWORD duration) {
    if (!model || !anim || !*anim || !duration) return false;
    return renderer->GetModelAnimationDuration(model, anim, duration) && *duration > 0;
}

static BOOL UI_GlueIsOneShot(LPCSTR anim) {
    if (!anim || !*anim) return false;
    return !strcmp(anim, "Birth") || !strcmp(anim, "Death") ||
           strstr(anim, " Birth") != NULL || strstr(anim, " Death") != NULL;
}

/* Stable glue states are named "<screen> Stand". Retail/Warsmash enters them
 * through the matching non-looping Birth sequence; preserve any suffix such as
 * "Alternate" while swapping the primary tag. */
static BOOL UI_GlueBirthForStand(LPCSTR stand, LPSTR birth, DWORD birth_size) {
    LPCSTR marker;
    size_t prefix;

    if (!stand || !birth || birth_size == 0) return false;
    if (!strcmp(stand, "Stand")) {
        snprintf(birth, birth_size, "Birth");
        return true;
    }
    marker = strstr(stand, " Stand");
    if (!marker) return false;
    prefix = (size_t)(marker - stand);
    if (prefix + strlen(" Birth") + strlen(marker + strlen(" Stand")) + 1 > birth_size) {
        return false;
    }
    memcpy(birth, stand, prefix);
    birth[prefix] = '\0';
    strncat(birth, " Birth", birth_size - strlen(birth) - 1);
    strncat(birth, marker + strlen(" Stand"), birth_size - strlen(birth) - 1);
    return true;
}

/* A glue track either advances an authored one-shot into its stable next
 * sequence or holds the one-shot's final pose. The renderer supplies only the
 * source timing; screen controllers never observe it. */
static void UI_GlueStartAnimation(LPRENDERER renderer, LPCMODEL model, LPCSTR active, LPCSTR next,
                                  uiGlueAnimState_t *state) {
    memset(state, 0, sizeof(*state));
    snprintf(state->active, sizeof(state->active), "%s", active);
    snprintf(state->next, sizeof(state->next), "%s", next);
    state->start_time = M_Time();
    if (!UI_GlueSequenceDuration(renderer, model, active, &state->duration))
        snprintf(state->active, sizeof(state->active), "%s", next);
}

static void UI_GlueBeginLayerAnimation(LPRENDERER renderer, LPCMODEL model, LPCSTR requested,
                                       uiGlueAnimState_t *state) {
    char birth[UI_GLUE_ANIM_NAME];

    if (!requested || !*requested) requested = "Stand";
    if (UI_GlueBirthForStand(requested, birth, sizeof(birth))) {
        UI_GlueStartAnimation(renderer, model, birth, requested, state);
        return;
    }
    UI_GlueStartAnimation(renderer, model, requested, requested, state);
    if (!UI_GlueIsOneShot(requested)) state->duration = 0;
}

static LPCSTR UI_GlueAnimationFrame(uiGlueAnimState_t *state, LPSTR scrubbed, DWORD scrubbed_size,
                                    BOOL *complete) {
    DWORD elapsed;
    FLOAT ratio;

    *complete = true;
    if (!state->duration) return state->active;
    elapsed = M_Time() - state->start_time;
    if (elapsed >= state->duration) {
        if (strcmp(state->active, state->next)) return state->next;
        snprintf(scrubbed, scrubbed_size, "%s@1.0000", state->active);
        return scrubbed;
    }

    ratio = (FLOAT)elapsed / (FLOAT)state->duration;
    snprintf(scrubbed, scrubbed_size, "%s@%.4f", state->active, ratio);
    *complete = false;
    return scrubbed;
}

static void UI_GlueSetLayerAnimation(LPRENDERER renderer, LPCMODEL model, LPCSTR requested,
                                     uiGlueAnimState_t *state) {
    if (!requested || !*requested || !strcmp(state->next, requested)) return;
    UI_GlueBeginLayerAnimation(renderer, model, requested, state);
}

static void UI_GlueRestartBackground(LPRENDERER renderer) {
    UI_GlueStartAnimation(renderer, menu_glue_scene.background, "Birth", "Stand", &menu_glue_scene.background_anim);
}

void UI_ResetGlueSceneModels(void) {
    memset(&menu_glue_scene, 0, sizeof(menu_glue_scene));
}

void UI_RestartGlueScene(void) {
    LPRENDERER renderer = menuimport.GetRenderer();

    if (!renderer) return;
    UI_GlueRestartBackground(renderer);
    memset(&menu_glue_scene.left_anim, 0, sizeof(menu_glue_scene.left_anim));
    memset(&menu_glue_scene.right_anim, 0, sizeof(menu_glue_scene.right_anim));
    menu_glue_scene.finished = NULL;
    menu_glue_scene.finished_params = NULL;
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
    UI_RestartGlueScene();
}

/* Finish callbacks replace draw-loop completion polling. They run only after
 * both panel layers reached their final authored frame. */
void UI_PlayGlueAnimation(LPCSTR panel_anim, uiGlueAnimationFinished_f finished, void *params) {
    LPRENDERER renderer = menuimport.GetRenderer();

    UI_PreloadGlueSceneModels();
    if (!renderer) return;
    UI_GlueBeginLayerAnimation(renderer, menu_glue_scene.top_left_panel, panel_anim, &menu_glue_scene.left_anim);
    UI_GlueBeginLayerAnimation(renderer, menu_glue_scene.top_right_panel, panel_anim, &menu_glue_scene.right_anim);
    menu_glue_scene.finished = finished;
    menu_glue_scene.finished_params = params;
}

void UI_DrawGlueSceneLayers(LPCSTR left_panel_anim, LPCSTR right_panel_anim) {
    LPRENDERER renderer = menuimport.GetRenderer();
    FLOAT right_offset;
    char background_anim[UI_GLUE_ANIM_NAME];
    char left_anim[UI_GLUE_ANIM_NAME];
    char right_anim[UI_GLUE_ANIM_NAME];
    BOOL left_complete, right_complete;

    if (!renderer) return;
    UI_PreloadGlueSceneModels();
    right_offset = UI_GlueRightPanelOffset(renderer);
    if (left_panel_anim) UI_GlueSetLayerAnimation(renderer, menu_glue_scene.top_left_panel, left_panel_anim, &menu_glue_scene.left_anim);
    if (right_panel_anim) UI_GlueSetLayerAnimation(renderer, menu_glue_scene.top_right_panel, right_panel_anim, &menu_glue_scene.right_anim);

    if (renderer->RenderFrame && menu_glue_scene.background) {
        renderEntity_t entity = {0};
        LPCSTR anim = UI_GlueAnimationFrame(&menu_glue_scene.background_anim, background_anim, sizeof(background_anim), &left_complete);
        entity.model = menu_glue_scene.background;
        entity.scale = 1.0f;
        entity.flags = RF_NO_SHADOW | RF_NO_FOGOFWAR | RF_PORTRAIT_LIGHTING;
        renderer->SetEntityAnimFrame(menu_glue_scene.background, anim, &entity);

        viewDef_t viewdef = {0};
        viewdef.viewport = (RECT){0, 0, 1, 1};
        viewdef.rdflags = RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL | RDF_NOFOG | RDF_USE_ENTITY_CAMERA;
        viewdef.num_entities = 1;
        viewdef.entities = &entity;
        renderer->RenderFrame(&viewdef);
    }

    left_complete = right_complete = true;
    if (renderer->DrawSprite && menu_glue_scene.top_left_panel) {
        LPCSTR anim = UI_GlueAnimationFrame(&menu_glue_scene.left_anim, left_anim, sizeof(left_anim), &left_complete);
        renderer->DrawSprite(menu_glue_scene.top_left_panel, anim, 0.0f, UI_BASE_HEIGHT);
    }
    if (renderer->DrawSprite && menu_glue_scene.top_right_panel) {
        LPCSTR anim = UI_GlueAnimationFrame(&menu_glue_scene.right_anim, right_anim, sizeof(right_anim), &right_complete);
        renderer->DrawSprite(menu_glue_scene.top_right_panel, anim, right_offset, UI_BASE_HEIGHT);
    }
    if (left_complete && right_complete && menu_glue_scene.finished) {
        uiGlueAnimationFinished_f finished = menu_glue_scene.finished;
        void *params = menu_glue_scene.finished_params;

        menu_glue_scene.finished = NULL;
        menu_glue_scene.finished_params = NULL;
        finished(params);
    }
}

void UI_DrawGlueScene(LPCSTR panel_anim) {
    UI_DrawGlueSceneLayers(panel_anim, panel_anim);
}
