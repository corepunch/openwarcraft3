/*
 * ui_render.c — Frame rendering and layout solving
 *
 * This module implements:
 * 1. Layout solving: Calculate screen positions from SetPoint anchors
 * 2. Frame rendering: Draw backdrops, textures, text, portraits
 * 3. Frame hierarchy traversal: Recursively render child frames
 *
 * Ported from client/cl_scrn.c (Phase 7 consolidation)
 */

#include "ui_local.h"

#define MAX_FRAME_DEPTH 64
#define NUM_BACKDROP_CORNERS 8

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* Runtime layout state (cached screen rects) */
typedef struct {
    RECT rect;
    BOOL calculated;
} frameRuntime_t;

static frameRuntime_t runtimes[MAX_UI_CLASSES];
static RECT scene_rect;
static BOOL scene_rect_valid = FALSE;
static LPCFRAMEDEF active_slider = NULL;

static LPRENDERER UI_GetRenderer(void) {
    if (!uiimport.GetRenderer) {
        return NULL;
    }
    return uiimport.GetRenderer();
}

static BOOL UI_FrameIndex(LPCFRAMEDEF frame, DWORD *index) {
    if (!frame || frame < frames || frame >= frames + MAX_UI_CLASSES) {
        return FALSE;
    }
    *index = (DWORD)(frame - frames);
    return TRUE;
}

static LPCSTR UI_FontFile(LPCSTR name) {
    return Theme_String(name && *name ? name : "MasterFont", "Default");
}

static DWORD UI_FontPixelSize(FLOAT size) {
    return size > 0 ? (DWORD)(size * 1000.0f + 0.5f) : 13;
}

typedef enum {
    BACKDROPINSET_RIGHT,
    BACKDROPINSET_TOP,
    BACKDROPINSET_BOTTOM,
    BACKDROPINSET_LEFT,
} BACKDROPINSET;

/* Forward declarations */
static LPCRECT UI_LayoutRect(LPCFRAMEDEF frame);
static void UI_DrawFrameOne(LPCFRAMEDEF frame);

/* ========================================================================
 * LAYOUT SOLVING
 * ======================================================================== */

static RECT UI_GetSceneRect(void) {
    if (scene_rect_valid) {
        return scene_rect;
    }
    
    LPRENDERER renderer = UI_GetRenderer();
    size2_t window;
    FLOAT window_aspect = UI_MIN_ASPECT;
    FLOAT x_scale = 1.0f;
    FLOAT y_scale = 1.0f;

    if (!renderer || !renderer->GetWindowSize) {
        return (RECT) { 0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT };
    }

    window = renderer->GetWindowSize();

    if (window.width > 0 && window.height > 0) {
        window_aspect = (FLOAT)window.width / (FLOAT)window.height;
    }

    if (window_aspect > UI_MIN_ASPECT) {
        x_scale = window_aspect / UI_MIN_ASPECT;
    } else if (window_aspect < UI_MIN_ASPECT) {
        y_scale = UI_MIN_ASPECT / window_aspect;
    }

    FLOAT scene_w = UI_BASE_WIDTH * x_scale;
    FLOAT scene_h = UI_BASE_HEIGHT * y_scale;
    
    scene_rect = (RECT) {
        .x = (UI_BASE_WIDTH - scene_w) * 0.5f,
        .y = (UI_BASE_HEIGHT - scene_h) * 0.5f,
        .w = scene_w,
        .h = scene_h
    };
    
    scene_rect_valid = TRUE;
    return scene_rect;
}

static VECTOR2 UI_GetXBounds(LPCRECT rect) {
    return (VECTOR2) { rect->x, rect->x + rect->w };
}

static VECTOR2 UI_GetYBounds(LPCRECT rect) {
    return (VECTOR2) { rect->y, rect->y + rect->h };
}

static VECTOR2 UI_GetAxisBounds(LPCRECT rect, BOOL is_x_axis) {
    return is_x_axis ? UI_GetXBounds(rect) : UI_GetYBounds(rect);
}

static FLOAT UI_NormalizeAnchorOffset(LPCFRAMEPOINT p, BOOL is_x_axis) {
    return is_x_axis ? p->offset : -p->offset;
}

static LPCRECT UI_GetRelativeRect(LPCFRAMEDEF frame, LPCFRAMEDEF relativeTo) {
    if (!relativeTo) {
        /* Anchor to root scene */
        return &scene_rect;
    }
    if (relativeTo == frame->Parent) {
        return UI_LayoutRect(frame->Parent);
    }
    return UI_LayoutRect(relativeTo);
}

