#include <stdlib.h>

#include "client.h"

#define COMMAND_SIZE 0.039
#define UI_SCALE 10000

static LPCSTR active_tooltip = NULL;

static UIFRAME frames[MAX_LAYOUT_OBJECTS];

struct {
    RECT rect;
    bool calculated;
} runtimes[MAX_LAYOUT_OBJECTS];

LPCRECT SCR_LayoutRect(LPCUIFRAME frame);

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

LPCRECT SCR_LayoutRectByNumber(LPCUIFRAME context, DWORD number) {
    if (number == UI_PARENT) {
        return SCR_LayoutRect(frames+context->parent);
    } else {
        return SCR_LayoutRect(frames+number);
    }
}

float SCR_GetAnchor(LPCUIFRAME f,
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

VECTOR2 get_position(LPCUIFRAME frame,
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

LPCSTR SCR_GetStringValue(LPCUIFRAME frame) {
    static char text[1024] = { 0 };
    if (frame->text) {
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

LPCRECT SCR_LayoutRect(LPCUIFRAME frame) {
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
            elemsize =
            re.GetTextSize(&MAKE(DRAWTEXT,
                                 .font = cl.fonts[frame->font.index],
                                 .text = SCR_GetStringValue(frame),
                                 .textWidth = 9999));
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
        ((LPUIFRAME )frame)->size.width = elemsize.x;
    }
    if (frame->size.height == 0) {
        ((LPUIFRAME )frame)->size.height = elemsize.y;
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

void layout_statusbar(LPCUIFRAME frame, LPCRECT screen) {
    RECT const uv = { 0, 0, 255, 255 };
    RECT screen2 = *screen;
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
        re.DrawImage(cl.pics[frame->tex.index2], screen, &suv, COLOR32_WHITE);
    }
}

void layout_texture(LPCUIFRAME frame, LPCRECT screen) {
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    re.DrawImage(cl.pics[frame->tex.index], screen, &suv, frame->color);
}

typedef enum {
    BACKDROP_TOP_LEFT_CORNER,
    BACKDROP_TOP_EDGE,
    BACKDROP_TOP_RIGHT_CORNER,
    BACKDROP_LEFT_EDGE,
    BACKDROP_CENTER,
    BACKDROP_RIGHT_EDGE,
    BACKDROP_BOTTOM_LEFT_CORNER,
    BACKDROP_BOTTOM_EDGE,
    BACKDROP_BOTTOM_RIGHT_CORNER,
    BACKDROP_SIZE,
} BACKDROPCORNER;

typedef enum {
    BACKDROPINSET_RIGHT,
    BACKDROPINSET_TOP,
    BACKDROPINSET_BOTTOM,
    BACKDROPINSET_LEFT,
} BACKDROPINSET;

#define NUM_BACKDROP_CORNERS 8

void get_rects(LPCRECT screen, LPRECT rects, float corner_size) {
    float x[] = { 0, corner_size, screen->w - corner_size, screen->w };
    float y[] = { 0, corner_size, screen->h - corner_size, screen->h };
    FOR_LOOP(i, BACKDROP_SIZE) {
        rects[i].x = screen->x + x[i % 3];
        rects[i].y = screen->y + y[i / 3];
        rects[i].w = x[(i % 3) + 1] - x[i % 3];
        rects[i].h = y[(i / 3) + 1] - y[i / 3];
    }
}

void layout_backdrop(LPCUIFRAME frame, LPCRECT screen) {
    RECT const uv = { 0, 0, 1, 1};
    
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
    
    UINAME CornerFlags;// "UL|UR|BL|BR|T|L|B|R",
    LONG TileBackground;
    LONG Background;
    LONG CornerSize;
    LONG BackgroundSize;
    LONG BackgroundInsets[4];// 0.01 0.01 0.01 0.01,
    LONG EdgeFile;//  "EscMenuBorder",
    LONG BlendAll;
    
    memset(CornerFlags, 0, sizeof(UINAME));
    memcpy(CornerFlags, frame->text, strchr(frame->text, ',') - frame->text);
    
    sscanf(strchr(frame->text, ',') + 1, "%d,%d,%d,%d,%d %d %d %d,%d,%d,",
           &TileBackground,
           &Background,
           &CornerSize,
           &BackgroundSize,
           &BackgroundInsets[0],
           &BackgroundInsets[1],
           &BackgroundInsets[2],
           &BackgroundInsets[3],
           &EdgeFile,
           &BlendAll);
    
    RECT rects[BACKDROP_SIZE];
    get_rects(screen, rects, CornerSize / (float)UI_SCALE);
    
    FOR_LOOP(i, NUM_BACKDROP_CORNERS) {
        float const k = 1.0 / NUM_BACKDROP_CORNERS;
        RECT const rect = { i * k, 0, k, 1 };
        bool const flip = (corners[i] == BACKDROP_TOP_EDGE || corners[i] == BACKDROP_BOTTOM_EDGE);
        re.DrawImageEx(cl.pics[EdgeFile], &rects[corners[i]], &rect, frame->color, flip);
    }
    
    RECT background = *screen;
    background.x += BackgroundInsets[BACKDROPINSET_LEFT] / (float)UI_SCALE;
    background.y += BackgroundInsets[BACKDROPINSET_TOP] / (float)UI_SCALE;
    background.w -= BackgroundInsets[BACKDROPINSET_LEFT] / (float)UI_SCALE;
    background.w -= BackgroundInsets[BACKDROPINSET_RIGHT] / (float)UI_SCALE;
    background.h -= BackgroundInsets[BACKDROPINSET_TOP] / (float)UI_SCALE;
    background.h -= BackgroundInsets[BACKDROPINSET_BOTTOM] / (float)UI_SCALE;
    re.DrawImage(cl.pics[Background], &background, &uv, frame->color);
}

void layout_buildqueue(LPCUIFRAME frame, LPCRECT scrn) {
    static UINAME buffer = { 0 };
    RECT screen = *scrn;
    RECT const uv = { 0, 0, 1, 1 };
    DWORD first_image, time_bar, offset_x;
    bool first_item = true;
    sscanf(frame->text, "%x %x %x,", &first_image, &time_bar, &offset_x);
    strcpy(buffer, strchr(frame->text, ',')+1);
    for (LPCSTR token = strtok(buffer, ","); token != NULL; token = strtok(NULL, ",")) {
        DWORD img = 0, nument = 0;
        sscanf(token, "%x %x", &img, &nument);
        entityState_t *ent = &cl.ents[nument].current;
        if (ent->stats[ENT_HEALTH] == 255) {
            continue;
        } else if (first_item) {
            ((LPUIFRAME )frames)[time_bar].value = BYTE2FLOAT(ent->stats[ENT_HEALTH]);
            ((LPUIFRAME )frames)[first_image].tex.index = img;
        } else {
            re.DrawImage(cl.pics[img], &screen, &uv, frame->color);
            screen.x += (float)offset_x / (float)UI_SCALE;
        }
        first_item = false;
    }
}

#define HP_BAR_HEIGHT_RATIO 0.175f
#define HP_BAR_SPACING_RATIO 0.02f

void layout_multiselect(LPCUIFRAME frame, LPCRECT scrn) {
    static UINAME buffer = { 0 };
    RECT screen = *scrn;
    DWORD hp_bar, mana_bar, columns;
    DWORD offset_x, offset_y;
    sscanf(frame->text, "%x %x %x %x %x,", &hp_bar, &mana_bar, &columns, &offset_x, &offset_y);
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
            screen.y += (float)offset_y / (float)UI_SCALE;
        } else {
            screen.x += (float)offset_x / (float)UI_SCALE;
        }
    }
}

void layout_portrait(LPCUIFRAME frame, LPCRECT screen) {
    RECT const viewport = {
        screen->x/0.8,1-(screen->y+screen->h)/0.6,screen->w/0.8,screen->h/0.6
    };
    LPCMODEL port = cl.portraits[frame->tex.index];
    re.DrawPortrait(port ? port : cl.models[frame->tex.index], &viewport);
}

void layout_cmd(LPCUIFRAME frame, LPCRECT screen) {
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    VECTOR2 m = {
        mouse.origin.x * 0.8 / WINDOW_WIDTH,
        mouse.origin.y * 0.6 / WINDOW_HEIGHT
    };
    RECT scrn = *screen;
    if (Rect_contains(screen, &m)) {
        if (mouse.button == 1 || mouse.event == UI_LEFT_MOUSE_UP) {
            scrn = scale_rect(&scrn, 0.95);
        }
        if (mouse.event == UI_LEFT_MOUSE_UP) {
            MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
            SZ_Printf(&cls.netchan.message, "button %s", frame->text);
        }
    }

    re.DrawImage(cl.pics[frame->tex.index], &scrn, &suv, COLOR32_WHITE);
}

DRAWTEXT get_drawtext(LPCUIFRAME frame, FLOAT avl_width, LPCSTR text) {
    return MAKE(DRAWTEXT,
                .font = cl.fonts[frame->font.index],
                .text = text,
                .color = frame->color,
                .halign = frame->flags.textalignx,
                .valign = frame->flags.textaligny,
                .icons = cl.pics,
                .lineHeight = 1.33,
                .textWidth = avl_width);
}

void layout_text(LPCUIFRAME frame, LPCRECT screen, LPCSTR text, bool wordWrap) {
    DRAWTEXT drawtext = get_drawtext(frame, screen->w, text);
    drawtext.rect = *screen;
    drawtext.wordWrap = wordWrap;
    re.DrawText(&drawtext);
}

void layout_string(LPCUIFRAME frame, LPCRECT screen) {
    layout_text(frame, screen, SCR_GetStringValue(frame), false);
}

RECT Rect_inset(LPCRECT r, float inset) {
    return MAKE(RECT,r->x+inset,r->y+inset,r->w-inset*2,r->h-inset*2);
}

void layout_tooltiptext(LPCUIFRAME frame, LPCRECT scrn) {
    if (active_tooltip) {
        RECT screen = *scrn;
        FLOAT const PADDING = 0.005;
        DRAWTEXT drawtext = get_drawtext(frame, screen.w - PADDING * 2, active_tooltip);
        VECTOR2 textsize = re.GetTextSize(&drawtext);
        textsize.y += PADDING * 2;
        screen.y += screen.h - textsize.y;
        screen.h = textsize.y;
        RECT text = Rect_inset(&screen, PADDING);
        layout_backdrop(frame, &screen);
        layout_text(frame, &text, active_tooltip, true);
    }
}

LPCUIFRAME SCR_Clear(HANDLE data) {
    memset(runtimes, 0, sizeof(runtimes));
    memset(frames, 0, sizeof(frames));
    frames[0].size.width = 8000;
    frames[0].size.height = 6000;
    frames[0].flags.type = FT_SCREEN;
    sizeBuf_t msg = {
        .data = data,
        .cursize = 100000,
        .readcount = 0,
    };
    while (true) {
        DWORD bits = 0;
        int nument = MSG_ReadEntityBits(&msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        LPUIFRAME ent = &frames[nument];
        ent->tex.coord[1] = 0xff;
        ent->tex.coord[3] = 0xff;
        MSG_ReadDeltaUIFrame(&msg, ent, nument, bits);
    }
    return frames;
}

typedef struct {
    uiFrameType_t type;
    void (*func)(LPCUIFRAME, LPCRECT);
} drawer_t;

static drawer_t drawers[] = {
    { FT_TEXTURE, layout_texture },
    { FT_BACKDROP, layout_backdrop },
    { FT_SIMPLESTATUSBAR, layout_statusbar },
    { FT_COMMANDBUTTON, layout_cmd },
    { FT_STRING, layout_string },
    { FT_TEXT, layout_string },
    { FT_TOOLTIPTEXT, layout_tooltiptext },
    { FT_PORTRAIT, layout_portrait },
    { FT_BUILDQUEUE, layout_buildqueue },
    { FT_MULTISELECT, layout_multiselect },
//    { FT_NONE, NULL },
};

void SCR_DrawFrame(LPCUIFRAME frame) {
    RECT const screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
    FOR_LOOP(j, sizeof(drawers)/sizeof(*drawers)) {
        if (drawers[j].type == frame->flags.type) {
            drawers[j].func(frame, &screen);
            break;
        }
    }
}

void SCR_UpdateTooltip(HANDLE _frames) {
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        LPCUIFRAME frame = frames+i;
        if (frame->flags.type != FT_COMMANDBUTTON) {
            continue;
        }
        VECTOR2 m = {
            mouse.origin.x * 0.8 / WINDOW_WIDTH,
            mouse.origin.y * 0.6 / WINDOW_HEIGHT
        };
        RECT const screen = Rect_div(SCR_LayoutRect(frame), UI_SCALE);
        if (Rect_contains(&screen, &m) && frame->tooltip) {
            active_tooltip = frame->tooltip;
        }
    }
}

void SCR_DrawOverlay(HANDLE _frames) {
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        SCR_DrawFrame(frames+i);
    }
}

void SCR_DrawOverlays(void) {
    active_tooltip = NULL;

    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        HANDLE *layout = cl.layout[layer];
        if (layout) {
            SCR_Clear(layout);
            SCR_UpdateTooltip(layout);
        }
    }
    
    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        HANDLE *layout = cl.layout[layer];
        if (layout) {
            SCR_Clear(layout);
            SCR_DrawOverlay(layout);
        }
    }
}

void SCR_UpdateScreen(void) {
    re.BeginFrame();
    
    V_RenderView();

    SCR_DrawOverlays();

    CON_DrawConsole();
    
    re.EndFrame();
}
