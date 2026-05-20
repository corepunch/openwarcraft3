/*
 * console_ui.c — In-game HUD screen (ConsoleUI, ResourceBar, command cards)
 */

#include "../ui_local.h"
#include "../ui_screen.h"

/* Forward declaration to avoid implicit declaration errors */
void UI_DrawFrame(LPCFRAMEDEF frame);

/* Root frame for ConsoleUI hierarchy */
static LPFRAMEDEF root_frame = NULL;

/* Key child frames for quick access */
static LPFRAMEDEF resource_bar = NULL;
static LPFRAMEDEF cinematic_panel = NULL;

/* Cached unit UI data (Phase 8) */
#define MAX_CACHED_UNITS 12
static uiUnitData_t cached_units[MAX_CACHED_UNITS];
static DWORD cached_unit_count = 0;

static void ConsoleUI_Init(void) {
    uiimport.Printf("ConsoleUI_Init: initializing in-game HUD\n");
    
    /* Find root ConsoleUI frame from parsed FDF */
    root_frame = UI_FindFrame("ConsoleUI");
    if (!root_frame) {
        uiimport.Error("ConsoleUI_Init: ConsoleUI frame not found\n");
        return;
    }
    
    /* Find ResourceBar for quick access */
    resource_bar = UI_FindFrame("ResourceBar");
    if (!resource_bar) {
        uiimport.Printf("ConsoleUI_Init: ResourceBar not found\n");
    }
    
    /* Find CinematicPanel for dialogue text */
    cinematic_panel = UI_FindFrame("CinematicPanel");
    if (!cinematic_panel) {
        uiimport.Printf("ConsoleUI_Init: CinematicPanel not found\n");
    }
    
    uiimport.Printf("ConsoleUI_Init: complete\n");
}

static void ConsoleUI_Refresh(int msec) {
    (void)msec;
    
    /* Update resource displays from playerstate */
    LPCPLAYER ps = uiimport.GetPlayerState();
    if (!ps) {
        return;
    }
    
    /* Map playerstate.stats[] to resource text frames */
    /* PLAYERSTATE_RESOURCE_GOLD = 1, LUMBER = 2, FOOD_CAP = 4, FOOD_USED = 5 */
    /* The actual text frame binding will happen in Phase 7.4 expansion */
    
    /* For now, just verify we can read playerstate */
    (void)ps;
}

static void ConsoleUI_Draw(void) {
    if (!root_frame) {
        return;
    }
    
    /* Render ResourceBar */
    if (resource_bar) {
        LPCPLAYER ps = uiimport.GetPlayerState();
        if (ps) {
            /* Format resource strings */
            char gold_text[32];
            char lumber_text[32];
            char food_text[32];
            
            snprintf(gold_text, sizeof(gold_text), "%d", ps->stats[PLAYERSTATE_RESOURCE_GOLD]);
            snprintf(lumber_text, sizeof(lumber_text), "%d", ps->stats[PLAYERSTATE_RESOURCE_LUMBER]);
            snprintf(food_text, sizeof(food_text), "%d/%d", ps->stats[PLAYERSTATE_RESOURCE_FOOD_USED], ps->stats[PLAYERSTATE_RESOURCE_FOOD_CAP]);
            
            /* TODO: Find child text frames and update their text
             * This requires extending UI_FindFrame to search within a parent,
             * or adding a UI_FindChildFrame helper. For now, just log values. */
            
            /* Temporary: draw simple text overlay at top-right */
            /* This will be replaced with proper frame hierarchy traversal */
            (void)gold_text;
            (void)lumber_text;
            (void)food_text;
        }
        
        /* Draw ResourceBar frame hierarchy */
        UI_DrawFrame(resource_bar);
    }
    
    /* Render CinematicPanel if cinematic text is active */
    if (cinematic_panel) {
        LPCPLAYER ps = uiimport.GetPlayerState();
        if (ps && ps->texts[PLAYERTEXT_SPEAKER] && *ps->texts[PLAYERTEXT_SPEAKER]) {
            /* Draw speaker name and dialogue text */
            UI_DrawFrame(cinematic_panel);
        }
    }
    
    /* TODO: Draw command card, inventory, portrait, info panel
     * These will be added in Phase 8-9 after unit query protocol is implemented */
}

static void ConsoleUI_KeyEvent(int key, BOOL down) {
    /* Handle hotkeys for command card, inventory, etc. */
    /* For now, pass-through (no special handling) */
    (void)key;
    (void)down;
}

static void ConsoleUI_MouseEvent(int x, int y, int buttons) {
    /* Handle clicks on command buttons, inventory slots, etc. */
    /* Will be implemented in Phase 8-9 */
    (void)x;
    (void)y;
    (void)buttons;
}

/* Receive unit UI data from server (Phase 8) */
static void ConsoleUI_UpdateUnitUI(DWORD num_units, uiUnitData_t *units) {
    if (num_units > MAX_CACHED_UNITS) {
        num_units = MAX_CACHED_UNITS;
    }
    
    /* Cache unit data for rendering */
    memcpy(cached_units, units, sizeof(uiUnitData_t) * num_units);
    cached_unit_count = num_units;
    
    uiimport.Printf("ConsoleUI: Received UI data for %d units\n", num_units);
    for (DWORD i = 0; i < num_units; i++) {
        uiimport.Printf("  Unit %d: %d buttons, %d inventory, %d queue\n",
                       units[i].entity_num,
                       units[i].num_buttons,
                       units[i].num_inventory,
                       units[i].num_queue);
    }
}

uiScreen_t console_ui_screen = {
    .name = "console",
    .init = ConsoleUI_Init,
    .refresh = ConsoleUI_Refresh,
    .draw = ConsoleUI_Draw,
    .key_event = ConsoleUI_KeyEvent,
    .mouse_event = ConsoleUI_MouseEvent,
    .update_unit_ui = ConsoleUI_UpdateUnitUI,
};
