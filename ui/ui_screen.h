/*
 * ui_screen.h — Screen controller interface.
 *
 * Each UI screen (main menu, single player menu, etc.) is a self-contained
 * module with lifecycle callbacks and input handlers.
 */

#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include "ui_local.h"

typedef struct uiScreen_s {
    LPCSTR name;
    void (*init)(void);
    void (*shutdown)(void);
    void (*refresh)(int msec);
    void (*draw)(void);
    void (*key_event)(int key, BOOL down);
    void (*mouse_event)(int x, int y, int buttons);
    void (*route)(LPCSTR path);  /* Handle sub-routes like /lan?join=192.168.1.1 */
    void (*update_unit_ui)(DWORD num_units, uiUnitData_t *units);  /* Phase 8: receive unit UI data */
} uiScreen_t;

/* Screen implementations */
extern uiScreen_t mainMenuScreen;
extern uiScreen_t singlePlayerMenuScreen;
extern uiScreen_t mapSelectScreen;
extern uiScreen_t lanJoinScreen;
extern uiScreen_t lanCreateScreen;
extern uiScreen_t gameSetupScreen;
extern uiScreen_t quitConfirmScreen;

LPCSTR LAN_SelectedMapPath(void);
LPCSTR LAN_SelectedMapName(void);

#endif /* UI_SCREEN_H */
