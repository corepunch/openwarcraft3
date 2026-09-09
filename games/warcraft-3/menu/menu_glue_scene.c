/*
 * scene.c - shared glue background/sprite-layer model cache.
 */

#include "menu_local.h"

#define UI_GLUE_ANIM_NAME 96 // chars; fits Blizzard glue sequence names and suffixes; used as animation storage.
#define UI_GLUE_BIRTH_TIME 1000 // ms; every named RoC/TFT panel Birth interval has this length.
#define UI_GLUE_DEATH_TIME 667 // ms; every named RoC/TFT panel Death interval rounds to this length.

#define BZ_GLUE_MAX_TABS 3 // tabs; largest authored group is custom-game browser/create/options.

typedef struct { LPCSTR stand, enter, leave; } GLUETAB;
typedef GLUETAB *LPGLUETAB;
typedef const GLUETAB *LPCGLUETAB;
typedef struct { LPCSTR name; GLUETAB tabs[BZ_GLUE_MAX_TABS]; } GLUEPANEL;
typedef GLUEPANEL *LPGLUEPANEL;
typedef const GLUEPANEL *LPCGLUEPANEL;

typedef enum {
    UI_GLUE_PANEL_IDLE,
    UI_GLUE_PANEL_EXIT,
    UI_GLUE_PANEL_ENTER,
} uiGluePanelPhase_t;

typedef struct {
    BOOL loaded;
    LPCMODEL background, top_left_panel, top_right_panel;
    GLUEDEST current, target;
    int morph; // destination of the running tab animation; -1 when idle
    DWORD tab_start, phase_start;
    uiGluePanelPhase_t phase;
    uiGluePanelChanged_f exited, changed;
} GLUESCENE;
typedef GLUESCENE *LPGLUESCENE;
typedef const GLUESCENE *LPCGLUESCENE;

static GLUESCENE scene;

/* Sequence families differ in the MDX: Create has Birth/Death, while morph-only
 * tabs retain their authored final pose instead of inventing a Stand sequence. */
