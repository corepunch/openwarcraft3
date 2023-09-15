#include <stdlib.h>

#include "client.h"

#define PLAYERSTATE_RESOURCE_FOOD_CAP 4
#define PLAYERSTATE_RESOURCE_FOOD_USED 5

static LPCSTR active_tooltip = NULL;
static UIFRAME frames[MAX_LAYOUT_OBJECTS];
static DWORD num_frames = 0;

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

RECT scale_rect(LPCRECT rect, FLOAT factor) {
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

LPCENTITYSTATE CL_SelectedEntity(void) {
    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        centity_t const *ce = &cl.ents[index];
        if (ce->current.renderfx & RF_SELECTED) {
            return &ce->current;
        }
    }
    return NULL;
}

LPCRECT SCR_LayoutRectByNumber(LPCUIFRAME context, DWORD number) {
    if (number == UI_PARENT) {
        return SCR_LayoutRect(frames+context->parent);
    } else {
        return SCR_LayoutRect(frames+number);
    }
}

FLOAT SCR_GetAnchor(LPCUIFRAME f,
                    uiFramePoint_t const *p,
                    VECTOR2 (*get)(LPCRECT))
{
    VECTOR2 b = get(SCR_LayoutRectByNumber(f, p->relativeTo));
    SHORT offset = get == get_x ? p->offset : -p->offset;
    if (p->targetPos == FPP_MID) {
        return (b.x + b.y) / 2 + offset / UI_FRAMEPOINT_SCALE;
    } else if (p->targetPos == FPP_MAX) {
        return b.y + offset / UI_FRAMEPOINT_SCALE;
    } else {
        return b.x + offset / UI_FRAMEPOINT_SCALE;
    }
}

VECTOR2 get_position(LPCUIFRAME frame,
                     uiFramePoints_t const p,
                     FLOAT width,
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
    if (frame->stat >= MAX_STATS) {
        if (cl.playerstate.texts[frame->stat - MAX_STATS]) {
            strcpy(text, cl.playerstate.texts[frame->stat - MAX_STATS]);
        } else {
            memset(text, 0, sizeof(text));
        }
    } else if (frame->stat > 0) {
        sprintf(text, "%d", cl.playerstate.stats[frame->stat]);
    } else if (frame->text) {
        return frame->text;
    } else if (frame->stat == PLAYERSTATE_RESOURCE_FOOD_USED) {
        DWORD food_used = cl.playerstate.stats[PLAYERSTATE_RESOURCE_FOOD_USED];
        DWORD food_made = cl.playerstate.stats[PLAYERSTATE_RESOURCE_FOOD_CAP];
        sprintf(text, "%d/%d", food_used, food_made);
    } else {
        sprintf(text, "text %d", frame->number);
    }
    return text;
}

DRAWTEXT get_drawtext(LPCUIFRAME frame,
                      FLOAT avl_width,
                      LPCSTR text,
                      uiLabel_t const *label)
{
    return MAKE(DRAWTEXT,
                .font = cl.fonts[label->font],
                .text = text,
                .color = frame->color,
                .halign = label->textalignx,
                .valign = label->textaligny,
                .icons = cl.pics,
                .lineHeight = 1.33,
                .wordWrap = true,
                .textWidth = avl_width);
}