static FLOAT UI_GetAnchor(LPCFRAMEDEF frame,
                         LPCFRAMEPOINT p,
                         BOOL is_x_axis)
{
    VECTOR2 b = UI_GetAxisBounds(UI_GetRelativeRect(frame, p->relativeTo), is_x_axis);
    FLOAT offset = UI_NormalizeAnchorOffset(p, is_x_axis);
    
    if (p->targetPos == FPP_MID) {
        return (b.x + b.y) / 2.0f + offset;
    } else if (p->targetPos == FPP_MAX) {
        return b.y + offset;
    } else {
        return b.x + offset;
    }
}

static VECTOR2 UI_SolveAxisPosition(LPCFRAMEDEF frame,
                                   FRAMEPOINT const *points,
                                   FLOAT size,
                                   BOOL is_x_axis)
{
    LPCFRAMEPOINT pmin = &points[FPP_MIN];
    LPCFRAMEPOINT pmid = &points[FPP_MID];
    LPCFRAMEPOINT pmax = &points[FPP_MAX];

    if (pmid->used) {
        /* Center anchor: position = mid - size/2 */
        return (VECTOR2) {
            UI_GetAnchor(frame, pmid, is_x_axis) - size / 2.0f,
            size,
        };
    } else if (pmin->used && pmax->used) {
        /* Both min and max: stretch between anchors */
        FLOAT anchor_min = UI_GetAnchor(frame, pmin, is_x_axis);
        FLOAT anchor_max = UI_GetAnchor(frame, pmax, is_x_axis);
        return (VECTOR2) {
            anchor_min,
            anchor_max - anchor_min,
        };
    } else if (pmax->used) {
        /* Max anchor only: position = max - size */
        return (VECTOR2) {
            UI_GetAnchor(frame, pmax, is_x_axis) - size,
            size,
        };
    } else if (pmin->used) {
        /* Min anchor only: position = min */
        return (VECTOR2) {
            UI_GetAnchor(frame, pmin, is_x_axis),
            size,
        };
    }
    
    /* No anchors set: default to (0,0) with given size */
    return (VECTOR2) { 0, size };
}

static LPCRECT UI_LayoutRect(LPCFRAMEDEF frame) {
    static RECT uncached_rect;
    DWORD frame_index;
    RECT *out;

    if (!frame) {
        return &scene_rect;
    }
    
    /* Check cache */
    if (UI_FrameIndex(frame, &frame_index) && runtimes[frame_index].calculated) {
        return &runtimes[frame_index].rect;
    }
    out = UI_FrameIndex(frame, &frame_index) ? &runtimes[frame_index].rect : &uncached_rect;
    
    /* Mark as calculated to prevent recursion */
    if (UI_FrameIndex(frame, &frame_index)) {
        runtimes[frame_index].calculated = TRUE;
    }
    
    /* Calculate intrinsic size based on frame type */
    FLOAT intrinsic_w = frame->Width;
    FLOAT intrinsic_h = frame->Height;
    
    if (intrinsic_w == 0 || intrinsic_h == 0) {
        /* Try to derive size from content */
        switch (frame->Type) {
            case FT_TEXT:
            case FT_STRING:
                if (frame->Text && frame->Font.Index) {
                    LPRENDERER renderer = UI_GetRenderer();
                    drawText_t dt = {
                        .font = renderer ? renderer->LoadFont(UI_FontFile(frame->Font.Name),
                                                              UI_FontPixelSize(frame->Font.Size)) : NULL,
                        .text = frame->Text,
                        .textWidth = intrinsic_w > 0 ? intrinsic_w : 0.2f,
                        .lineHeight = 1.33f,
                        .wordWrap = TRUE,
                    };
                    VECTOR2 text_size = renderer ? renderer->GetTextSize((LPCDRAWTEXT)&dt) : MAKE(VECTOR2, 0, 0);
                    if (intrinsic_w == 0) intrinsic_w = text_size.x;
                    if (intrinsic_h == 0) intrinsic_h = text_size.y;
                }
                break;
            case FT_TEXTURE:
            case FT_BACKDROP:
                if (frame->Texture.Image) {
                    LPRENDERER renderer = UI_GetRenderer();
                    LPCTEXTURE texture = UI_GetTexture(frame->Texture.Image);
                    size2_t tex_size = (renderer && texture) ? renderer->GetTextureSize(texture) : MAKE(size2_t, 0, 0);
                    if (intrinsic_w == 0) intrinsic_w = tex_size.width / 1000.0f;  /* Normalize to 0-1 space */
                    if (intrinsic_h == 0) intrinsic_h = tex_size.height / 1000.0f;
                }
                break;
            default:
                /* Default size if not specified */
                if (intrinsic_w == 0) intrinsic_w = 0.1f;
                if (intrinsic_h == 0) intrinsic_h = 0.1f;
                break;
        }
    }
    
    /* Solve X and Y positions */
    VECTOR2 x_pos = UI_SolveAxisPosition(frame, frame->Points.x, intrinsic_w, TRUE);
    VECTOR2 y_pos = UI_SolveAxisPosition(frame, frame->Points.y, intrinsic_h, FALSE);
    
    *out = (RECT) {
        .x = x_pos.x,
        .y = y_pos.x,
        .w = x_pos.y,
        .h = y_pos.y,
    };
    
    return out;
}