static const GLUEPANEL glue_panels[UI_GLUE_PANEL_COUNT] = {
    [UI_GLUE_MAIN_MENU] = { .name = "MainMenu" },
    [UI_GLUE_REALM_SELECTION] = { .name = "RealmSelection" },
    [UI_GLUE_SINGLE_PLAYER] = { .name = "SinglePlayer", .tabs = {
        [1] = { "SinglePlayerSkirmish Stand", "SinglePlayerSkirmish Morph", "SinglePlayerSkirmish Morph Alternate" },
    } },
    [UI_GLUE_OPTIONS] = { .name = "Options", .tabs = {
        [1] = { "Options Stand Alternate", "Options Morph", "Options Morph Alternate" },
    } },
    [UI_GLUE_MULTIPLAYER_PRE_GAME_CHAT] = { .name = "MultiplayerPreGameChat", .tabs = {
        [1] = { "MultiplayerSubmenu Morph@1.0000", "MultiplayerSubmenu Morph", "MultiplayerSubmenu Morph Alternate" },
    } },
    [UI_GLUE_BATTLENET_CUSTOM] = { .name = "BattlenetCustom", .tabs = {
        [1] = { "BattlenetCustomCreate Stand", "BattlenetCustomCreate Birth", "BattlenetCustomCreate Death" },
        [2] = { "BattlenetAdvancedOptions Morph@1.0000", "BattlenetAdvancedOptions Morph", "BattlenetAdvancedOptions Morph Alternate" },
    } },
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

/* Both clocks use the renderer's normalized sequence-time contract. */
static void UI_GlueProgress(LPSTR anim, DWORD start, DWORD duration) {
    size_t len = strlen(anim);
    snprintf(anim + len, UI_GLUE_ANIM_NAME - len, "@%.4f", (FLOAT)MIN(M_Time() - start, duration) / duration);
}

static LPCSTR UI_GluePanelAnimation(LPCSTR name, LPSTR anim) {
    snprintf(anim, UI_GLUE_ANIM_NAME, "%s %s", name, phases[scene.phase]);
    if (scene.phase != UI_GLUE_PANEL_IDLE)
        UI_GlueProgress(anim, scene.phase_start, durations[scene.phase]);
    return anim;
}

/* The right layer remains at Stand while the left changes tabs. */
static LPCSTR UI_GlueLeftAnimation(LPCGLUEPANEL panel, LPSTR anim) {
    if (scene.phase != UI_GLUE_PANEL_IDLE || (!scene.current.tab && scene.morph < 0))
        return UI_GluePanelAnimation(panel->name, anim);
    if (scene.morph < 0) {
        snprintf(anim, UI_GLUE_ANIM_NAME, "%s", panel->tabs[scene.current.tab].stand);
    } else {
        LPCGLUETAB tab = &panel->tabs[scene.morph ? scene.morph : scene.current.tab];
        snprintf(anim, UI_GLUE_ANIM_NAME, "%s", scene.morph ? tab->enter : tab->leave);
        UI_GlueProgress(anim, scene.tab_start, scene.morph ? UI_GLUE_BIRTH_TIME : UI_GLUE_DEATH_TIME);
    }
    return anim;
}

#ifdef WC3_DEBUG_GLUE
/* Log only phase boundaries; animation strings otherwise change every rendered frame. */
static void UI_GlueDebugPhase(LPCSTR event) {
    char left[UI_GLUE_ANIM_NAME], right[UI_GLUE_ANIM_NAME];
    LPCGLUEPANEL panel = scene.current.panel ? &glue_panels[scene.current.panel] : NULL;

    if (!panel) {
        fprintf(stderr, "WC3 glue: %s current=none target=%u phase=%u\n", event,
                (unsigned)scene.target.panel, (unsigned)scene.phase);
        return;
    }
    UI_GlueLeftAnimation(panel, left);
    UI_GluePanelAnimation(panel->name, right);
    fprintf(stderr, "WC3 glue: %s current=%u target=%u phase=%s tab=%d/%d left=\"%s\" right=\"%s\"\n", event,
            (unsigned)scene.current.panel, (unsigned)scene.target.panel, phases[scene.phase],
            scene.current.tab, scene.morph, left, right);
}
#else
#define UI_GlueDebugPhase(EVENT) ((void)0)
#endif


void UI_ResetGlueSceneModels(void) {
    memset(&scene, 0, sizeof(scene));
    scene.morph = -1;
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

/* Finish an in-flight morph before honoring a newer request. Non-default tabs
 * leave through tab zero; restarting a Morph midway used to snap its pose. */
static void UI_GlueAdvanceTabTransition(void) {
    DWORD duration = scene.morph > 0 ? UI_GLUE_BIRTH_TIME : UI_GLUE_DEATH_TIME;

    if (scene.morph >= 0 && M_Time() - scene.tab_start >= duration) {
        scene.current.tab = scene.morph;
        scene.morph = -1;
        UI_GlueDebugPhase("tab-idle");
    }
    if (scene.morph < 0 && scene.current.tab != scene.target.tab) {
        scene.morph = scene.current.tab ? 0 : scene.target.tab;
        scene.tab_start = M_Time();
        UI_GlueDebugPhase("tab-begin");
    }
}

/* A destination is atomic: validate its tab against its own panel, and retain
 * it through Death. Same-panel requests must deliver completion, too. */
void UI_GotoGluePanel(GLUEDEST dest, uiGluePanelChanged_f exited, uiGluePanelChanged_f changed) {
    if (dest.panel < UI_GLUE_NONE || dest.panel >= UI_GLUE_PANEL_COUNT || dest.tab < 0 ||
        dest.tab >= BZ_GLUE_MAX_TABS || (dest.tab && !glue_panels[dest.panel].tabs[dest.tab].stand)) {
        fprintf(stderr, "UI: invalid glue destination %u/%d\n", (unsigned)dest.panel, dest.tab);
        return;
    }
    if (dest.panel) UI_PreloadGlueSceneModels();
    scene.target = dest;
    scene.exited = NULL;
    scene.changed = changed;
    if (dest.panel == scene.current.panel && scene.phase != UI_GLUE_PANEL_EXIT) {
        if (scene.phase == UI_GLUE_PANEL_ENTER) scene.current.tab = dest.tab;
        else UI_GlueAdvanceTabTransition();
        BOOL done = scene.phase == UI_GLUE_PANEL_IDLE;
        if (done) scene.changed = NULL;
        if (exited) exited();
        if (done && changed) changed();
        return;
    }
    scene.morph = -1;
    if (!scene.current.panel || (dest.panel && scene.phase == UI_GLUE_PANEL_ENTER)) {
        /* Startup overrides replace the pending Birth without an extra Death. */
        scene.current = dest;
        scene.phase = dest.panel ? UI_GLUE_PANEL_ENTER : UI_GLUE_PANEL_IDLE;
        scene.phase_start = M_Time();
        if (!dest.panel) scene.changed = NULL;
        UI_GlueDebugPhase("begin");
        if (exited) exited();
        if (!dest.panel && changed) changed();
        return;
    }
    if (scene.phase != UI_GLUE_PANEL_EXIT) scene.phase_start = M_Time();
    scene.phase = UI_GLUE_PANEL_EXIT;
    scene.exited = exited;
    UI_GlueDebugPhase("exit");
}

void UI_CloseGluePanel(uiGluePanelChanged_f changed) { UI_GotoGluePanel((GLUEDEST){0}, NULL, changed); }

/* Clear callbacks and commit the complete destination before invoking screen
 * code; a callback may immediately request another transition. */
static void UI_GlueAdvanceTransition(void) {
    if (scene.phase == UI_GLUE_PANEL_IDLE || M_Time() - scene.phase_start < durations[scene.phase]) return;
    uiGluePanelChanged_f exited = scene.exited, changed = scene.changed;
    scene.exited = NULL;
    if (scene.phase == UI_GLUE_PANEL_EXIT) {
        scene.current = scene.target;
        scene.phase = scene.current.panel ? UI_GLUE_PANEL_ENTER : UI_GLUE_PANEL_IDLE;
    } else scene.phase = UI_GLUE_PANEL_IDLE;
    scene.phase_start = M_Time();
    BOOL done = scene.phase == UI_GLUE_PANEL_IDLE;
    if (done) scene.changed = NULL;
    UI_GlueDebugPhase(done ? "idle" : "enter");
    if (exited) exited();
    if (done && changed) changed();
}

void UI_DrawGlueScene(void) {
    LPRENDERER renderer = mi.GetRenderer();
    LPCGLUEPANEL panel;
    FLOAT right_offset;
    char left_anim[UI_GLUE_ANIM_NAME];
    char right_anim[UI_GLUE_ANIM_NAME];

    if (!renderer || !scene.current.panel) return;
    UI_PreloadGlueSceneModels();
    UI_GlueAdvanceTransition();
    if (scene.phase == UI_GLUE_PANEL_IDLE)
        UI_GlueAdvanceTabTransition();
    if (!scene.current.panel) return;
    panel = &glue_panels[scene.current.panel];
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
                             UI_GlueLeftAnimation(panel, left_anim),
                             0.0f, UI_BASE_HEIGHT);
    }
    if (renderer->DrawSprite && scene.top_right_panel) {
        renderer->DrawSprite(scene.top_right_panel,
                             UI_GluePanelAnimation(panel->name, right_anim),
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