LPCRECT SCR_LayoutRect(LPCUIFRAME frame) {
    if (runtimes[frame->number].calculated) {
        return &runtimes[frame->number].rect;
    } else {
        runtimes[frame->number].calculated = true; // done here to avoid recursion
    }
    if (frame->flags.type == FT_PORTRAIT) {
        int a=0;
    }
    VECTOR2 elemsize;
    size2_t imagesize;
    FLOAT avl_space = runtimes[0].rect.w;
    DRAWTEXT drawtext;
    switch (frame->flags.type) {
        case FT_STRING:
        case FT_TEXT:
            if (frame->size.width > 0) {
                avl_space = frame->size.width;
            }
            drawtext = get_drawtext(frame, avl_space, SCR_GetStringValue(frame), frame->buffer.data);
            elemsize = re.GetTextSize(&drawtext);
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

void SCR_DrawStatusbar(LPCUIFRAME frame, LPCRECT screen) {
    RECT const uv = { 0, 0, 255, 255 };
    RECT screen2 = *screen;
    RECT uv2 = uv;
//    if (frame->stat > 0 ) {
//        screen2.w *= cl.playerstate.unit_stats[frame->stat] / (FLOAT)255;
//        uv2.w *= cl.playerstate.unit_stats[frame->stat] / (FLOAT)255;
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

void SCR_DrawTexture(LPCUIFRAME frame, LPCRECT screen) {
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    re.DrawImage(cl.pics[frame->tex.index], screen, &suv, frame->color);
}

void SCR_DrawHighlight(LPCUIFRAME frame, LPCRECT screen) {
    uiHighlight_t const *highlight = frame->buffer.data;
    re.DrawImageEx(&MAKE(DRAWIMAGE,
                         .texture = cl.pics[highlight->alphaFile],
                         .alphamode = highlight->alphaMode,
                         .screen = *screen,
                         .uv = MAKE(RECT,0,0,1,1),
                         .color = COLOR32_WHITE,
                         .rotate = false,
                         .shader = SHADER_UI));
}

void SCR_SimpleButton(LPCUIFRAME frame, LPCRECT screen) {
    uiSimpleButton_t *button = frame->buffer.data;
    LPCSTR label = frame->text;
    RECT const uv = get_uvrect((BYTE *)&button->normal.texcoord);
    RECT const suv = Rect_div(&uv, 0xff);
    re.DrawImage(cl.pics[button->normal.texture], screen, &suv, COLOR32_WHITE);
    re.DrawText(&MAKE(DRAWTEXT,
                      .rect = *screen,
                      .font = cl.fonts[button->normal.font],
                      .text = label,
                      .color = button->normal.fontcolor,
                      .textWidth = screen->w));
}

typedef enum {
    BACKDROPINSET_RIGHT,
    BACKDROPINSET_TOP,
    BACKDROPINSET_BOTTOM,
    BACKDROPINSET_LEFT,
} BACKDROPINSET;

#define NUM_BACKDROP_CORNERS 8

void backdrop_rects(LPCRECT screen, LPRECT rects, FLOAT corner_size) {
    FLOAT x[] = { 0, corner_size, screen->w - corner_size, screen->w };
    FLOAT y[] = { 0, corner_size, screen->h - corner_size, screen->h };
    FOR_LOOP(i, BACKDROP_SIZE) {
        rects[i].x = screen->x + x[i % 3];
        rects[i].y = screen->y + y[i / 3];
        rects[i].w = x[(i % 3) + 1] - x[i % 3];
        rects[i].h = y[(i / 3) + 1] - y[i / 3];
    }
}

FLOAT backdrop_edge_tile(LPCRECT rect, BACKDROPCORNER edge, FLOAT imagesize) {
    switch (edge) {
        case BACKDROP_LEFT_EDGE:
        case BACKDROP_RIGHT_EDGE:
            return ceil(rect->h / imagesize);
        case BACKDROP_TOP_EDGE:
        case BACKDROP_BOTTOM_EDGE:
            return ceil(rect->w / imagesize);
        default:
            return 1;
    }
}

BOOL backdrop_edge_flip(BACKDROPCORNER edge) {
    switch (edge) {
        case BACKDROP_TOP_EDGE:
        case BACKDROP_BOTTOM_EDGE:
            return true;
        default:
            return false;
    }
}

void SCR_DrawBackdrop2(LPCUIFRAME frame, LPCRECT screen, uiBackdrop_t const *backdrop) {
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
    backdrop_rects(screen, rects, backdrop->CornerSize);
    
    size2_t backSize = re.GetTextureSize(cl.pics[backdrop->Background]);
    size2_t edgeSize = re.GetTextureSize(cl.pics[backdrop->EdgeFile]);
    
    FOR_LOOP(i, NUM_BACKDROP_CORNERS) {
        if ((backdrop->CornerFlags & (1 << corners[i])) == 0)
            continue;
        FLOAT const k = 1.0 / NUM_BACKDROP_CORNERS;
        FLOAT const h = edgeSize.height / 1000.f;
        FLOAT const tile = backdrop_edge_tile(rects+corners[i], corners[i], h);
        if (tile > 100) {
            backdrop_edge_tile(rects+corners[i], corners[i], h);
        }
        BOOL const flip = backdrop_edge_flip(corners[i]);
        RECT const rect = { i * k, 0, k, tile };
        re.DrawImageEx(&MAKE(DRAWIMAGE,
                             .texture = cl.pics[backdrop->EdgeFile],
                             .screen = rects[corners[i]],
                             .uv = rect,
                             .color = frame->color,
                             .rotate = flip,
                             .shader = SHADER_UI));
    }
    
    RECT uv = { 0, 0, 1, 1};
    RECT background = *screen;
    background.x += backdrop->BackgroundInsets[BACKDROPINSET_LEFT];
    background.y += backdrop->BackgroundInsets[BACKDROPINSET_TOP];
    background.w -= backdrop->BackgroundInsets[BACKDROPINSET_LEFT];
    background.w -= backdrop->BackgroundInsets[BACKDROPINSET_RIGHT];
    background.h -= backdrop->BackgroundInsets[BACKDROPINSET_TOP];
    background.h -= backdrop->BackgroundInsets[BACKDROPINSET_BOTTOM];
    if (backdrop->TileBackground) {
        uv.w = background.w / (backSize.width / 1000.f);
        uv.h = background.h / (backSize.height / 1000.f);
    }
    re.DrawImage(cl.pics[backdrop->Background], &background, &uv, frame->color);
}

void SCR_DrawBackdrop(LPCUIFRAME frame, LPCRECT screen) {
    SCR_DrawBackdrop2(frame, screen, frame->buffer.data);
}

void SCR_GlueTextButton(LPCUIFRAME frame, LPCRECT screen) {
    uiGlueTextButton_t const *gluetextbutton = frame->buffer.data;
    SCR_DrawBackdrop2(frame, screen, &gluetextbutton->normal);
}

void SCR_DrawBuildQueue(LPCUIFRAME frame, LPCRECT scrn) {
    RECT screen = *scrn;
    RECT const uv = { 0, 0, 1, 1 };
    bool first_item = true;
    uiBuildQueue_t const *queue = frame->buffer.data;
    FOR_LOOP(i, queue->numitems) {
        LPENTITYSTATE ent = &cl.ents[queue->items[i].entity].current;
        if (ent->stats[ENT_HEALTH] == 255) {
            continue;
        } else if (!first_item) {
            re.DrawImage(cl.pics[queue->items[i].image], &screen, &uv, frame->color);
            screen.x += queue->itemoffset;
        }
        first_item = false;
    }
}

void SCR_UpdateBuildQueue(LPCUIFRAME frame, LPCRECT screen) {
    uiBuildQueue_t const *queue = frame->buffer.data;
    FOR_LOOP(i, queue->numitems) {
        LPENTITYSTATE ent = &cl.ents[queue->items[i].entity].current;
        if (ent->stats[ENT_HEALTH] != 255) {
            ((LPUIFRAME)frames)[queue->buildtimer].value = BYTE2FLOAT(ent->stats[ENT_HEALTH]);
            ((LPUIFRAME)frames)[queue->firstitem].tex.index = queue->items[i].image;
            break;
        }
    }
}

#define HP_BAR_HEIGHT_RATIO 0.175f
#define HP_BAR_SPACING_RATIO 0.02f

void SCR_DrawMultiSelect(LPCUIFRAME frame, LPCRECT scrn) {
    RECT screen = *scrn;
    uiMultiselect_t const *multiselect = frame->buffer.data;
    DWORD column = 0;
    FOR_LOOP(i, multiselect->numitems) {
        RECT uv = { 0, 0, 1, 1 };
        uiBuildQueueItem_t const *item = &multiselect->items[i];
        re.DrawImage(cl.pics[item->image], &screen, &uv, frame->color);
        if (item->entity < MAX_CLIENT_ENTITIES) {
            FLOAT health = BYTE2FLOAT(cl.ents[item->entity].current.stats[ENT_HEALTH]);
            FLOAT mana = BYTE2FLOAT(cl.ents[item->entity].current.stats[ENT_MANA]);
            RECT rect = {
                screen.x,
                screen.y + screen.h * (1 + HP_BAR_SPACING_RATIO),
                screen.w * health,
                screen.h * HP_BAR_HEIGHT_RATIO
            };
            uv.w = health;
            re.DrawImage(cl.pics[multiselect->hp_bar], &rect, &uv, MAKE(COLOR32,0,255,0,255));
            uv.w = mana;
            rect.w = screen.w * mana;
            rect.y += screen.h * (HP_BAR_HEIGHT_RATIO + HP_BAR_SPACING_RATIO);
            re.DrawImage(cl.pics[multiselect->mana_bar], &rect, &uv, MAKE(COLOR32,0,255,255,255));
        }
        if (++column >= multiselect->numcolumns) {
            column = 0;
            screen.x = SCR_LayoutRect(frame)->x;
            screen.y += multiselect->offset.y;
        } else {
            screen.x += multiselect->offset.x;
        }
    }
}

void SCR_DrawPortrait(LPCUIFRAME frame, LPCRECT screen) {
    RECT const viewport = {
        screen->x/0.8,1-(screen->y+screen->h)/0.6,screen->w/0.8,screen->h/0.6
    };
    LPCMODEL port = cl.portraits[frame->tex.index];
    re.DrawPortrait(port ? port : cl.models[frame->tex.index], &viewport);
}

void SCR_DrawCommandButton(LPCUIFRAME frame, LPCRECT screen) {
    LPCENTITYSTATE selentity = CL_SelectedEntity();
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    VECTOR2 m = {
        mouse.origin.x * 0.8 / WINDOW_WIDTH,
        mouse.origin.y * 0.6 / WINDOW_HEIGHT
    };
    RECT scrn = scale_rect(screen, 0.925);
    if (Rect_contains(screen, &m)) {
        if (mouse.button == 1 || mouse.event == UI_LEFT_MOUSE_UP) {
            scrn = scale_rect(screen, 0.875);
        }
    }
    re.DrawImageEx(&MAKE(DRAWIMAGE,
                         .texture = cl.pics[frame->tex.index],
                         .screen = scrn,
                         .uv = suv,
                         .color = COLOR32_WHITE,
                         .rotate = false,
                         .shader = SHADER_COMMANDBUTTON,
                         .uActiveGlow = selentity ? selentity->ability == frame->stat : 0));
}

void layout_text(LPCUIFRAME frame, LPCRECT screen, LPCSTR text) {
    DRAWTEXT drawtext = get_drawtext(frame, screen->w, text, frame->buffer.data);
    drawtext.rect = *screen;
    drawtext.wordWrap = true;
    re.DrawText(&drawtext);
}

void SCR_DrawString(LPCUIFRAME frame, LPCRECT screen) {
    uiLabel_t const *label = frame->buffer.data;
    RECT scr = *screen;
    scr.x += label->offsetx;
    scr.y += label->offsety;
    layout_text(frame, &scr, SCR_GetStringValue(frame));
}

void SCR_DrawTextArea(LPCUIFRAME frame, LPCRECT screen) {
    uiTextArea_t const *textArea = frame->buffer.data;
    RECT scr = {
        screen->x + textArea->inset,
        screen->y + textArea->inset,
        screen->w - textArea->inset * 2,
        screen->h - textArea->inset * 2,
    };
    re.DrawText(&MAKE(DRAWTEXT,
                      .font = cl.fonts[textArea->font],
                      .text = frame->text,
                      .color = frame->color,
                      .halign = FONT_JUSTIFYLEFT,
                      .valign = FONT_JUSTIFYTOP,
                      .icons = cl.pics,
                      .lineHeight = 1.33,
                      .textWidth = scr.w,
                      .rect = scr,
                      .wordWrap = true));
}

RECT Rect_inset(LPCRECT r, FLOAT inset) {
    return MAKE(RECT,r->x+inset,r->y+inset,r->w-inset*2,r->h-inset*2);
}

void SCR_DrawTooltip(LPCUIFRAME frame, LPCRECT scrn) {
    if (active_tooltip) {
        RECT screen = *scrn;
        uiTooltip_t const *tooltip = frame->buffer.data;
        FLOAT const PADDING = 0.005;
        FLOAT const avlspace = screen.w - PADDING * 2;
        DRAWTEXT drawtext = get_drawtext(frame, avlspace, active_tooltip, &tooltip->text);
        drawtext.wordWrap = true;
        VECTOR2 textsize = re.GetTextSize(&drawtext);
        textsize.y += PADDING * 2;
        screen.y += screen.h - textsize.y;
        screen.h = textsize.y;
        RECT text = Rect_inset(&screen, PADDING);
        SCR_DrawBackdrop(frame, &screen);
        drawtext = get_drawtext(frame, text.w, active_tooltip, &tooltip->text);
        drawtext.rect = text;
        drawtext.wordWrap = true;
        re.DrawText(&drawtext);
    }
}

LPCUIFRAME SCR_Clear(HANDLE data) {
    memset(runtimes, 0, sizeof(runtimes));
    memset(frames, 0, sizeof(frames));
    num_frames = 0;
    frames[0].size.width = 0.8;
    frames[0].size.height = 0.6;
    frames[0].flags.type = FT_SCREEN;
    sizeBuf_t msg = {
        .data = data,
        .cursize = 100000,
        .readcount = 0,
    };
    while (true) {
        DWORD bits = 0;
        DWORD nument = MSG_ReadEntityBits(&msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        LPUIFRAME ent = &frames[nument];
        ent->tex.coord[1] = 0xff;
        ent->tex.coord[3] = 0xff;
        MSG_ReadDeltaUIFrame(&msg, ent, nument, bits);
        ent->buffer.size = MSG_ReadByte(&msg);
        ent->buffer.data = msg.data + msg.readcount;
        msg.readcount += ent->buffer.size;
        num_frames = MAX(num_frames, nument+1);
    }
    return frames;
}

void SCR_UpdateCommandButton(LPCUIFRAME frame, LPCRECT screen) {
    VECTOR2 m = {
        mouse.origin.x * 0.8 / WINDOW_WIDTH,
        mouse.origin.y * 0.6 / WINDOW_HEIGHT
    };
    if (Rect_contains(screen, &m) && frame->tooltip) {
        active_tooltip = frame->tooltip;
    }
}

typedef struct {
    FRAMETYPE type;
    void (*func)(LPCUIFRAME, LPCRECT);
} drawer_t;

static drawer_t updaters[] = {
    { FT_COMMANDBUTTON, SCR_UpdateCommandButton },
    { FT_BUILDQUEUE, SCR_UpdateBuildQueue },
};

static drawer_t drawers[] = {
    { FT_TEXTURE, SCR_DrawTexture },
    { FT_HIGHLIGHT, SCR_DrawHighlight },
    { FT_BACKDROP, SCR_DrawBackdrop },
    { FT_SIMPLESTATUSBAR, SCR_DrawStatusbar },
    { FT_COMMANDBUTTON, SCR_DrawCommandButton },
    { FT_STRING, SCR_DrawString },
    { FT_TEXT, SCR_DrawString },
    { FT_TEXTAREA, SCR_DrawTextArea },
    { FT_TOOLTIPTEXT, SCR_DrawTooltip },
    { FT_PORTRAIT, SCR_DrawPortrait },
    { FT_BUILDQUEUE, SCR_DrawBuildQueue },
    { FT_MULTISELECT, SCR_DrawMultiSelect },
    { FT_SIMPLEBUTTON, SCR_SimpleButton },
    { FT_GLUETEXTBUTTON, SCR_GlueTextButton },
    { FT_GLUEBUTTON, SCR_GlueTextButton },
//    { FT_NONE, NULL },
};

void SCR_DrawFrame(LPCUIFRAME frame) {
    VECTOR2 m = {
        mouse.origin.x * 0.8 / WINDOW_WIDTH,
        mouse.origin.y * 0.6 / WINDOW_HEIGHT
    };
    RECT const *screen = SCR_LayoutRect(frame);
    FOR_LOOP(j, sizeof(drawers)/sizeof(*drawers)) {
        if (drawers[j].type == frame->flags.type) {
            drawers[j].func(frame, screen);
            break;
        }
    }
    if (Rect_contains(screen, &m) &&
        mouse.event == UI_LEFT_MOUSE_UP &&
        frame->onclick)
    {
        MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
        MSG_WriteString(&cls.netchan.message, frame->onclick);
    }
}

void SCR_UpdateFrame(LPCUIFRAME frame) {
    RECT const *screen = SCR_LayoutRect(frame);
    FOR_LOOP(j, sizeof(updaters)/sizeof(*updaters)) {
        if (updaters[j].type == frame->flags.type) {
            updaters[j].func(frame, screen);
            break;
        }
    }
}

void SCR_UpdateTooltip(HANDLE _frames) {
    FOR_LOOP(i, num_frames) {
        SCR_UpdateFrame(frames+i);
    }
}

void SCR_DrawOverlay(HANDLE _frames) {
    FOR_LOOP(i, num_frames) {
        SCR_DrawFrame(frames+i);
    }
}

void SCR_DrawOverlays(void) {
    active_tooltip = NULL;
    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        if ((1 << layer) & cl.playerstate.uiflags)
            continue;
        HANDLE *layout = cl.layout[layer];
        if (layout) {
            SCR_Clear(layout);
            SCR_UpdateTooltip(layout);
        }
    }
    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        if ((1 << layer) & cl.playerstate.uiflags)
            continue;
        HANDLE *layout = cl.layout[layer];
        if (layout) {
            SCR_Clear(layout);
            SCR_UpdateTooltip(layout);
            SCR_DrawOverlay(layout);
        }
    }
}

void SCR_UpdateScreen(void) {
    re.BeginFrame();
    
    V_RenderView();

    SCR_DrawOverlays();

    CON_DrawConsole();
    
//    if (cl.pics[42]) re.DrawPic(cl.pics[42], 0, 0);
    
    re.EndFrame();
}
