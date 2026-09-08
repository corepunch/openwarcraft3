/*
 * scene.c - shared glue background/sprite-layer model cache.
 */

#include "menu_local.h"

#define UI_GLUE_ANIM_NAME 96 // chars; fits Blizzard glue sequence names and suffixes; used as animation storage.
#define UI_GLUE_BIRTH_TIME 1000 // ms; every named RoC/TFT panel Birth interval has this length.
#define UI_GLUE_DEATH_TIME 667 // ms; every named RoC/TFT panel Death interval rounds to this length.

#define UI_GLUE_MAX_TABS 4

typedef struct {
    LPCSTR name;
    LPCSTR tabs[UI_GLUE_MAX_TABS];
    int tab_count;
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
    int active_tab;       // index into current panel's tabs[], 0 = default
    int target_tab;       // >=0 while morphing to a new tab; -1 when idle
    DWORD tab_start;      // timestamp when the current tab morph began
    uiGluePanelPhase_t phase;
    DWORD phase_start;
    uiGluePanelChanged_f exited;
    uiGluePanelChanged_f changed;
} uiGlueSceneState_t;

static uiGlueSceneState_t scene;

static uiGluePanelDef_t const glue_panels[UI_GLUE_PANEL_COUNT] = {
    [UI_GLUE_MAIN_MENU] = {
        "MainMenu", {"MainMenu"}, 1
    },
    [UI_GLUE_REALM_SELECTION] = {
        "RealmSelection", {"RealmSelection"}, 1
    },
    [UI_GLUE_SINGLE_PLAYER] = {
        "SinglePlayer", {"SinglePlayer", "SinglePlayerSkirmish"}, 2
    },
    [UI_GLUE_OPTIONS] = {
        /* tabs[1] shares the panel name; UI_GlueLeftAnimation uses Stand Alternate / Morph */
        "Options", {"Options", "Options"}, 2
    },
    [UI_GLUE_MULTIPLAYER_PRE_GAME_CHAT] = {
        "MultiplayerPreGameChat", {"MultiplayerPreGameChat", "MultiplayerSubmenu"}, 2
    },
    [UI_GLUE_BATTLENET_CUSTOM] = {
        "BattlenetCustom", {"BattlenetCustom", "BattlenetCustomCreate", "BattlenetAdvancedOptions"}, 3
    },
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

/* Both panel models share the same phase clock; the sequence name is derived only when drawing.
 * Takes the panel name directly ("MainMenu", "Options", …) and appends the phase word. */
static LPCSTR UI_GluePanelAnimation(LPCSTR name, LPSTR anim, DWORD anim_size) {
    DWORD duration, elapsed;

    snprintf(anim, anim_size, "%s %s", name, phases[scene.phase]);
    if (scene.phase == UI_GLUE_PANEL_IDLE) return anim;
    duration = durations[scene.phase];
    elapsed = MIN(M_Time() - scene.phase_start, duration);
    snprintf(anim + strlen(anim), anim_size - strlen(anim), "@%.4f", (FLOAT)elapsed / (FLOAT)duration);
    return anim;
}

/* Left panel animation.  During full panel Birth/Death both layers match.
 * While a tab morph is running the left plays the appropriate Morph sequence;
 * at rest it shows the active tab's standing state. */
static LPCSTR UI_GlueLeftAnimation(uiGluePanelDef_t const *panel, LPSTR anim, DWORD anim_size) {
    LPCSTR tab_name;
    LPCSTR seq;
    DWORD duration, elapsed;
    int featured;

    if (scene.phase != UI_GLUE_PANEL_IDLE)
        return UI_GluePanelAnimation(panel->name, anim, anim_size);

    if (scene.target_tab >= 0) {
        /* Morph in progress.  The sequence name always belongs to the non-default
         * tab involved: Morph when entering it, Morph Alternate when leaving. */
        featured  = scene.target_tab > 0 ? scene.target_tab : scene.active_tab;
        tab_name  = panel->tabs[featured];
        if (scene.target_tab > 0) {
            seq      = "Morph";
            duration = UI_GLUE_BIRTH_TIME;
        } else {
            seq      = "Morph Alternate";
            duration = UI_GLUE_DEATH_TIME;
        }
        snprintf(anim, anim_size, "%s %s", tab_name, seq);
        elapsed = MIN(M_Time() - scene.tab_start, duration);
        snprintf(anim + strlen(anim), anim_size - strlen(anim), "@%.4f", (FLOAT)elapsed / (FLOAT)duration);
        return anim;
    }

    if (scene.active_tab == 0) {
        snprintf(anim, anim_size, "%s Stand", panel->name);
        return anim;
    }
    tab_name = panel->tabs[scene.active_tab];
    if (strcmp(tab_name, panel->name) == 0)
        snprintf(anim, anim_size, "%s Stand Alternate", panel->name);
    else
        snprintf(anim, anim_size, "%s Stand", tab_name);
    return anim;
}

#ifdef WC3_DEBUG_GLUE
/* Log only phase boundaries; animation strings otherwise change every rendered frame. */
static void UI_GlueDebugPhase(LPCSTR event) {
    char left[UI_GLUE_ANIM_NAME], right[UI_GLUE_ANIM_NAME];
    uiGluePanelDef_t const *panel = scene.current ? &glue_panels[scene.current] : NULL;

    if (!panel) {
        fprintf(stderr, "WC3 glue: %s current=none target=%u phase=%u\n", event,
                (unsigned)scene.target, (unsigned)scene.phase);
        return;
    }
    UI_GlueLeftAnimation(panel, left, sizeof(left));
    UI_GluePanelAnimation(panel->name, right, sizeof(right));
    fprintf(stderr, "WC3 glue: %s current=%u target=%u phase=%s tab=%d/%d left=\"%s\" right=\"%s\"\n", event,
            (unsigned)scene.current, (unsigned)scene.target, phases[scene.phase],
            scene.active_tab, scene.target_tab, left, right);
}
#else
#define UI_GlueDebugPhase(EVENT) ((void)0)
#endif


void UI_ResetGlueSceneModels(void) {
    memset(&scene, 0, sizeof(scene));
    scene.target_tab = -1;
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
        UI_GlueDebugPhase("begin");
        return;
    }
    if (panel == scene.current || panel == scene.target) return;
    scene.target = panel;
    scene.phase = UI_GLUE_PANEL_EXIT;
    scene.phase_start = M_Time();
    scene.target_tab = -1;  // abort any in-flight tab morph; tab is set fresh on ENTER
    scene.exited = exited;
    scene.changed = changed;
    UI_GlueDebugPhase("exit");
}