/* ========================================================================
 * FRAME RENDERING
 * ======================================================================== */

static BOOL UI_BackdropHasArt(LPCFRAMEDEF frame) {
    return frame && (frame->Backdrop.Background || frame->Backdrop.EdgeFile);
}

static void UI_BackdropRects(LPCRECT screen, LPRECT rects, FLOAT corner_size) {
    FLOAT x[] = { 0, corner_size, screen->w - corner_size, screen->w };
    FLOAT y[] = { 0, corner_size, screen->h - corner_size, screen->h };
    FOR_LOOP(i, BACKDROP_SIZE) {
        rects[i].x = screen->x + x[i % 3];
        rects[i].y = screen->y + y[i / 3];
        rects[i].w = x[(i % 3) + 1] - x[i % 3];
        rects[i].h = y[(i / 3) + 1] - y[i / 3];
    }
}

static FLOAT UI_BackdropEdgeTile(LPCRECT rect, BACKDROPCORNER edge, FLOAT image_size) {
    if (image_size <= 0) {
        return 1;
    }
    switch (edge) {
        case BACKDROP_LEFT_EDGE:
        case BACKDROP_RIGHT_EDGE:
            return ceilf(rect->h / image_size);
        case BACKDROP_TOP_EDGE:
        case BACKDROP_BOTTOM_EDGE:
            return ceilf(rect->w / image_size);
        default:
            return 1;
    }
}

static BOOL UI_BackdropEdgeFlip(BACKDROPCORNER edge) {
    switch (edge) {
        case BACKDROP_TOP_EDGE:
        case BACKDROP_BOTTOM_EDGE:
            return true;
        default:
            return false;
    }
}

static void UI_DrawBackdropWithColor(LPCFRAMEDEF frame, LPCRECT rect, COLOR32 color) {
    LPRENDERER renderer = UI_GetRenderer();
    BACKDROPCORNER const corners[NUM_BACKDROP_CORNERS] = {
        BACKDROP_LEFT_EDGE,
        BACKDROP_RIGHT_EDGE,
        BACKDROP_TOP_EDGE,
        BACKDROP_BOTTOM_EDGE,
        BACKDROP_TOP_LEFT_CORNER,
        BACKDROP_TOP_RIGHT_CORNER,
        BACKDROP_BOTTOM_LEFT_CORNER,
        BACKDROP_BOTTOM_RIGHT_CORNER,
    };
    RECT rects[BACKDROP_SIZE];

    if (!UI_BackdropHasArt(frame)) {
        return;
    }

    if (!renderer || !renderer->DrawImageEx) {
        return;
    }

    if (frame->Backdrop.Background) {
        LPCTEXTURE tex = UI_GetTexture(frame->Backdrop.Background);
        if (tex) {
            size2_t back_size = renderer->GetTextureSize ? renderer->GetTextureSize(tex) : MAKE(size2_t, 0, 0);
            RECT uv = { frame->Backdrop.Mirrored ? 1 : 0, 0, frame->Backdrop.Mirrored ? -1 : 1, 1 };
            RECT background = *rect;
            background.x += frame->Backdrop.BackgroundInsets[BACKDROPINSET_LEFT];
            background.y += frame->Backdrop.BackgroundInsets[BACKDROPINSET_TOP];
            background.w -= frame->Backdrop.BackgroundInsets[BACKDROPINSET_LEFT];
            background.w -= frame->Backdrop.BackgroundInsets[BACKDROPINSET_RIGHT];
            background.h -= frame->Backdrop.BackgroundInsets[BACKDROPINSET_TOP];
            background.h -= frame->Backdrop.BackgroundInsets[BACKDROPINSET_BOTTOM];
            if (frame->Backdrop.TileBackground && back_size.width > 0 && back_size.height > 0) {
                uv.w = background.w / (back_size.width / 1000.0f);
                if (frame->Backdrop.Mirrored) {
                    uv.x = uv.w;
                    uv.w = -uv.w;
                }
                uv.h = background.h / (back_size.height / 1000.0f);
            }
            renderer->DrawImageEx(&MAKE(drawImage_t,
                                        .texture = tex,
                                        .shader = SHADER_UI,
                                        .alphamode = BLEND_MODE_BLEND,
                                        .screen = background,
                                        .uv = uv,
                                        .color = color,
                                        .rotate = FALSE));
        }
    }

    if (!frame->Backdrop.EdgeFile || !frame->Backdrop.CornerFlags) {
        return;
    }

    LPCTEXTURE edge_tex = UI_GetTexture(frame->Backdrop.EdgeFile);
    if (!edge_tex) {
        return;
    }

    size2_t edge_size = renderer->GetTextureSize ? renderer->GetTextureSize(edge_tex) : MAKE(size2_t, 0, 0);
    FLOAT const edge_image_height = edge_size.height / 1000.0f;
    UI_BackdropRects(rect, rects, frame->Backdrop.CornerSize);
    FOR_LOOP(i, NUM_BACKDROP_CORNERS) {
        if ((frame->Backdrop.CornerFlags & (1 << corners[i])) == 0) {
            continue;
        }
        FLOAT const k = 1.0f / NUM_BACKDROP_CORNERS;
        FLOAT const tile = UI_BackdropEdgeTile(rects + corners[i], corners[i], edge_image_height);
        BOOL const flip = UI_BackdropEdgeFlip(corners[i]);
        RECT const uv = { i * k, 0, k, tile };
        renderer->DrawImageEx(&MAKE(drawImage_t,
                                    .texture = edge_tex,
                                    .shader = SHADER_UI,
                                    .alphamode = BLEND_MODE_BLEND,
                                    .screen = rects[corners[i]],
                                    .uv = uv,
                                    .color = color,
                                    .rotate = flip));
    }
}

