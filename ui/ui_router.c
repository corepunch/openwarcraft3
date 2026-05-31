/*
 * ui_router.c — UI navigation router.
 *
 * The router dispatches resolved menu routes (e.g. "/main", "/lan/setup")
 * to the appropriate screen controller.
 */

#include "ui_local.h"
#include "ui_screen.h"

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

static uiScreen_t *current_screen = NULL;

void UI_ResetRouter(void) {
    current_screen = NULL;
}

static uiScreen_t *UI_FindScreen(LPCSTR name) {
    for (int i = 0; screens[i]; i++) {
        if (!strcmp(screens[i]->name, name)) {
            return screens[i];
        }
    }
    return NULL;
}

static BOOL UI_LoadScreen(uiScreen_t *screen) {
    return !screen || !screen->load || screen->load();
}

static void UI_EnterScreen(uiScreen_t *screen) {
    if (!screen) {
        current_screen = NULL;
        return;
    }
    if (!UI_LoadScreen(screen)) {
        uiimport.Printf("UI_Route: failed to load screen '%s'\n", screen->name);
        current_screen = NULL;
        return;
    }
    current_screen = screen;
    if (screen->init) {
        screen->init();
    }
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
        UI_EnterScreen(screen);
        if (current_screen != screen) {
            return;
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

uiScreen_t *UI_GetCurrentScreen(void) {
    return current_screen;
}
