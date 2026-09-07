/*
 * menu_glue_scene.c - shared glue background/sprite-layer model cache.
 */

#include "menu_local.h"

#define UI_GLUE_ANIM_NAME 96 // chars; fits Blizzard glue sequence names and suffixes; used as animation storage.
#define UI_GLUE_BIRTH_TIME 1000 // ms; every named RoC/TFT panel Birth interval has this length.
#define UI_GLUE_DEATH_TIME 667 // ms; every named RoC/TFT panel Death interval rounds to this length.

typedef struct {
    LPCSTR name;
    LPCSTR left;
    LPCSTR right;
} uiGluePanel_t;

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
    uiGluePanel_t const *current;
    uiGluePanel_t const *target;
    uiGluePanelPhase_t phase;
    DWORD phase_start;
    uiGluePanelChanged_f changed;
} uiGlueSceneState_t;

static uiGlueSceneState_t menu_glue_scene;

static uiGluePanel_t const glue_panels[] = {
    { "MainMenu", "MainMenu Stand", "MainMenu Stand" },
    { "RealmSelection", "RealmSelection Stand", "RealmSelection Stand" },
    { "SinglePlayer", "SinglePlayer Stand", "SinglePlayer Stand" },
    { "Options", "Options Stand Alternate", "Options Stand" },
    { "SinglePlayerSkirmish", "SinglePlayerSkirmish Stand", "SinglePlayerSkirmish Stand" },
    { "MultiplayerPreGameChat", "MultiplayerPreGameChat Stand", "MultiplayerPreGameChat Stand" },
    { "BattlenetCustom", "BattlenetCustom Stand", "BattlenetCustom Stand" },
    { "BattlenetCustomCreate", "BattlenetCustomCreate Stand", "BattlenetCustomCreate Stand" },
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

/* Stable glue states are named "<screen> Stand". Retail/Warsmash enters them
 * through the matching non-looping Birth sequence; preserve any suffix such as
 * "Alternate" while swapping the primary tag. */
static BOOL UI_GlueSequenceForStand(LPCSTR stand, LPCSTR primary, LPSTR sequence, DWORD sequence_size) {
    LPCSTR marker;
    size_t prefix;

    if (!stand || !primary || !*primary || !sequence || sequence_size == 0) return false;
    if (!strcmp(stand, "Stand")) {
        snprintf(sequence, sequence_size, "%s", primary);
        return true;
    }
    marker = strstr(stand, " Stand");
    if (!marker) return false;
    prefix = (size_t)(marker - stand);
    if (prefix + strlen(primary) + strlen(marker + strlen(" Stand")) + 3 > sequence_size) {
        return false;
    }
    memcpy(sequence, stand, prefix);
    sequence[prefix] = '\0';
    strncat(sequence, " ", sequence_size - strlen(sequence) - 1);
    strncat(sequence, primary, sequence_size - strlen(sequence) - 1);
    strncat(sequence, marker + strlen(" Stand"), sequence_size - strlen(sequence) - 1);
    return true;
}

static uiGluePanel_t const *UI_GluePanel(LPCSTR name) {
    if (!name || !*name) return NULL;
    FOR_LOOP(i, sizeof(glue_panels) / sizeof(glue_panels[0])) if (!strcmp(glue_panels[i].name, name)) return &glue_panels[i];
    return NULL;
}

/* Both panel models use the same fixed intervals, so one phase clock drives
 * both layers and the sequence name is derived only when drawing. */
static LPCSTR UI_GluePanelAnimation(LPCSTR stand, LPSTR anim, DWORD anim_size) {
    LPCSTR primary;
    DWORD duration, elapsed;

    if (menu_glue_scene.phase == UI_GLUE_PANEL_IDLE) return stand;
    primary = menu_glue_scene.phase == UI_GLUE_PANEL_EXIT ? "Death" : "Birth";
    duration = menu_glue_scene.phase == UI_GLUE_PANEL_EXIT ? UI_GLUE_DEATH_TIME : UI_GLUE_BIRTH_TIME;
    elapsed = MIN(M_Time() - menu_glue_scene.phase_start, duration);
    if (!UI_GlueSequenceForStand(stand, primary, anim, anim_size)) {
        fprintf(stderr, "UI: glue panel sequence has no Stand primary: %s\n", stand);
        return stand;
    }
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
void UI_GotoGluePanel(LPCSTR panel_name, uiGluePanelChanged_f changed) {
    LPRENDERER renderer = menuimport.GetRenderer();
    uiGluePanel_t const *panel = UI_GluePanel(panel_name);

    UI_PreloadGlueSceneModels();
    if (!renderer) return;
    if (!panel) {
        fprintf(stderr, "UI: unknown glue panel %s\n", panel_name ? panel_name : "(null)");
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
    menu_glue_scene.target = NULL;
    menu_glue_scene.phase = UI_GLUE_PANEL_EXIT;
    menu_glue_scene.phase_start = M_Time();
    menu_glue_scene.changed = changed;
}

static void UI_GlueFinishExit(void) {
    if (menu_glue_scene.target) {
        menu_glue_scene.current = menu_glue_scene.target;
        menu_glue_scene.target = NULL;
        menu_glue_scene.phase = UI_GLUE_PANEL_ENTER;
    } else {
        uiGluePanelChanged_f changed = menu_glue_scene.changed;

        menu_glue_scene.current = NULL;
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
    FLOAT right_offset;
    char left_anim[UI_GLUE_ANIM_NAME];
    char right_anim[UI_GLUE_ANIM_NAME];

    if (!renderer || !menu_glue_scene.current) return;
    UI_PreloadGlueSceneModels();
    UI_GlueAdvanceTransition();
    if (!menu_glue_scene.current) return;
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
        LPCSTR anim = UI_GluePanelAnimation(menu_glue_scene.current->left, left_anim, sizeof(left_anim));
        renderer->DrawSprite(menu_glue_scene.top_left_panel, anim, 0.0f, UI_BASE_HEIGHT);
    }
    if (renderer->DrawSprite && menu_glue_scene.top_right_panel) {
        LPCSTR anim = UI_GluePanelAnimation(menu_glue_scene.current->right, right_anim, sizeof(right_anim));
        renderer->DrawSprite(menu_glue_scene.top_right_panel, anim, right_offset, UI_BASE_HEIGHT);
    }
}