static void UI_DrawBackdrop(LPCFRAMEDEF frame, LPCRECT rect) {
    UI_DrawBackdropWithColor(frame, rect, frame->Color);
}

static void UI_DrawTexture(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();

    if (!frame->Texture.Image) {
        return;
    }

    if (!renderer || !renderer->DrawImageEx) {
        return;
    }

    LPCTEXTURE tex = UI_GetTexture(frame->Texture.Image);
    if (!tex) {
        return;
    }
    
    /* Use TexCoord if specified, otherwise full texture */
    /* BOX2 has min/max, not x/y/w/h */
    RECT uv;
    if (frame->Texture.TexCoord.min.x != 0 || frame->Texture.TexCoord.min.y != 0 ||
        frame->Texture.TexCoord.max.x != 0 || frame->Texture.TexCoord.max.y != 0) {
        uv = (RECT) {
            frame->Texture.TexCoord.min.x,
            frame->Texture.TexCoord.min.y,
            frame->Texture.TexCoord.max.x - frame->Texture.TexCoord.min.x,
            frame->Texture.TexCoord.max.y - frame->Texture.TexCoord.min.y
        };
    } else {
        uv = (RECT) { 0, 0, 1, 1 };
    }
    
    drawImage_t di = {
        .texture = tex,
        .shader = SHADER_UI,
        .alphamode = frame->AlphaMode,
        .screen = *rect,
        .uv = uv,
        .color = frame->Color,
        .rotate = FALSE,
    };
    
    renderer->DrawImageEx((LPCDRAWIMAGE)&di);
}

static BOOL UI_ButtonEnabled(LPCFRAMEDEF frame) {
    return frame && frame->OnClick[0];
}

static BOOL UI_ButtonIsPushed(LPCFRAMEDEF frame, LPCRECT rect) {
    (void)frame;
    return UI_MouseContains(rect) && ui_mouse.button == 1 && ui_mouse.down;
}

static void UI_DrawText(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();
    LPCSTR font_name;
    DWORD font_size;
    RECT text_rect = *rect;

    if (!frame->Text || !*frame->Text) {
        return;
    }

    if (!renderer || !renderer->LoadFont || !renderer->DrawText) {
        return;
    }

    font_name = UI_FontFile(frame->Font.Name);
    font_size = UI_FontPixelSize(frame->Font.Size);
    LPCFONT font = renderer->LoadFont(font_name, font_size);
    if (!font) {
        return;
    }
    
    text_rect.x += frame->Font.Justification.Offset.x;
    text_rect.y += frame->Font.Justification.Offset.y;

    drawText_t dt = {
        .font = font,
        .text = frame->Text,
        .rect = text_rect,
        .color = frame->Font.Color,
        .textWidth = text_rect.w,
        .lineHeight = 1.33f,
        .wordWrap = TRUE,
        .halign = frame->Font.Justification.Horizontal,
        .valign = frame->Font.Justification.Vertical,
    };
    
    renderer->DrawText((LPCDRAWTEXT)&dt);
}

