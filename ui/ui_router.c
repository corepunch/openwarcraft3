/*
 * ui_router.c — UI navigation router.
 *
 * The router dispatches menu commands (e.g. "menu /main", "menu /lan/setup")
 * to the appropriate screen controller and manages the screen stack.
 */

#include "ui_local.h"
#include "ui_screen.h"

#define MAX_SCREEN_STACK 16

/* Screen registry */
static uiScreen_t *screens[] = {
    &mainMenuScreen,
    &singlePlayerMenuScreen,
    &mapSelectScreen,
    &lanJoinScreen,
    &lanCreateScreen,
    &gameSetupScreen,
    NULL
};

/* Screen stack */
static uiScreen_t *screen_stack[MAX_SCREEN_STACK];
static int screen_stack_depth = 0;
static uiScreen_t *current_screen = NULL;

static uiScreen_t *UI_FindScreen(LPCSTR name) {
    for (int i = 0; screens[i]; i++) {
        if (!strcmp(screens[i]->name, name)) {
            return screens[i];
        }
    }
    return NULL;
}

void UI_Route(LPCSTR path) {
    uiimport.Printf("UI_Route: %s\n", path);

    if (!path || !*path) {
        return;
    }

    /* Handle special routes */
    if (!strcmp(path, "/quit")) {
        uiimport.Cmd_ExecuteText("quit\n");
        return;
    }
    if (!strcmp(path, "/back")) {
        UI_Pop();
        return;
    }

    /* Parse route path */
    char route[256];
    strncpy(route, path, sizeof(route) - 1);
    route[sizeof(route) - 1] = '\0';

    /* Split path into screen name and sub-route */
    char *query = strchr(route, '?');
    if (query) {
        *query++ = '\0';
    }

    /* Strip leading slash */
    char *screen_name = route;
    if (screen_name[0] == '/') {
        screen_name++;
    }

    /* Handle multi-part paths like /lan/refresh */
    char *slash = strchr(screen_name, '/');
    char sub_route[256] = "";
    if (slash) {
        strncpy(sub_route, slash, sizeof(sub_route) - 1);
        sub_route[sizeof(sub_route) - 1] = '\0';
        *slash = '\0';
    }

    /* Find screen */
    uiScreen_t *screen = UI_FindScreen(screen_name);
    if (!screen) {
        uiimport.Printf("UI_Route: screen '%s' not found\n", screen_name);
        return;
    }

    /* Switch to screen if different */
    if (current_screen != screen) {
        if (current_screen && current_screen->shutdown) {
            current_screen->shutdown();
        }
        current_screen = screen;
        if (screen->init) {
            screen->init();
        }
    }

    /* Route to sub-path if present */
    if (sub_route[0] && screen->route) {
        screen->route(sub_route);
    } else if (query && screen->route) {
        char query_path[256];
        snprintf(query_path, sizeof(query_path), "?%s", query);
        screen->route(query_path);
    }
}

void UI_Push(LPCSTR path) {
    if (screen_stack_depth >= MAX_SCREEN_STACK) {
        uiimport.Printf("UI_Push: stack overflow\n");
        return;
    }

    if (current_screen) {
        screen_stack[screen_stack_depth++] = current_screen;
    }
    UI_Route(path);
}

void UI_Pop(void) {
    uiScreen_t *screen;

    if (screen_stack_depth == 0) {
        uiimport.Printf("UI_Pop: stack underflow\n");
        return;
    }

    screen = screen_stack[--screen_stack_depth];
    screen_stack[screen_stack_depth] = NULL;

    if (current_screen && current_screen->shutdown) {
        current_screen->shutdown();
    }
    current_screen = screen;
    if (current_screen && current_screen->init) {
        current_screen->init();
    }
}

uiScreen_t *UI_GetCurrentScreen(void) {
    return current_screen;
}
