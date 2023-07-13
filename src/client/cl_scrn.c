#include <stdlib.h>

#include "client.h"

#define COMMAND_SIZE 0.039
#define UI_SCALE 10000

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

LPCRECT SCR_LayoutRectByNumber(uiFrame_t const *context, DWORD number) {
    if (number == UI_PARENT) {
        return SCR_LayoutRect(frames+context->parent);
    } else {
        return SCR_LayoutRect(frames+number);
    }
}

float SCR_GetAnchor(uiFrame_t const *f,
                    uiFramePoint_t const *p,
                    VECTOR2 (*get)(LPCRECT))
{
    VECTOR2 b = get(SCR_LayoutRectByNumber(f, p->relativeTo));
    float offset = get == get_x ? p->offset : -p->offset;
    if (p->targetPos == FPP_MID) {
        return (b.x + b.y) / 2 + offset;
    } else if (p->targetPos == FPP_MAX) {
        return b.y + offset;
    } else {
        return b.x + offset;
    }
}

VECTOR2 get_position(uiFrame_t const *frame,
                     uiFramePoints_t const p,
                     DWORD width,
                     VECTOR2 (*get)(LPCRECT))
{
    uiFramePoint_t const *pmin = p+FPP_MIN;
    uiFramePoint_t const *pmid = p+FPP_MID;
    uiFramePoint_t const *pmax = p+FPP_MAX;
    if (pmid->used) {
        return (VECTOR2) {
            SCR_GetAnchor(frame, pmid, get) - width / 2,
            width,
        };
    } else if (pmin->used && pmax->used) {
        return (VECTOR2) {
            SCR_GetAnchor(frame, pmin, get),
            SCR_GetAnchor(frame, pmax, get) - SCR_GetAnchor(frame, pmin, get),
        };
    } else if (pmax->used) {
        return (VECTOR2) {
            SCR_GetAnchor(frame, pmax, get) - width,
            width,
        };
    } else {
        return (VECTOR2) {
            SCR_GetAnchor(frame, pmin, get),
            width,
        };
    }
}

LPCSTR SCR_GetStringValue(uiFrame_t const *frame) {
    static char text[1024] = { 0 };
    if (frame->text[0]) {
        return frame->text;
    } else if (frame->stat == STAT_FOOD) {
        DWORD food_used = cl.playerstate.stats[STAT_FOOD_USED];
        DWORD food_made = cl.playerstate.stats[STAT_FOOD_MADE];
        sprintf(text, "%d/%d", food_used, food_made);
    } else if (frame->stat > 0) {
        sprintf(text, "%d", cl.playerstate.stats[frame->stat]);
    } else {
        sprintf(text, "text %d", frame->number);
    }
    return text;
}