static void UI_DrawMapListControl(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();
    uiMapListControl_t const *control;
    uiMapListState_t *state;
    LPCFONT font;
    DWORD visible_rows;
    FLOAT row_height;
    DWORD first_row;
    FLOAT visual_scroll;
    FLOAT row_offset;
    RECT content;
    RECT clip;
    VECTOR2 mouse;

    if (!frame || !rect) {
        return;
    }

    control = &frame->MapListControl;
    state = control->State;
    if (!state || !renderer || !renderer->LoadFont || !renderer->DrawText) {
        return;
    }

    row_height = control->RowHeight > 0 ? control->RowHeight : 0.019f;
    visible_rows = control->VisibleRows ? control->VisibleRows : (DWORD)((rect->h - control->InsetY * 2.0f) / row_height);
    content = MAKE(RECT,
                   rect->x + control->InsetX,
                   rect->y + control->InsetY,
                   rect->w - control->InsetX * 2.0f,
                   row_height);
    clip = MAKE(RECT,
                content.x,
                content.y,
                content.w,
                row_height * (FLOAT)visible_rows);

    if (UI_MouseContains(rect) && ui_mouse.event == UI_MOUSE_LEFT_UP && visible_rows > 0) {
        FLOAT row;
        DWORD index;
        mouse = UI_MouseToFdf();
        row = (mouse.y - content.y) / row_height;
        index = (DWORD)floorf(state->visualScroll + row);
        if (row >= 0.0f && row < (FLOAT)visible_rows && index < state->count) {
            char command[128];
            snprintf(command,
                     sizeof(command),
                     control->SelectRoute[0] ? control->SelectRoute : "menu /lan/select/%u",
                     (unsigned)index);
            UI_MenuCommandLocal(command);
        }
    }
    if (UI_MouseContains(rect) &&
        (ui_mouse.event == UI_MOUSE_WHEEL_UP || ui_mouse.event == UI_MOUSE_WHEEL_DOWN) &&
        visible_rows > 0 && state->count > visible_rows) {
        DWORD const max_scroll = state->count - visible_rows;

        if (ui_mouse.event == UI_MOUSE_WHEEL_UP) {
            state->scroll = state->scroll > 0 ? state->scroll - 1 : 0;
        } else if (state->scroll < max_scroll) {
            state->scroll++;
        }
    }

    font = renderer->LoadFont(UI_FontFile(control->FontName), UI_FontPixelSize(control->FontSize));
    if (!font) {
        return;
    }

    visual_scroll = state->visualScroll;
    if (visual_scroll < 0.0f) {
        visual_scroll = 0.0f;
    }
    first_row = (DWORD)floorf(visual_scroll);
    row_offset = (visual_scroll - (FLOAT)first_row) * row_height;

    for (DWORD row = 0; row <= visible_rows; row++) {
        DWORD const index = first_row + row;
        uiMapListItem_t const *item;
        char text[256];
        BOOL selected;
        RECT row_rect = content;
        RECT icon_rect;
        RECT text_rect;

        if (index >= state->count) {
            break;
        }

        item = &state->items[index];
        selected = index == state->selected;
        row_rect.y += row_height * (FLOAT)row - row_offset;
        if (row_rect.y + row_rect.h <= clip.y || row_rect.y >= clip.y + clip.h) {
            continue;
        }
        if (selected && renderer->DrawImageEx) {
            RECT selection = row_rect;
            selection.x += 0.0025f;
            selection.y += 0.002f;
            selection.w -= 0.005f;
            selection.h -= 0.004f;
            renderer->DrawImageEx(&MAKE(drawImage_t,
                                        .texture = NULL,
                                        .shader = SHADER_UI,
                                        .alphamode = BLEND_MODE_BLEND,
                                        .screen = selection,
                                        .uv = MAKE(RECT, 0, 0, 1, 1),
                                        .color = Theme_ListBoxSelectionColor(),
                                        .hasClip = TRUE,
                                        .clip = clip));
        }
        snprintf(text,
                 sizeof(text),
                 "%s",
                 item->name[0] ? item->name : item->path);
        icon_rect = row_rect;
        icon_rect.x += 0.004f;
        icon_rect.y += 0.001f;
        icon_rect.w = row_height - 0.002f;
        icon_rect.h = row_height - 0.002f;
        if (renderer->DrawImageEx) {
            DWORD const icon = UI_LoadTexture("ui\\widgets\\glues\\icon-file-melee.blp", false);
            LPCTEXTURE icon_texture = UI_GetTexture(icon);

            if (icon_texture) {
                renderer->DrawImageEx(&MAKE(drawImage_t,
                                            .texture = icon_texture,
                                            .shader = SHADER_UI,
                                            .alphamode = BLEND_MODE_BLEND,
                                            .screen = icon_rect,
                                            .uv = MAKE(RECT, 0, 0, 1, 1),
                                            .color = COLOR32_WHITE,
                                            .hasClip = TRUE,
                                            .clip = clip));
            }
        }
        if (item->players > 0) {
            char players[8];
            LPCFONT small_font = renderer->LoadFont(UI_FontFile(control->FontName), 9);

            snprintf(players, sizeof(players), "%u", (unsigned)item->players);
            if (small_font) {
                renderer->DrawText(&MAKE(drawText_t,
                                         .font = small_font,
                                         .text = players,
                                         .rect = icon_rect,
                                         .color = Theme_ListBoxIconTextColor(),
                                         .textWidth = icon_rect.w,
                                         .lineHeight = 1.0f,
                                         .wordWrap = FALSE,
                                         .halign = FONT_JUSTIFYCENTER,
                                         .valign = FONT_JUSTIFYMIDDLE,
                                         .hasClip = TRUE,
                                         .clip = clip));
            }
        }
        text_rect = row_rect;
        text_rect.x += 0.026f;
        text_rect.w -= 0.028f;
        renderer->DrawText(&MAKE(drawText_t,
                                 .font = font,
                                 .text = text,
                                 .rect = text_rect,
                                 .color = selected ? control->SelectedTextColor : control->TextColor,
                                 .textWidth = text_rect.w,
                                 .lineHeight = 1.0f,
                                 .wordWrap = FALSE,
                                 .halign = FONT_JUSTIFYLEFT,
                                 .valign = FONT_JUSTIFYMIDDLE,
                                 .hasClip = TRUE,
                                 .clip = clip));
    }
}

