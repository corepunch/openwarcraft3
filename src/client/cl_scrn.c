#include <stdlib.h>

#include "client.h"

#define COMMAND_SIZE 0.039

static uiFrame_t const *frames;

struct {
    RECT rect;
    bool calculated;
} runtimes[MAX_LAYOUT_OBJECTS];

LPCRECT SCR_LayoutRect(uiFrame_t const *frame);

RECT get_uvrect(uint8_t const *texcoord) {
    RECT const uv = {
        texcoord[0],
        texcoord[2],
        texcoord[1] - texcoord[0],
        texcoord[3] - texcoord[2],
    };
    return uv;
}

RECT scale_rect(LPCRECT rect, float factor) {
    VECTOR2 diff = {
        rect->w * (1 - factor),
        rect->h * (1 - factor)
    };
    return (RECT) {
        .x = rect->x + diff.x / 2,
        .y = rect->y + diff.y / 2,
        .w = rect->w - diff.x,
        .h = rect->h - diff.y,
    };
}

VECTOR2 get_x(LPCRECT rect) {
    return (VECTOR2) { rect->x, rect->x + rect->w };
}

VECTOR2 get_y(LPCRECT rect) {
    return (VECTOR2) { rect->y, rect->y + rect->h };
}

float get_anchor(uiFramePoint_t const *p, VECTOR2 (*get)(LPCRECT)) {
    VECTOR2 b = get(SCR_LayoutRect(frames+p->relativeTo));
    if (p->targetPos == FPP_MID) {
        return (b.x + b.y) / 2 + p->offset;
    } else if (p->targetPos == FPP_MAX) {
        return b.y - p->offset;
    } else {
        return b.x + p->offset;
    }
}

VECTOR2 get_position(uiFrame_t const *frame,
                     uiFramePoints_t const p,
                     DWORD width,
                     VECTOR2 (*get)(LPCRECT))
{
    if (width == 0 && frame->flags.type == FT_SIMPLEFRAME) {
        VECTOR2 b = get(SCR_LayoutRect(frames+frame->parent));
        return (VECTOR2) { b.x, b.y - b.x };
    }
    uiFramePoint_t const *pmin = p+FPP_MIN;
    uiFramePoint_t const *pmid = p+FPP_MID;
    uiFramePoint_t const *pmax = p+FPP_MAX;
    if (pmid->used) {
        return (VECTOR2) { get_anchor(pmid, get) - width / 2, width, };
    } else if (pmin->used && pmax->used) {
        return (VECTOR2) {
            get_anchor(pmin, get),
            get_anchor(pmax, get) - get_anchor(pmin, get),
        };
    } else if (pmax->used) {
        return (VECTOR2) { get_anchor(pmax, get) - width, width };
    } else {
        return (VECTOR2) { get_anchor(pmin, get), width, };
    }
}

LPCRECT SCR_LayoutRect(uiFrame_t const *frame) {
    if (runtimes[frame->number].calculated) {
        return &runtimes[frame->number].rect;
    } else {
        runtimes[frame->number].calculated = true; // done here to avoid recursion
    }
    VECTOR2 const rect[] = {
        get_position(frame, frame->points.x, frame->size.width, get_x),
        get_position(frame, frame->points.y, frame->size.height, get_y),
    };
    runtimes[frame->number].rect = (RECT) {
        .x = rect[0].x,
        .y = rect[1].x,
        .w = rect[0].y,
        .h = rect[1].y,
    };
    return &runtimes[frame->number].rect;
}

void layout_tex(uiFrame_t const *frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), 10000);
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    re.DrawImage(cl.pics[frame->tex.index], &screen, &suv);
}

void layout_portrait(uiFrame_t const *frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), 10000);
    RECT const viewport = {
        screen.x/0.8, 1 - (screen.y + screen.h)/ 0.6, screen.w/0.8, screen.h/0.6
    };
    re.DrawPortrait(cl.portraits[frame->tex.index], &viewport);
}

void layout_cmd(uiFrame_t const *frame) {
    RECT screen = Rect_div(SCR_LayoutRect(frame), 10000);
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    VECTOR2 m = {
        mouse.origin.x * 0.8 / WINDOW_WIDTH,
        mouse.origin.y * 0.6 / WINDOW_HEIGHT
    };
    if (Rect_contains(&screen, &m)) {
        if (mouse.button == 1 || mouse.event == UI_LEFT_MOUSE_UP) {
            screen = scale_rect(&screen, 0.95);
        }
        if (mouse.event == UI_LEFT_MOUSE_UP) {
            MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
            SZ_Printf(&cls.netchan.message, "button %d", frame->code);
        }
    }

    re.DrawImage(cl.pics[frame->tex.index], &screen, &suv);
}

void layout_string(uiFrame_t const *frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), 10000);
    COLOR32 color = { 255, 0, 0, 255 };
    char text[64] = { 0 };
    if (frame->stat > 0) {
        sprintf(text, "%d", cl.playerstate.stats[frame->stat]);
    }
    re.DrawText(&(drawText_t) {
        .font = cl.fonts[frame->font.index],
        .text = text,
        .rect = screen,
        .color = color,
        .halign = frame->flags.textalignx,
        .valign = frame->flags.textaligny,
    });
}

void SCR_Clear(uiFrame_t const *_frames) {
    memset(runtimes, 0, sizeof(runtimes));
    frames = _frames;
}

void SCR_DrawOverlay(uiFrame_t const *_frames) {
    SCR_Clear(_frames);
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        uiFrame_t const *frame = frames+i;
        switch (frame->flags.type) {
            case FT_TEXTURE: layout_tex(frame); break;
            case FT_COMMANDBUTTON: layout_cmd(frame); break;
            case FT_STRING: layout_string(frame); break;
            case FT_PORTRAIT: layout_portrait(frame); break;
            case FT_NONE:
                return;
            default:
                break;
        }
    }
}

void SCR_DrawOverlays(void) {
    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        SCR_DrawOverlay(cl.layout[layer]);
    }
}

void SCR_UpdateScreen(void) {
    re.BeginFrame();
    
    V_RenderView();

    SCR_DrawOverlays();

    CON_DrawConsole();
    
    re.EndFrame();
}