/* Command-line menu overrides replace the queued default before its first transition completes. */
void UI_RetargetGluePanel(uiGluePanel_t panel, uiGluePanelChanged_f exited, uiGluePanelChanged_f changed) {
    if (panel <= UI_GLUE_NONE || panel >= UI_GLUE_PANEL_COUNT) {
        fprintf(stderr, "UI: unknown glue panel %u\n", (unsigned)panel);
        return;
    }
    scene.current = panel;
    scene.target = UI_GLUE_NONE;
    scene.phase = UI_GLUE_PANEL_ENTER;
    scene.phase_start = M_Time();
    scene.target_tab = -1;
    scene.exited = NULL;
    scene.changed = changed;
    if (exited) exited();
    UI_GlueDebugPhase("retarget");
}

/* Switch the left panel to a specific tab within the current panel.
 * tab == 0 returns to the default tab.  If the main panel is mid-transition
 * the new tab is applied directly so Birth plays with the correct state. */
void UI_SetGlueTab(int tab) {
    uiGluePanelDef_t const *panel;

    if (!scene.current) {
        scene.active_tab = tab > 0 ? tab : 0;
        scene.target_tab = -1;
        return;
    }
    panel = &glue_panels[scene.current];
    if (tab < 0 || tab >= panel->tab_count)
        tab = 0;
    if (tab == scene.active_tab && scene.target_tab < 0)
        return;
    if (scene.phase != UI_GLUE_PANEL_IDLE) {
        scene.active_tab = tab;
        scene.target_tab = -1;
        return;
    }
    scene.target_tab = tab;
    scene.tab_start = M_Time();
    UI_GlueDebugPhase(tab ? "tab-enter" : "tab-leave");
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
    UI_GlueDebugPhase("close");
}

static void UI_GlueFinishExit(void) {
    uiGluePanelChanged_f exited = scene.exited;

    scene.exited = NULL;
    if (scene.target) {
        scene.current = scene.target;
        scene.target = UI_GLUE_NONE;
        scene.phase = UI_GLUE_PANEL_ENTER;
        scene.active_tab = 0;
    } else {
        uiGluePanelChanged_f changed = scene.changed;

        scene.current = UI_GLUE_NONE;
        scene.phase = UI_GLUE_PANEL_IDLE;
        scene.changed = NULL;
        if (changed) changed();
    }
    scene.phase_start = M_Time();
    UI_GlueDebugPhase(scene.current ? "enter" : "closed");
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
        UI_GlueDebugPhase("idle");
        if (changed) changed();
    }
}

static void UI_GlueAdvanceTabTransition(void) {
    uiGluePanelDef_t const *panel;
    DWORD duration, elapsed;

    if (scene.target_tab < 0) return;
    panel = scene.current ? &glue_panels[scene.current] : NULL;
    if (!panel) return;
    duration = scene.target_tab > 0 ? UI_GLUE_BIRTH_TIME : UI_GLUE_DEATH_TIME;
    elapsed  = M_Time() - scene.tab_start;
    if (elapsed >= duration) {
        scene.active_tab = scene.target_tab;
        scene.target_tab = -1;
        UI_GlueDebugPhase("tab-idle");
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
    if (scene.phase == UI_GLUE_PANEL_IDLE)
        UI_GlueAdvanceTabTransition();
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
        renderer->DrawSprite(scene.top_left_panel,
                             UI_GlueLeftAnimation(panel, left_anim, sizeof(left_anim)),
                             0.0f, UI_BASE_HEIGHT);
    }
    if (renderer->DrawSprite && scene.top_right_panel) {
        renderer->DrawSprite(scene.top_right_panel,
                             UI_GluePanelAnimation(panel->name, right_anim, sizeof(right_anim)),
                             right_offset, UI_BASE_HEIGHT);
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