LPCRECT SCR_LayoutRect(uiFrame_t const *frame) {
    if (runtimes[frame->number].calculated) {
        return &runtimes[frame->number].rect;
    } else {
        runtimes[frame->number].calculated = true; // done here to avoid recursion
    }
    VECTOR2 elemsize;
    size2_t imagesize;
    switch (frame->flags.type) {
        case FT_STRING:
        case FT_TEXT:
            elemsize = re.GetTextSize(cl.fonts[frame->font.index], SCR_GetStringValue(frame));
            elemsize.x *= UI_SCALE;
            elemsize.y *= UI_SCALE;
            break;
        case FT_TEXTURE:
        case FT_SIMPLESTATUSBAR:
            imagesize = re.GetTextureSize(cl.pics[frame->tex.index]);
            elemsize.x = imagesize.width * 8;
            elemsize.y = imagesize.height * 6;
            break;
        default:
            break;
    }
    if (frame->size.width == 0) {
        ((uiFrame_t *)frame)->size.width = elemsize.x;
    }
    if (frame->size.height == 0) {
        ((uiFrame_t *)frame)->size.height = elemsize.y;
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

void layout_statusbar(uiFrame_t const *frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
    RECT const uv = { 0, 0, 255, 255 };
    RECT screen2 = screen;
    RECT uv2 = uv;
//    if (frame->stat > 0 ) {
//        screen2.w *= cl.playerstate.unit_stats[frame->stat] / (float)255;
//        uv2.w *= cl.playerstate.unit_stats[frame->stat] / (float)255;
//    } else {
        screen2.w *= frame->value;
        uv2.w *= frame->value;
//    }
    RECT const suv2 = Rect_div(&uv2, 0xff);
    re.DrawImage(cl.pics[frame->tex.index], &screen2, &suv2, frame->color);
    if (frame->tex.index2 > 0) {
        RECT const suv = Rect_div(&uv, 0xff);
        re.DrawImage(cl.pics[frame->tex.index2], &screen, &suv, COLOR32_WHITE);
    }
}

void layout_texture(uiFrame_t const *frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    re.DrawImage(cl.pics[frame->tex.index], &screen, &suv, frame->color);
}

void layout_buildqueue(uiFrame_t const *frame) {
    static UINAME buffer = { 0 };
    RECT screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
    RECT const uv = { 0, 0, 1, 1 };
    DWORD start_time, first_image, time_bar;
    bool first_item = true;
    sscanf(frame->text, "%x %x %x,", &first_image, &time_bar, &start_time);
    strcpy(buffer, strchr(frame->text, ',')+1);
    for (LPCSTR token = strtok(buffer, ","); token != NULL; token = strtok(NULL, ",")) {
        DWORD img = 0, end_time = 0;
        sscanf(token, "%x %x", &img, &end_time);
        if (end_time < cl.time) {
            start_time = end_time;
            continue;
        } else if (first_item) {
            if (start_time != end_time) {
                float total = end_time - start_time;
                float current = cl.time - start_time;
                ((uiFrame_t *)frames)[time_bar].value = MIN(1, current / total);
            }
            ((uiFrame_t *)frames)[first_image].tex.index = img;
        } else {
            re.DrawImage(cl.pics[img], &screen, &uv, frame->color);
            screen.x += (float)frame->offset.x / (float)UI_SCALE;
        }
        first_item = false;
    }
}

#define HP_BAR_HEIGHT_RATIO 0.175f
#define HP_BAR_SPACING_RATIO 0.02f

void layout_multiselect(uiFrame_t const *frame) {
    static UINAME buffer = { 0 };
    RECT screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
    DWORD hp_bar, mana_bar, columns;
    sscanf(frame->text, "%x %x %x,", &hp_bar, &mana_bar, &columns);
    strcpy(buffer, strchr(frame->text, ',')+1);
    DWORD column = 0;
    for (LPCSTR token = strtok(buffer, ","); token != NULL; token = strtok(NULL, ",")) {
        RECT uv = { 0, 0, 1, 1 };
        DWORD img = 0, nument = 0;
        sscanf(token, "%x %x", &img, &nument);
        re.DrawImage(cl.pics[img], &screen, &uv, frame->color);
        if (nument < MAX_CLIENT_ENTITIES) {
            float health = BYTE2FLOAT(cl.ents[nument].current.stats[ENT_HEALTH]);
            float mana = BYTE2FLOAT(cl.ents[nument].current.stats[ENT_MANA]);
            RECT rect = {
                screen.x,
                screen.y + screen.h * (1 + HP_BAR_SPACING_RATIO),
                screen.w * health,
                screen.h * HP_BAR_HEIGHT_RATIO
            };
            uv.w = health;
            re.DrawImage(cl.pics[hp_bar], &rect, &uv, MAKE(COLOR32,0,255,0,255));
            uv.w = mana;
            rect.w = screen.w * mana;
            rect.y += screen.h * (HP_BAR_HEIGHT_RATIO + HP_BAR_SPACING_RATIO);
            re.DrawImage(cl.pics[mana_bar], &rect, &uv, MAKE(COLOR32,0,255,255,255));
        }
        if (++column >= columns) {
            column = 0;
            screen.x = Rect_div(SCR_LayoutRect(frame), UI_SCALE).x;
            screen.y += (float)frame->offset.y / (float)UI_SCALE;
        } else {
            screen.x += (float)frame->offset.x / (float)UI_SCALE;
        }
    }
}

void layout_portrait(uiFrame_t const *frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
    RECT const viewport = {
        screen.x/0.8,1-(screen.y+screen.h)/0.6,screen.w/0.8,screen.h/0.6
    };
    LPCMODEL port = cl.portraits[frame->tex.index];
    re.DrawPortrait(port ? port : cl.models[frame->tex.index], &viewport);
}

void layout_cmd(uiFrame_t const *frame) {
    RECT screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
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
            SZ_Printf(&cls.netchan.message, "button %s", frame->text);
        }
    }

    re.DrawImage(cl.pics[frame->tex.index], &screen, &suv, COLOR32_WHITE);
}

void layout_string(uiFrame_t const *frame) {
    LPCSTR text = SCR_GetStringValue(frame);
    RECT screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);

    re.DrawText(&(drawText_t) {
        .font = cl.fonts[frame->font.index],
        .text = text,
        .rect = screen,
        .color = frame->color,
        .halign = frame->flags.textalignx,
        .valign = frame->flags.textaligny,
    });
}

void SCR_Clear(uiFrame_t const *_frames) {
    memset(runtimes, 0, sizeof(runtimes));
    frames = _frames;
}

typedef struct {
    uiFrameType_t type;
    void (*func)(uiFrame_t const *);
} drawer_t;

static drawer_t drawers[] = {
    { FT_TEXTURE, layout_texture },
    { FT_BACKDROP, layout_texture },
    { FT_SIMPLESTATUSBAR, layout_statusbar },
    { FT_COMMANDBUTTON, layout_cmd },
    { FT_STRING, layout_string },
    { FT_TEXT, layout_string },
    { FT_PORTRAIT, layout_portrait },
    { FT_BUILDQUEUE, layout_buildqueue },
    { FT_MULTISELECT, layout_multiselect },
//    { FT_NONE, NULL },
};

void SCR_DrawFrame(uiFrame_t const *frame) {
    FOR_LOOP(j, sizeof(drawers)/sizeof(*drawers)) {
        if (drawers[j].type == frame->flags.type) {
            drawers[j].func(frame);
            break;
        }
    }
}

void SCR_DrawOverlay(uiFrame_t const *_frames) {
    SCR_Clear(_frames);
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        SCR_DrawFrame(frames+i);
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
