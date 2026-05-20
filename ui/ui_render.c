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
                        .font = renderer ? renderer->LoadFont(frame->Font.Name, (DWORD)frame->Font.Size) : NULL,
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

    font_name = Theme_String(frame->Font.Name, "Default");
    font_size = frame->Font.Size > 0 ? (DWORD)(frame->Font.Size * 1000.0f) : 13;
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