static void UI_DrawButtonText(LPCFRAMEDEF frame, LPCRECT rect) {
    LPCFRAMEDEF text_frame = NULL;
    RECT text_rect = *rect;

    if (frame->Text && *frame->Text) {
        text_frame = UI_FindChildFrame((LPFRAMEDEF)frame, frame->Text);
    }
    if (!text_frame && frame->Button.NormalText.frame[0]) {
        text_frame = UI_FindChildFrame((LPFRAMEDEF)frame, frame->Button.NormalText.frame);
    }
    if (!text_frame) {
        return;
    }

    if (UI_ButtonIsPushed(frame, rect)) {
        text_rect.x += frame->Button.PushedTextOffset.x;
        text_rect.y -= frame->Button.PushedTextOffset.y;
    }

    UI_DrawText(text_frame, &text_rect);
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

static FLOAT UI_SliderFraction(LPCFRAMEDEF frame) {
    FLOAT min_value = frame->Slider.MinValue;
    FLOAT max_value = frame->Slider.MaxValue;
    FLOAT value = frame->Slider.InitialValue;

    if (max_value <= min_value) {
        return 0.0f;
    }
    value = MAX(min_value, MIN(max_value, value));
    return (value - min_value) / (max_value - min_value);
}

static RECT UI_SliderThumbRect(LPCFRAMEDEF slider, LPCRECT slider_rect, LPCFRAMEDEF thumb) {
    FLOAT const fraction = UI_SliderFraction(slider);
    FLOAT const thumb_w = thumb && thumb->Width > 0 ? thumb->Width : slider_rect->h;
    FLOAT const thumb_h = thumb && thumb->Height > 0 ? thumb->Height : slider_rect->h;
    FLOAT const travel_w = MAX(0.0f, slider_rect->w - thumb_w);
    FLOAT const travel_h = MAX(0.0f, slider_rect->h - thumb_h);
    RECT rect = {
        .x = slider_rect->x + (slider_rect->w - thumb_w) * 0.5f,
        .y = slider_rect->y + (slider_rect->h - thumb_h) * 0.5f,
        .w = thumb_w,
        .h = thumb_h,
    };

    if (slider->Slider.Layout == LAYOUT_VERTICAL) {
        rect.y = slider_rect->y + travel_h * (1.0f - fraction);
    } else {
        rect.x = slider_rect->x + travel_w * fraction;
    }
    return rect;
}

static FLOAT UI_SliderValueFromMouse(LPCFRAMEDEF slider, LPCRECT slider_rect, LPCFRAMEDEF thumb) {
    VECTOR2 const mouse = UI_MouseToFdf();
    FLOAT const min_value = slider->Slider.MinValue;
    FLOAT const max_value = slider->Slider.MaxValue;
    FLOAT value_range = max_value - min_value;
    FLOAT fraction;
    FLOAT value;

    if (value_range <= 0.0f) {
        return min_value;
    }

    if (slider->Slider.Layout == LAYOUT_VERTICAL) {
        FLOAT const thumb_h = thumb && thumb->Height > 0 ? thumb->Height : slider_rect->h;
        FLOAT const travel_h = MAX(0.0f, slider_rect->h - thumb_h);
        FLOAT const local = mouse.y - slider_rect->y - thumb_h * 0.5f;
        fraction = travel_h > 0.0f ? 1.0f - (local / travel_h) : 0.0f;
    } else {
        FLOAT const thumb_w = thumb && thumb->Width > 0 ? thumb->Width : slider_rect->h;
        FLOAT const travel_w = MAX(0.0f, slider_rect->w - thumb_w);
        FLOAT const local = mouse.x - slider_rect->x - thumb_w * 0.5f;
        fraction = travel_w > 0.0f ? local / travel_w : 0.0f;
    }

    fraction = MAX(0.0f, MIN(1.0f, fraction));
    value = min_value + fraction * value_range;
    if (slider->Slider.StepSize > 0.0f) {
        value = min_value + roundf((value - min_value) / slider->Slider.StepSize) * slider->Slider.StepSize;
    }
    return MAX(min_value, MIN(max_value, value));
}

static void UI_UpdateSliderInteraction(LPCFRAMEDEF frame, LPCRECT rect, LPCFRAMEDEF thumb) {
    RECT thumb_rect = UI_SliderThumbRect(frame, rect, thumb);
    BOOL const can_start = ui_mouse.event == UI_MOUSE_LEFT_DOWN &&
                           (UI_MouseContains(rect) || UI_MouseContains(&thumb_rect));

    if (can_start) {
        active_slider = frame;
    }
    if (active_slider == frame && ui_mouse.down) {
        ((LPFRAMEDEF)frame)->Slider.InitialValue = UI_SliderValueFromMouse(frame, rect, thumb);
    }
    if (active_slider == frame && ui_mouse.event == UI_MOUSE_LEFT_UP) {
        ((LPFRAMEDEF)frame)->Slider.InitialValue = UI_SliderValueFromMouse(frame, rect, thumb);
        active_slider = NULL;
    }
    if (!ui_mouse.down && active_slider == frame) {
        active_slider = NULL;
    }
}

static void UI_DrawSlider(LPCFRAMEDEF frame, LPCRECT rect) {
    LPCFRAMEDEF backdrop;
    LPCFRAMEDEF thumb;

    if (frame->Control.Backdrop.Normal[0]) {
        backdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Normal);
        UI_DrawBackdropWithColor(backdrop, rect, frame->Color);
    }

    thumb = UI_FindFrameNear(frame, frame->Slider.ThumbButtonFrame);
    if (thumb) {
        UI_UpdateSliderInteraction(frame, rect, thumb);
        RECT thumb_rect = UI_SliderThumbRect(frame, rect, thumb);
        LPCFRAMEDEF thumb_backdrop = UI_FindFrameNear(thumb, thumb->Control.Backdrop.Normal);
        if (!thumb_backdrop) {
            thumb_backdrop = UI_ButtonBackdrop(thumb, &thumb_rect);
        }
        UI_DrawBackdropWithColor(thumb_backdrop, &thumb_rect, thumb->Color);
        UI_DrawTexture(thumb, &thumb_rect);
    }
}

