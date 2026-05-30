#include "ui_dialog.h"

static LPCSTR UI_DialogWar3IconPath(uiDialogWar3Icon_t icon) {
    switch (icon) {
        case UI_DIALOG_WAR3_ICON_ERROR:
            return "UI\\Widgets\\Glues\\dialogbox-error.blp";
        case UI_DIALOG_WAR3_ICON_QUESTION:
            return "UI\\Widgets\\Glues\\dialogbox-question.blp";
        case UI_DIALOG_WAR3_ICON_MESSAGE:
        default:
            return "UI\\Widgets\\Glues\\dialogbox-message.blp";
    }
}

static LPCSTR UI_DialogWar3TemplateName(uiDialogWar3Init_t const *init) {
    return init && init->template_name && init->template_name[0]
           ? init->template_name
           : "DialogWar3";
}

static LPCSTR UI_DialogWar3ModalName(uiDialogWar3Init_t const *init) {
    return init && init->modal_name && init->modal_name[0]
           ? init->modal_name
           : "DialogWar3Modal";
}

static LPCSTR UI_DialogWar3CoverName(uiDialogWar3Init_t const *init) {
    return init && init->cover_name && init->cover_name[0]
           ? init->cover_name
           : "DialogWar3ModalCover";
}

static void UI_DialogWar3SetVisible(uiDialogWar3_t *dialog, BOOL visible) {
    if (!dialog) {
        return;
    }
    UI_SetHidden(dialog->modal, !visible);
    UI_SetHidden(dialog->frames.DialogWar3, !visible);
}

static BOOL UI_DialogWar3CreateModal(uiDialogWar3_t *dialog,
                                     LPFRAMEDEF parent,
                                     uiDialogWar3Init_t const *init)
{
    LPCSTR modal_name = UI_DialogWar3ModalName(init);
    LPCSTR cover_name = UI_DialogWar3CoverName(init);

    dialog->modal = UI_FindFrame(modal_name);
    if (!dialog->modal) {
        dialog->modal = UI_Spawn(FT_DIALOG, parent);
        if (!dialog->modal) {
            uiimport.Printf("ERROR: failed to create %s\n", modal_name);
            return false;
        }
        snprintf(dialog->modal->Name, sizeof(dialog->modal->Name), "%s", modal_name);
    } else {
        UI_SetParent(dialog->modal, parent);
    }

    UI_SetPoint(dialog->modal, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    UI_SetPoint(dialog->modal, FRAMEPOINT_BOTTOMRIGHT, parent, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);

    dialog->cover = UI_FindChildFrame(dialog->modal, cover_name);
    if (!dialog->cover) {
        dialog->cover = UI_Spawn(FT_TEXTURE, dialog->modal);
        if (dialog->cover) {
            snprintf(dialog->cover->Name, sizeof(dialog->cover->Name), "%s", cover_name);
        }
    }
    if (dialog->cover) {
        UI_SetPoint(dialog->cover, FRAMEPOINT_TOPLEFT, dialog->modal, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
        UI_SetPoint(dialog->cover, FRAMEPOINT_BOTTOMRIGHT, dialog->modal, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
        dialog->cover->Texture.Image = UI_LoadTexture("Textures\\Black32.blp", false);
        dialog->cover->Color = MAKE(COLOR32, 255, 255, 255, 128);
    }

    return true;
}

static BOOL UI_DialogWar3CreateFrame(uiDialogWar3_t *dialog,
                                     uiDialogWar3Init_t const *init)
{
    LPCSTR template_name = UI_DialogWar3TemplateName(init);
    DialogWar3FdfBindings_t template_bindings;
    LPFRAMEDEF template_dialog;
    LPFRAMEDEF dialog_frame;

    dialog_frame = UI_FindChildFrame(dialog->modal, template_name);
    if (!dialog_frame) {
        if (!DialogWar3FdfBindings_Bind(&template_bindings)) {
            uiimport.Printf("ERROR: %s not found\n", template_name);
            return false;
        }
        template_dialog = template_bindings.DialogWar3;
        dialog_frame = UI_CloneFrameTree(template_dialog, dialog->modal);
        if (!dialog_frame) {
            uiimport.Printf("ERROR: failed to clone %s\n", template_name);
            return false;
        }
    }

    if (!DialogWar3FdfBindings_BindFromDialogWar3(&dialog->frames, dialog_frame)) {
        return false;
    }

    UI_SetParent(dialog->frames.DialogWar3, dialog->modal);
    UI_SetPoint(dialog->frames.DialogWar3, FRAMEPOINT_CENTER, dialog->modal, FRAMEPOINT_CENTER, 0.0f, 0.0f);

    dialog->default_height = dialog->frames.DialogWar3->Height;

    return true;
}

BOOL UI_DialogWar3Init(uiDialogWar3_t *dialog,
                       LPFRAMEDEF parent,
                       uiDialogWar3Init_t const *init)
{
    if (!dialog || !parent) {
        return false;
    }

    memset(dialog, 0, sizeof(*dialog));
    dialog->parent = parent;

    if (!UI_DialogWar3CreateModal(dialog, parent, init)) {
        return false;
    }
    if (!UI_DialogWar3CreateFrame(dialog, init)) {
        UI_DialogWar3SetVisible(dialog, false);
        return false;
    }

    UI_DialogWar3SetVisible(dialog, false);
    return true;
}

void UI_DialogWar3Show(uiDialogWar3_t *dialog,
                       uiDialogWar3Config_t const *config)
{
    if (!dialog || !dialog->frames.DialogWar3 || !config) {
        return;
    }

    if (dialog->frames.DialogText) {
        UI_SetText(dialog->frames.DialogText, "%s", config->message ? config->message : "");
    }
    if (dialog->frames.DialogIcon) {
        dialog->frames.DialogIcon->Backdrop.Background = UI_LoadTexture(UI_DialogWar3IconPath(config->icon), false);
    }

    UI_SetHidden(dialog->frames.DialogButtonOKBackdrop, config->buttons != UI_DIALOG_WAR3_BUTTONS_OK);
    UI_SetHidden(dialog->frames.DialogButtonNoBackdrop, config->buttons != UI_DIALOG_WAR3_BUTTONS_YES_NO);
    UI_SetHidden(dialog->frames.DialogButtonYesBackdrop, config->buttons != UI_DIALOG_WAR3_BUTTONS_YES_NO);

    UI_SetOnClick(dialog->frames.DialogButtonOK, "%s", config->ok_route ? config->ok_route : "");
    UI_SetOnClick(dialog->frames.DialogButtonNo, "%s", config->no_route ? config->no_route : "");
    UI_SetOnClick(dialog->frames.DialogButtonYes, "%s", config->yes_route ? config->yes_route : "");

    UI_DialogWar3SetVisible(dialog, true);
}

void UI_DialogWar3Hide(uiDialogWar3_t *dialog) {
    UI_DialogWar3SetVisible(dialog, false);
}

BOOL UI_DialogWar3Visible(uiDialogWar3_t const *dialog) {
    return dialog && dialog->modal && dialog->frames.DialogWar3 &&
           !dialog->modal->hidden && !dialog->frames.DialogWar3->hidden;
}
