#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "ui_local.h"

typedef enum {
    UI_DIALOG_WAR3_ICON_MESSAGE,
    UI_DIALOG_WAR3_ICON_ERROR,
    UI_DIALOG_WAR3_ICON_QUESTION,
} uiDialogWar3Icon_t;

typedef enum {
    UI_DIALOG_WAR3_BUTTONS_OK,
    UI_DIALOG_WAR3_BUTTONS_YES_NO,
} uiDialogWar3Buttons_t;

typedef struct {
    LPCSTR modal_name;
    LPCSTR cover_name;
    LPCSTR template_name;
} uiDialogWar3Init_t;

typedef struct {
    LPCSTR message;
    uiDialogWar3Icon_t icon;
    uiDialogWar3Buttons_t buttons;
    LPCSTR ok_route;
    LPCSTR yes_route;
    LPCSTR no_route;
} uiDialogWar3Config_t;

typedef struct {
    LPFRAMEDEF parent;
    LPFRAMEDEF modal;
    LPFRAMEDEF cover;
    LPFRAMEDEF dialog;
    LPFRAMEDEF text;
    LPFRAMEDEF icon;
    LPFRAMEDEF ok_backdrop;
    LPFRAMEDEF no_backdrop;
    LPFRAMEDEF yes_backdrop;
    LPFRAMEDEF ok_button;
    LPFRAMEDEF no_button;
    LPFRAMEDEF yes_button;
    FLOAT default_height;
} uiDialogWar3_t;

BOOL UI_DialogWar3Init(uiDialogWar3_t *dialog,
                       LPFRAMEDEF parent,
                       uiDialogWar3Init_t const *init);
void UI_DialogWar3Show(uiDialogWar3_t *dialog,
                       uiDialogWar3Config_t const *config);
void UI_DialogWar3Hide(uiDialogWar3_t *dialog);
BOOL UI_DialogWar3Visible(uiDialogWar3_t const *dialog);

#endif /* UI_DIALOG_H */