static void UI_DrawHighlightFrame(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();

    if (!frame || !frame->Highlight.AlphaFile) {
        return;
    }
    if (!renderer || !renderer->DrawImageEx) {
        return;
    }

    LPCTEXTURE tex = UI_GetTexture(frame->Highlight.AlphaFile);
    if (!tex) {
        return;
    }

    renderer->DrawImageEx(&MAKE(drawImage_t,
                                .texture = tex,
                                .shader = SHADER_UI,
                                .alphamode = frame->Highlight.AlphaMode,
                                .screen = *rect,
                                .uv = MAKE(RECT, 0, 0, 1, 1),
                                .color = COLOR32_WHITE,
                                .rotate = FALSE));
}

static void UI_DrawButtonHighlight(LPCFRAMEDEF frame) {
    LPCRECT rect;
    LPCFRAMEDEF highlight;

    if (!frame || !UI_ButtonEnabled(frame)) {
        return;
    }
    rect = UI_LayoutRect(frame);
    if (!rect || !UI_MouseContains(rect)) {
        return;
    }

    highlight = UI_FindFrameNear(frame, frame->Control.Backdrop.MouseOver);
    UI_DrawHighlightFrame(highlight, rect);
}

static BOOL UI_RenderIsButtonFrameType(FRAMETYPE type) {
    return type == FT_BUTTON ||
           type == FT_TEXTBUTTON ||
           type == FT_GLUETEXTBUTTON ||
           type == FT_GLUEBUTTON ||
           type == FT_GLUEPOPUPMENU ||
           type == FT_POPUPMENU ||
           type == FT_SIMPLEBUTTON;
}

static void UI_DrawPortrait(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();

    if (!frame->Portrait.model) {
        return;
    }

    if (!renderer || !renderer->DrawPortrait) {
        return;
    }

    LPCMODEL model = UI_GetModel(frame->Portrait.model);
    if (!model) {
        return;
    }
    
    /* Default animation for portraits is "Stand" or first available */
    LPCSTR anim = "Stand";
    renderer->DrawPortrait(model, rect, anim);
}

