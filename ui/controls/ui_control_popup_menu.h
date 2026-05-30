#ifndef UI_CONTROL_POPUP_MENU_H
#define UI_CONTROL_POPUP_MENU_H

static BOOL UI_IsPopupFrameType(FRAMETYPE type) {
    return type == FT_POPUPMENU || type == FT_GLUEPOPUPMENU;
}

static LPFRAMEDEF UI_PopupMenuFrame(LPCFRAMEDEF popup) {
    LPFRAMEDEF menu;

    if (!popup || !popup->Popup.MenuFrame[0]) {
        return NULL;
    }
    menu = UI_FindChildFrame((LPFRAMEDEF)popup, popup->Popup.MenuFrame);
    if (!menu) {
        menu = UI_FindFrameNear(popup, popup->Popup.MenuFrame);
    }
    return menu;
}

static BOOL UI_IsActivePopupMenu(LPCFRAMEDEF frame) {
    return frame && active_popup && frame == UI_PopupMenuFrame(active_popup);
}

static BOOL UI_PointerBlockedByPopup(LPCFRAMEDEF frame) {
    LPFRAMEDEF menu;
    LPCRECT rect;

    if (UI_PointerBlockedByModal(frame)) {
        return true;
    }
    if (!active_popup) {
        return false;
    }
    menu = UI_PopupMenuFrame(active_popup);
    if (!menu || frame == active_popup || frame == menu) {
        return false;
    }
    rect = UI_LayoutRect(menu);
    return rect && UI_MouseContains(rect);
}

static LPFRAMEDEF UI_PopupTitleTextFrame(LPCFRAMEDEF popup) {
    LPFRAMEDEF title;
    LPFRAMEDEF text;

    if (!popup) {
        return NULL;
    }
    title = UI_FindChildFrame((LPFRAMEDEF)popup, popup->Popup.TitleFrame);
    text = title ? UI_FindChildFrame(title, "StandardPopupMenuTitleTextTemplate") : NULL;
    if (!text) {
        text = title ? UI_FindChildFrame(title, "CampaignPopupMenuTitleTextTemplate") : NULL;
    }
    return text ? text : title;
}

static void UI_PositionPopupParts(LPFRAMEDEF popup) {
    LPFRAMEDEF title;
    LPFRAMEDEF arrow;
    LPFRAMEDEF menu;
    FLOAT inset;
    FLOAT arrow_width;
    FLOAT title_width;

    if (!popup) {
        return;
    }

    inset = popup->Popup.ButtonInset;
    title = UI_FindChildFrame(popup, popup->Popup.TitleFrame);
    arrow = UI_FindChildFrame(popup, popup->Popup.ArrowFrame);
    menu = UI_PopupMenuFrame(popup);
    arrow_width = arrow && arrow->Width > 0.0f ? arrow->Width : 0.011f;
    title_width = popup->Width - arrow_width - inset * 2.0f;

    if (title && !title->AnyPointsSet) {
        UI_SetSize(title, MAX(0.0f, title_width), popup->Height);
        UI_SetPoint(title, FRAMEPOINT_LEFT, popup, FRAMEPOINT_LEFT, inset, 0.0f);
    }
    if (arrow && !arrow->AnyPointsSet) {
        UI_SetPoint(arrow, FRAMEPOINT_RIGHT, popup, FRAMEPOINT_RIGHT, -inset, 0.0f);
    }
    if (menu) {
        FLOAT row_height = menu->Menu.Item.Height > 0.0f ? menu->Menu.Item.Height : 0.014f;
        FLOAT border = menu->Menu.Border > 0.0f ? menu->Menu.Border : 0.006f;
        if (menu->Menu.ItemCount > 0) {
            UI_SetSize(menu, popup->Width, border * 2.0f + row_height * (FLOAT)menu->Menu.ItemCount);
        }
        if (!menu->AnyPointsSet) {
            UI_SetPoint(menu, FRAMEPOINT_TOPLEFT, popup, FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f);
            UI_SetPoint(menu, FRAMEPOINT_TOPRIGHT, popup, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);
        }
    }
}

