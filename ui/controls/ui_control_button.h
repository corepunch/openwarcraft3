#ifndef UI_CONTROL_BUTTON_H
#define UI_CONTROL_BUTTON_H

static BOOL UI_ButtonEnabled(LPCFRAMEDEF frame) {
    return frame &&
           (frame->OnClick[0] ||
            frame->Type == FT_POPUPMENU ||
            frame->Type == FT_GLUEPOPUPMENU);
}

static BOOL UI_ButtonIsPushed(LPCFRAMEDEF frame, LPCRECT rect) {
    return UI_ButtonEnabled(frame) && UI_MouseContains(rect) && ui_mouse.button == 1 && ui_mouse.down;
}

static void UI_DrawButtonText(LPCFRAMEDEF frame, LPCRECT rect) {
    LPFRAMEDEF text_frame = NULL;
    RECT text_rect = *rect;
    COLOR32 original_color;
    BOOL use_disabled_color;

    if (frame->Text && *frame->Text) {
        text_frame = UI_FindChildFrame((LPFRAMEDEF)frame, frame->Text);
    }
    if (!text_frame && frame->Button.NormalText.frame[0]) {
        text_frame = UI_FindChildFrame((LPFRAMEDEF)frame, frame->Button.NormalText.frame);
    }
    if (!text_frame && frame->Button.NormalText.frame[0]) {
        text_frame = UI_FindFrameNear(frame, frame->Button.NormalText.frame);
        if (text_frame && frame->Button.NormalText.text[0]) {
            UI_SetText((LPFRAMEDEF)text_frame, "%s", frame->Button.NormalText.text);
        }
    }
    if (!text_frame) {
        return;
    }

    if (UI_ButtonIsPushed(frame, rect)) {
        text_rect.x += frame->Button.PushedTextOffset.x;
        text_rect.y -= frame->Button.PushedTextOffset.y;
    }

    original_color = text_frame->Font.Color;
    use_disabled_color = !UI_ButtonEnabled(frame) &&
                         (text_frame->Font.DisabledColor.a ||
                          text_frame->Font.DisabledColor.r ||
                          text_frame->Font.DisabledColor.g ||
                          text_frame->Font.DisabledColor.b);
    if (use_disabled_color) {
        text_frame->Font.Color = text_frame->Font.DisabledColor;
    }
    UI_DrawText(text_frame, &text_rect);
    text_frame->Font.Color = original_color;
}

static LPCFRAMEDEF UI_ButtonBackdrop(LPCFRAMEDEF frame, LPCRECT rect) {
    LPCSTR backdrop_name = frame->Control.Backdrop.Normal;
    BOOL const pushed = UI_ButtonIsPushed(frame, rect);

    if (!UI_ButtonEnabled(frame)) {
        backdrop_name = pushed && frame->Control.Backdrop.DisabledPushed[0]
                        ? frame->Control.Backdrop.DisabledPushed
                        : frame->Control.Backdrop.Disabled;
    } else if (pushed && frame->Control.Backdrop.Pushed[0]) {
        backdrop_name = frame->Control.Backdrop.Pushed;
    }

    LPCFRAMEDEF backdrop = UI_FindFrameNear(frame, backdrop_name);
    if (!backdrop && frame->Button.NormalTexture[0]) {
        backdrop = UI_FindFrameNear(frame, frame->Button.NormalTexture);
    }
    return backdrop;
}

#endif