static void UI_DrawSprite(LPCFRAMEDEF frame, LPCRECT rect) {
    LPRENDERER renderer = UI_GetRenderer();

    if (frame->Texture.Image) {
        UI_DrawTexture(frame, rect);
        return;
    }

    if (!frame->Portrait.model) {
        return;
    }

    if (!renderer || !renderer->DrawSprite) {
        return;
    }

    LPCMODEL model = UI_GetModel(frame->Portrait.model);
    if (!model) {
        return;
    }
    
    LPCSTR anim = (frame->Text && *frame->Text) ? frame->Text : "Stand";
    renderer->DrawSprite(model, anim, rect->x, rect->y);
}

static void UI_DrawFrameOne(LPCFRAMEDEF frame) {
    if (!frame) {
        return;
    }
    
    /* Skip hidden frames */
    if (frame->hidden) {
        return;
    }
    
    /* Calculate layout */
    LPCRECT rect = UI_LayoutRect(frame);
    if (!rect || rect->w <= 0 || rect->h <= 0) {
        return;
    }
    
    /* Render based on frame type */
    switch (frame->Type) {
        case FT_FRAME:
        case FT_SIMPLEFRAME:
            /* Container frames have no visual representation */
            break;

        case FT_CONTROL:
            if (frame->Control.Backdrop.Normal[0]) {
                LPCFRAMEDEF backdrop = UI_FindFrameNear(frame, frame->Control.Backdrop.Normal);
                UI_DrawBackdropWithColor(backdrop, rect, frame->Color);
            }
            UI_DrawMapListControl(frame, rect);
            break;
            
        case FT_BACKDROP:
            UI_DrawBackdrop(frame, rect);
            break;
            
        case FT_TEXTURE:
            UI_DrawTexture(frame, rect);
            break;
            
        case FT_TEXT:
        case FT_STRING:
            UI_DrawText(frame, rect);
            break;
            
        case FT_BUTTON:
        case FT_TEXTBUTTON:
        case FT_GLUETEXTBUTTON:
        case FT_GLUEBUTTON:
        case FT_GLUEPOPUPMENU:
        case FT_POPUPMENU:
        case FT_SIMPLEBUTTON:
            /* Draw button background */
            {
                LPCFRAMEDEF backdrop = UI_ButtonBackdrop(frame, rect);
                UI_DrawBackdropWithColor(backdrop, rect, frame->Color);
            }
            UI_DrawTexture(frame, rect);
            UI_DrawButtonText(frame, rect);
            if (UI_MouseContains(rect) &&
                ui_mouse.event == UI_MOUSE_LEFT_UP &&
                frame->OnClick[0]) {
                UI_MenuCommandLocal(frame->OnClick);
            }
            break;
            
        case FT_MODEL:
            UI_DrawPortrait(frame, rect);
            break;
            
        case FT_SPRITE:
            UI_DrawSprite(frame, rect);
            break;
            
        case FT_SLIDER:
            UI_DrawSlider(frame, rect);
            break;

        case FT_MENU:
        case FT_LISTBOX:
        case FT_CHECKBOX:
        case FT_EDITBOX:
        case FT_TEXTAREA:
            /* TODO: Implement complex control rendering */
            break;
            
        default:
            break;
    }
    
}

/* ========================================================================
 * PUBLIC API
 * ======================================================================== */

void UI_DrawFrame(LPCFRAMEDEF frame) {
    if (!frame) {
        return;
    }
    
    /* Clear layout cache (scene rect may have changed) */
    scene_rect_valid = FALSE;
    memset(runtimes, 0, sizeof(runtimes));
    
    /* Initialize scene rect */
    scene_rect = UI_GetSceneRect();
    
    LPCFRAMEDEF draw_order[MAX_UI_CLASSES];
    DWORD const total = UI_CollectFrameTree(frame, draw_order, MAX_UI_CLASSES);
    DWORD const count = MIN(total, MAX_UI_CLASSES);

    /* Match the old client overlay pass: animated/model sprites first, then
     * regular UI controls above them. */
    FOR_LOOP(i, count) {
        if (draw_order[i]->Type == FT_SPRITE) {
            UI_DrawFrameOne(draw_order[i]);
        }
    }
    FOR_LOOP(i, count) {
        if (draw_order[i]->Type != FT_SPRITE) {
            UI_DrawFrameOne(draw_order[i]);
        }
    }
    FOR_LOOP(i, count) {
        if (UI_RenderIsButtonFrameType(draw_order[i]->Type)) {
            UI_DrawButtonHighlight(draw_order[i]);
        }
    }
}