static void UI_UpdatePopupVisibility(LPCFRAMEDEF const *draw_order, DWORD count) {
    FOR_LOOP(i, count) {
        LPFRAMEDEF frame = (LPFRAMEDEF)draw_order[i];

        if (!UI_IsPopupFrameType(frame->Type)) {
            continue;
        }

        LPFRAMEDEF menu = UI_PopupMenuFrame(frame);
        UI_PositionPopupParts(frame);
        if (menu) {
            UI_SetHidden(menu, active_popup != frame);
        }
    }
}

static void UI_ClosePopupIfClickedOutside(void) {
    LPFRAMEDEF menu;
    LPCRECT popup_rect;
    LPCRECT menu_rect = NULL;

    if (!active_popup || ui_mouse.event != UI_MOUSE_LEFT_DOWN) {
        return;
    }

    popup_rect = UI_LayoutRect(active_popup);
    menu = UI_PopupMenuFrame(active_popup);
    if (menu && !menu->hidden) {
        menu_rect = UI_LayoutRect(menu);
    }
    if ((popup_rect && UI_MouseContains(popup_rect)) ||
        (menu_rect && UI_MouseContains(menu_rect))) {
        return;
    }
    active_popup = NULL;
}

static void UI_DrawMenu(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();
    LPCFRAMEDEF backdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Normal);
    LPCFONT font;
    FLOAT const border = frame->Menu.Border > 0.0f ? frame->Menu.Border : 0.006f;
    FLOAT const row_height = frame->Menu.Item.Height > 0.0f ? frame->Menu.Item.Height : 0.014f;
    COLOR32 const highlight_color = frame->Menu.TextHighlightColor.a
        ? frame->Menu.TextHighlightColor
        : Theme_ListBoxSelectedTextColor();
    COLOR32 const text_color = frame->Font.Color.a ? frame->Font.Color : COLOR32_WHITE;

    UI_DrawBackdropWithColor(backdrop, rect, frame->Color);
    if (!renderer || !renderer->LoadFont || !renderer->DrawText) {
        return;
    }
    font = renderer->LoadFont(UI_FontFile(frame->Font.Name), UI_FontPixelSize(frame->Font.Size));
    if (!font) {
        return;
    }

    FOR_LOOP(i, frame->Menu.ItemCount) {
        RECT row = MAKE(RECT,
                        rect->x + border,
                        rect->y + border + row_height * (FLOAT)i,
                        MAX(0.0f, rect->w - border * 2.0f),
                        row_height);
        BOOL const hover = UI_MouseContains(&row);

        renderer->DrawText(&MAKE(drawText_t,
                                 .font = font,
                                 .text = frame->Menu.Items[i].text,
                                 .rect = row,
                                 .color = hover ? highlight_color : text_color,
                                 .textWidth = row.w,
                                 .lineHeight = 1.0f,
                                 .wordWrap = FALSE,
                                 .halign = FONT_JUSTIFYLEFT,
                                 .valign = FONT_JUSTIFYMIDDLE));
        if (hover && ui_mouse.event == UI_MOUSE_LEFT_UP) {
            LPFRAMEDEF popup = (LPFRAMEDEF)frame->Parent;
            LPFRAMEDEF title = UI_IsPopupFrameType(popup ? popup->Type : FT_NONE)
                ? UI_PopupTitleTextFrame(popup)
                : NULL;
            char command[160];

            if (title) {
                UI_SetText(title, "%s", frame->Menu.Items[i].text);
            }
            if (frame->OnClick[0]) {
                snprintf(command,
                         sizeof(command),
                         frame->OnClick,
                         (unsigned)frame->Menu.Items[i].value,
                         (unsigned)i);
                UI_MenuCommandLocal(command);
            }
            active_popup = NULL;
        }
    }
}

#endif
