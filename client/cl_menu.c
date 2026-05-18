#include <stdlib.h>

#include "client.h"
#include <SDL2/SDL.h>

#define MAX_EDIT_STATES 32
#define MAX_EDIT_TEXT 256
#define MAX_LISTBOX_TEXT 2048

static LPCSTR active_tooltip = NULL;
static DWORD active_edit_number = 0;
static HANDLE active_layout = NULL;

static RECT Rect_inset(LPCRECT r, FLOAT inset);

typedef struct {
    DWORD number;
    BOOL inuse;
    DWORD cursor;
    DWORD maxChars;
    char text[MAX_EDIT_TEXT];
} editState_t;

static editState_t edit_states[MAX_EDIT_STATES];

static RECT get_uvrect(uint8_t const *texcoord) {
    RECT const uv = {
        texcoord[0],
        texcoord[2],
        texcoord[1] - texcoord[0],
        texcoord[3] - texcoord[2],
    };
    return uv;
}

static RECT scale_rect(LPCRECT rect, FLOAT factor) {
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

static LPCENTITYSTATE CL_SelectedEntity(void) {
    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        centity_t const *ce = &cl.ents[index];
        if (ce->current.renderfx & RF_SELECTED) {
            return &ce->current;
        }
    }
    return NULL;
}

static BOOL SCR_IsEditBoxType(FRAMETYPE type) {
    return type == FT_EDITBOX || type == FT_GLUEEDITBOX || type == FT_SLASHCHATBOX;
}

static editState_t *SCR_EditState(LPCUIFRAME frame, BOOL create) {
    editState_t *free_state = NULL;
    uiEditBox_t const *edit = frame->buffer.data;

    FOR_LOOP(i, MAX_EDIT_STATES) {
        editState_t *state = edit_states + i;
        if (state->inuse && state->number == frame->number) {
            state->maxChars = edit && edit->maxChars ? edit->maxChars : MAX_EDIT_TEXT - 1;
            return state;
        }
        if (!state->inuse && !free_state) {
            free_state = state;
        }
    }

    if (!create || !free_state) {
        return NULL;
    }

    memset(free_state, 0, sizeof(*free_state));
    free_state->inuse = true;
    free_state->number = frame->number;
    free_state->maxChars = edit && edit->maxChars ? edit->maxChars : MAX_EDIT_TEXT - 1;
    if (frame->text) {
        snprintf(free_state->text, sizeof(free_state->text), "%s", frame->text);
    }
    free_state->cursor = (DWORD)strlen(free_state->text);
    return free_state;
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

static void SCR_DrawHighlightData(uiHighlight_t const *highlight, LPCRECT screen) {
    if (!highlight || !highlight->alphaFile) {
        return;
    }
    re.DrawImageEx(&MAKE(DRAWIMAGE,
                         .texture = cl.pics[highlight->alphaFile],
                         .alphamode = highlight->alphaMode,
                         .screen = *screen,
                         .uv = MAKE(RECT,0,0,1,1),
                         .color = COLOR32_WHITE,
                         .rotate = false,
                         .shader = SHADER_UI));
}

void SCR_DrawHighlight(LPCUIFRAME frame, LPCRECT screen) {
    SCR_DrawHighlightData(frame->buffer.data, screen);
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
        BACKDROP_BOTTOM_EDGE,
        BACKDROP_TOP_EDGE,
        BACKDROP_BOTTOM_LEFT_CORNER,
        BACKDROP_BOTTOM_RIGHT_CORNER,
        BACKDROP_TOP_LEFT_CORNER,
        BACKDROP_TOP_RIGHT_CORNER,
    };
    RECT rects[BACKDROP_SIZE];
    backdrop_rects(screen, rects, backdrop->CornerSize);

#ifdef DIAG_OUTPUT
    (void)frame;
    (void)screen;
    (void)backdrop;
#endif

    size2_t backSize = re.GetTextureSize(cl.pics[backdrop->Background]);
    size2_t edgeSize = re.GetTextureSize(cl.pics[backdrop->EdgeFile]);

    RECT uv = { backdrop->Mirrored ? 1 : 0, 0, backdrop->Mirrored ? -1 : 1, 1};
    RECT background = *screen;
    background.x += backdrop->BackgroundInsets[BACKDROPINSET_LEFT];
    background.y += backdrop->BackgroundInsets[BACKDROPINSET_TOP];
    background.w -= backdrop->BackgroundInsets[BACKDROPINSET_LEFT];
    background.w -= backdrop->BackgroundInsets[BACKDROPINSET_RIGHT];
    background.h -= backdrop->BackgroundInsets[BACKDROPINSET_TOP];
    background.h -= backdrop->BackgroundInsets[BACKDROPINSET_BOTTOM];
    if (backdrop->TileBackground) {
        uv.w = background.w / (backSize.width / 1000.f);
        if (backdrop->Mirrored) {
            uv.x = uv.w;
            uv.w = -uv.w;
        }
        uv.h = background.h / (backSize.height / 1000.f);
    }
    re.DrawImageEx(&MAKE(DRAWIMAGE,
                         .texture = cl.pics[backdrop->Background],
                         .alphamode = BLEND_MODE_BLEND,
                         .screen = background,
                         .uv = uv,
                         .color = frame->color,
                         .rotate = false,
                         .shader = SHADER_UI));

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
                             .alphamode = BLEND_MODE_BLEND,
                             .screen = rects[corners[i]],
                             .uv = rect,
                             .color = frame->color,
                             .rotate = flip,
                             .shader = SHADER_UI));
    }
}

void SCR_DrawBackdrop(LPCUIFRAME frame, LPCRECT screen) {
    SCR_DrawBackdrop2(frame, screen, frame->buffer.data);
}

static BOOL SCR_BackdropHasArt(uiBackdrop_t const *backdrop) {
    return backdrop && (backdrop->Background || backdrop->EdgeFile);
}

static void SCR_DrawBackdropPart(LPCUIFRAME frame, LPCRECT screen, uiBackdrop_t const *backdrop) {
    if (SCR_BackdropHasArt(backdrop) && screen->w > 0 && screen->h > 0) {
        SCR_DrawBackdrop2(frame, screen, backdrop);
    }
}

void SCR_DrawScrollBar(LPCUIFRAME frame, LPCRECT screen) {
    uiScrollBar_t const *scrollbar = frame->buffer.data;
    FLOAT button_height;
    VECTOR2 const m = SCR_MouseToFdf();
    RECT inc;
    RECT dec;
    RECT track;
    RECT thumb;

    if (!scrollbar || screen->w <= 0 || screen->h <= 0) {
        return;
    }

    SCR_DrawBackdropPart(frame, screen, &scrollbar->background);

    button_height = MIN(screen->w, screen->h * 0.5f);
    inc = MAKE(RECT, screen->x, screen->y + screen->h - button_height, screen->w, button_height);
    dec = MAKE(RECT, screen->x, screen->y, screen->w, button_height);
    track = MAKE(RECT, screen->x, dec.y + dec.h, screen->w, inc.y - (dec.y + dec.h));
    SCR_DrawBackdropPart(frame, &inc, &scrollbar->incButton);
    SCR_DrawBackdropPart(frame, &dec, &scrollbar->decButton);

    if (frame->parent < SCR_NumFrames() && mouse.event == UI_LEFT_MOUSE_DOWN) {
        LPCUIFRAME listFrame = SCR_Frame(frame->parent);

        if (listFrame && listFrame->flags.type == FT_LISTBOX && listFrame->buffer.data) {
            uiListBox_t const *listbox = listFrame->buffer.data;
            RECT list_rect = Rect_inset(SCR_LayoutRect(listFrame), listbox->border);
            FLOAT item_height = listbox->itemHeight > 0 ? listbox->itemHeight : 0.018f;
            DWORD visibleRows = MAX((DWORD)floorf(list_rect.h / item_height), 1);

            if (Rect_contains(&inc, &m)) {
                CL_ListBoxScroll(active_layout, listFrame, listbox, -1, visibleRows);
            } else if (Rect_contains(&dec, &m)) {
                CL_ListBoxScroll(active_layout, listFrame, listbox, 1, visibleRows);
            }
        }
    }

    if (track.h <= 0) {
        return;
    }

#ifdef UI_STRETCHED_SCROLLBAR_THUMB
    thumb.h = MIN(MAX(button_height, track.h * 0.25f), track.h);
#else
    thumb.h = MIN(MIN(button_height, 0.010f), track.h);
#endif
    thumb.w = MIN(screen->w, 0.010f);
    thumb.x = screen->x + (screen->w - thumb.w) * 0.5f;
    thumb.y = track.y + track.h - thumb.h - (track.h - thumb.h) * MIN(MAX(frame->value, 0.0f), 1.0f);
    SCR_DrawBackdropPart(frame, &thumb, &scrollbar->thumbButton);
}

static BOOL SCR_GlueTextButtonIsPushed(LPCUIFRAME frame, LPCRECT screen) {
    VECTOR2 const m = SCR_MouseToFdf();
    return Rect_contains(screen, &m) && mouse.button == 1;
}

void SCR_GlueTextButton(LPCUIFRAME frame, LPCRECT screen) {
    uiGlueTextButton_t const *gluetextbutton = frame->buffer.data;
    BOOL const enabled = frame->onclick && *frame->onclick;
    uiBackdrop_t const *backdrop = &gluetextbutton->normal;
    if (!enabled) {
        backdrop = SCR_GlueTextButtonIsPushed(frame, screen) ? &gluetextbutton->disabledPushed : &gluetextbutton->disabled;
    } else if (SCR_GlueTextButtonIsPushed(frame, screen)) {
        backdrop = &gluetextbutton->pushed;
    }

    SCR_DrawBackdrop2(frame, screen, backdrop);
}

static void SCR_DrawGlueTextButtonHighlight(LPCUIFRAME frame) {
    uiGlueTextButton_t const *gluetextbutton = frame->buffer.data;
    RECT const *screen = SCR_LayoutRect(frame);
    VECTOR2 const m = SCR_MouseToFdf();
    BOOL const enabled = frame->onclick && *frame->onclick;
    BOOL const mouse_over = Rect_contains(screen, &m);

    if (enabled && mouse_over) {
        SCR_DrawHighlightData(&gluetextbutton->highlight, screen);
    }
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
            LPUIFRAME buildtimer = SCR_Frame(queue->buildtimer);
            LPUIFRAME firstitem = SCR_Frame(queue->firstitem);
            if (buildtimer) {
                buildtimer->value = BYTE2FLOAT(ent->stats[ENT_HEALTH]);
            }
            if (firstitem) {
                firstitem->tex.index = queue->items[i].image;
            }
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
        screen->x / UI_BASE_WIDTH,
        screen->y / UI_BASE_HEIGHT,
        screen->w / UI_BASE_WIDTH,
        screen->h / UI_BASE_HEIGHT
    };
    LPCMODEL port = cl.portraits[frame->tex.index];
    re.DrawPortrait(port ? port : cl.models[frame->tex.index], &viewport, "Stand");
}

void SCR_DrawSprite(LPCUIFRAME frame, LPCRECT screen) {
    LPCSTR anim = frame->text;

    if (anim && *anim) {
        re.DrawSprite(cl.models[frame->tex.index], anim, screen->x, screen->y);
    } else {
        re.DrawSprite(cl.models[frame->tex.index], "Stand", screen->x, screen->y);
    }
}

void SCR_DrawCommandButton(LPCUIFRAME frame, LPCRECT screen) {
    LPCENTITYSTATE selentity = CL_SelectedEntity();
    RECT const uv = get_uvrect(frame->tex.coord);
    RECT const suv = Rect_div(&uv, 0xff);
    VECTOR2 const m = SCR_MouseToFdf();
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
    DRAWTEXT drawtext = SCR_GetDrawText(frame, screen->w, text, frame->buffer.data);
    drawtext.rect = *screen;
    drawtext.wordWrap = true;
    re.DrawText(&drawtext);
}

static void SCR_ApplyPushedTextOffset(LPCUIFRAME frame, LPRECT screen) {
    if (frame->parent >= SCR_NumFrames()) {
        return;
    }

    LPCUIFRAME parent = SCR_Frame(frame->parent);
    if (!parent) {
        return;
    }
    if (parent->flags.type != FT_GLUETEXTBUTTON && parent->flags.type != FT_GLUEBUTTON) {
        return;
    }

    LPCRECT parent_screen = SCR_LayoutRect(parent);
    if (!SCR_GlueTextButtonIsPushed(parent, parent_screen)) {
        return;
    }

    uiGlueTextButton_t const *button = parent->buffer.data;
    screen->x += button->pushedTextOffset.x;
    screen->y += button->pushedTextOffset.y;
}

void SCR_DrawString(LPCUIFRAME frame, LPCRECT screen) {
    uiLabel_t const *label = frame->buffer.data;
    RECT scr = *screen;
    scr.x += label->offsetx;
    scr.y += label->offsety;
    SCR_ApplyPushedTextOffset(frame, &scr);
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

void SCR_DrawEditBox(LPCUIFRAME frame, LPCRECT screen) {
    uiEditBox_t const *edit = frame->buffer.data;
    editState_t *state = SCR_EditState(frame, true);
    RECT text_rect = Rect_inset(screen, edit->borderSize);
    char cursor_text[MAX_EDIT_TEXT + 2];
    LPCSTR text = state ? state->text : "";

    SCR_DrawBackdrop2(frame, screen, &edit->background);

    re.DrawText(&MAKE(DRAWTEXT,
                      .font = cl.fonts[edit->font],
                      .text = text,
                      .color = edit->textColor,
                      .halign = FONT_JUSTIFYLEFT,
                      .valign = FONT_JUSTIFYMIDDLE,
                      .icons = cl.pics,
                      .lineHeight = 1.33,
                      .textWidth = text_rect.w,
                      .rect = text_rect,
                      .wordWrap = false));

    if (active_edit_number == frame->number && state && (cl.time % 500) < 250) {
        DWORD cursor = MIN(state->cursor, (DWORD)strlen(state->text));
        DRAWTEXT measure;
        VECTOR2 prefix_size;
        RECT cursor_rect = text_rect;

        snprintf(cursor_text, sizeof(cursor_text), "%.*s", (int)cursor, state->text);
        measure = MAKE(DRAWTEXT,
                       .font = cl.fonts[edit->font],
                       .text = cursor_text,
                       .color = edit->textColor,
                       .halign = FONT_JUSTIFYLEFT,
                       .valign = FONT_JUSTIFYMIDDLE,
                       .icons = cl.pics,
                       .lineHeight = 1.33,
                       .textWidth = text_rect.w,
                       .rect = text_rect,
                       .wordWrap = false);
        prefix_size = re.GetTextSize(&measure);
        cursor_rect.x += prefix_size.x;
        cursor_rect.w = MAX(cursor_rect.w - prefix_size.x, 0.0f);
        re.DrawText(&MAKE(DRAWTEXT,
                          .font = cl.fonts[edit->font],
                          .text = "|",
                          .color = edit->cursorColor,
                          .halign = FONT_JUSTIFYLEFT,
                          .valign = FONT_JUSTIFYMIDDLE,
                          .icons = cl.pics,
                          .lineHeight = 1.33,
                          .textWidth = cursor_rect.w,
                          .rect = cursor_rect,
                          .wordWrap = false));
    }
}

void SCR_DrawListBox(LPCUIFRAME frame, LPCRECT screen) {
    uiListBox_t const *listbox = frame->buffer.data;
    RECT list_rect = Rect_inset(screen, listbox->border);
    LPCUIFRAME scrollbar = NULL;
    FLOAT item_y = list_rect.y + list_rect.h;
    FLOAT item_height = listbox->itemHeight > 0 ? listbox->itemHeight : 0.018f;
    LPCSTR text = frame->text;
    BOOL loading = false;
    SHORT selectedIndex = listbox->selectedIndex;
    DWORD scrollOffset = 0;
    DWORD numRows = 0;
    DWORD visibleRows;
    char items[MAX_LISTBOX_TEXT];
    char *line = NULL;
    char *save = NULL;
    int index = 0;

    SCR_DrawBackdrop2(frame, screen, &listbox->background);
    CL_ListBoxApplyFetch(active_layout, frame, listbox, &text, &loading, &selectedIndex, &scrollOffset, &numRows);

    FOR_LOOP(i, SCR_NumFrames()) {
        LPCUIFRAME child = SCR_Frame(i);
        if (child && child->parent == frame->number && child->flags.type == FT_SCROLLBAR) {
            scrollbar = child;
            break;
        }
    }
    if (scrollbar) {
        LPCRECT scroll_rect = SCR_LayoutRect(scrollbar);
        FLOAT scroll_inset = MAX(scroll_rect->w, 0.0f);

        if (scroll_inset > 0 && scroll_inset < list_rect.w) {
            list_rect.w -= scroll_inset;
        }
    }
    visibleRows = MAX((DWORD)floorf(list_rect.h / item_height), 1);
    VECTOR2 const mouse_pos = SCR_MouseToFdf();
    if (mouse.wheel && Rect_contains(screen, &mouse_pos)) {
        CL_ListBoxScroll(active_layout, frame, listbox, -mouse.wheel, visibleRows);
        CL_ListBoxApplyFetch(active_layout, frame, listbox, &text, &loading, &selectedIndex, &scrollOffset, &numRows);
    }
    if (scrollbar) {
        DWORD maxScroll = numRows > visibleRows ? numRows - visibleRows : 0;

        ((LPUIFRAME)scrollbar)->value = maxScroll ? scrollOffset / (FLOAT)maxScroll : 0.0f;
    }

    if (loading) {
        re.DrawLoadingIndicator(&list_rect, cl.time, frame->color);
        return;
    }

    if (!text || !*text) {
        return;
    }

    snprintf(items, sizeof(items), "%s", text);
    line = strtok_r(items, "\n", &save);
    while (line && index < (int)scrollOffset) {
        line = strtok_r(NULL, "\n", &save);
        index++;
    }
    while (line && item_y > list_rect.y) {
        RECT row = list_rect;
        char *display = line;
        char *hidden = strchr(display, '\t');
        int rowIndex = index;

        if (hidden) {
            *hidden = '\0';
        }
        row.h = MIN(item_height, item_y - list_rect.y);
        row.y = item_y - row.h;
        if (rowIndex == selectedIndex) {
            re.DrawImage(cl.pics[0], &row, &MAKE(RECT, 0, 0, 1, 1), MAKE(COLOR32, 32, 64, 180, 128));
        }
        if (mouse.event == UI_LEFT_MOUSE_UP && Rect_contains(&row, &mouse_pos)) {
            CL_ListBoxSelect(active_layout, frame, listbox, (DWORD)rowIndex);
            selectedIndex = (SHORT)rowIndex;
        }
        re.DrawText(&MAKE(DRAWTEXT,
                          .font = cl.fonts[listbox->text.font],
                          .text = display,
                          .color = frame->color.a ? frame->color : COLOR32_WHITE,
                          .halign = FONT_JUSTIFYLEFT,
                          .valign = FONT_JUSTIFYMIDDLE,
                          .icons = cl.pics,
                          .lineHeight = 1.33,
                          .textWidth = row.w,
                          .rect = row,
                          .wordWrap = false));
        item_y -= item_height;
        line = strtok_r(NULL, "\n", &save);
        index++;
    }
}

static RECT Rect_inset(LPCRECT r, FLOAT inset) {
    return MAKE(RECT,r->x+inset,r->y+inset,r->w-inset*2,r->h-inset*2);
}

void SCR_DrawTooltip(LPCUIFRAME frame, LPCRECT scrn) {
    if (active_tooltip) {
        RECT screen = *scrn;
        uiTooltip_t const *tooltip = frame->buffer.data;
        FLOAT const PADDING = 0.005;
        FLOAT const avlspace = screen.w - PADDING * 2;
        DRAWTEXT drawtext = SCR_GetDrawText(frame, avlspace, active_tooltip, &tooltip->text);
        drawtext.wordWrap = true;
        VECTOR2 textsize = re.GetTextSize(&drawtext);
        textsize.y += PADDING * 2;
        screen.y += screen.h - textsize.y;
        screen.h = textsize.y;
        RECT text = Rect_inset(&screen, PADDING);
        SCR_DrawBackdrop(frame, &screen);
        drawtext = SCR_GetDrawText(frame, text.w, active_tooltip, &tooltip->text);
        drawtext.rect = text;
        drawtext.wordWrap = true;
        re.DrawText(&drawtext);
    }
}

void SCR_UpdateCommandButton(LPCUIFRAME frame, LPCRECT screen) {
    VECTOR2 const m = SCR_MouseToFdf();
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
    { FT_EDITBOX, SCR_DrawEditBox },
    { FT_GLUEEDITBOX, SCR_DrawEditBox },
    { FT_SLASHCHATBOX, SCR_DrawEditBox },
    { FT_LISTBOX, SCR_DrawListBox },
    { FT_SCROLLBAR, SCR_DrawScrollBar },
    { FT_TOOLTIPTEXT, SCR_DrawTooltip },
    { FT_MODEL, SCR_DrawPortrait },
    { FT_SPRITE, SCR_DrawSprite },
    { FT_PORTRAIT, SCR_DrawPortrait },
    { FT_BUILDQUEUE, SCR_DrawBuildQueue },
    { FT_MULTISELECT, SCR_DrawMultiSelect },
    { FT_SIMPLEBUTTON, SCR_SimpleButton },
    { FT_GLUETEXTBUTTON, SCR_GlueTextButton },
    { FT_GLUEBUTTON, SCR_GlueTextButton },
//    { FT_NONE, NULL },
};

void SCR_DrawFrame(LPCUIFRAME frame) {
    VECTOR2 const m = SCR_MouseToFdf();
    RECT const *screen = SCR_LayoutRect(frame);
    if (SCR_IsEditBoxType(frame->flags.type) &&
        Rect_contains(screen, &m) &&
        mouse.event == UI_LEFT_MOUSE_DOWN)
    {
        active_edit_number = frame->number;
        SCR_EditState(frame, true);
    }
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
    FOR_LOOP(i, SCR_NumFrames()) {
        LPCUIFRAME frame = SCR_Frame(i);
        if (frame) {
            SCR_UpdateFrame(frame);
        }
    }
}

void SCR_DrawOverlay(HANDLE _frames) {
    if (mouse.event == UI_LEFT_MOUSE_DOWN) {
        active_edit_number = 0;
    }
    FOR_LOOP(i, SCR_NumFrames()) {
        LPCUIFRAME frame = SCR_Frame(i);
        if (frame && frame->flags.type == FT_SPRITE) {
            SCR_DrawFrame(frame);
        }
    }
    FOR_LOOP(i, SCR_NumFrames()) {
        LPCUIFRAME frame = SCR_Frame(i);
        if (frame && frame->flags.type != FT_SPRITE) {
            SCR_DrawFrame(frame);
        }
    }
    FOR_LOOP(i, SCR_NumFrames()) {
        LPCUIFRAME frame = SCR_Frame(i);
        if (frame && (frame->flags.type == FT_GLUETEXTBUTTON || frame->flags.type == FT_GLUEBUTTON)) {
            SCR_DrawGlueTextButtonHighlight(frame);
        }
    }
}

void SCR_DrawOverlays(void) {
    active_tooltip = NULL;
    
    if (cl.playerstate.cinefade > 0) {
        COLOR32 color = COLOR32_BLACK;
        color.a = 255 * cl.playerstate.cinefade;
        re.DrawImage(cl.pics[0], &MAKE(RECT,0,0,1,1), &MAKE(RECT,0,0,1,1), color);
    }
    
    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        if ((1 << layer) & cl.playerstate.uiflags)
            continue;
        HANDLE *layout = cl.layout[layer];
        if (layout) {
            active_layout = layout;
            SCR_Clear(layout);
            SCR_UpdateTooltip(layout);
        }
    }
    
    FOR_LOOP(layer, MAX_LAYOUT_LAYERS) {
        if ((1 << layer) & cl.playerstate.uiflags)
            continue;
        HANDLE *layout = cl.layout[layer];
        if (layout) {
            active_layout = layout;
            SCR_Clear(layout);
            SCR_UpdateTooltip(layout);
            SCR_DrawOverlay(layout);
        }
    }
}

static editState_t *SCR_ActiveEditState(void) {
    FOR_LOOP(i, MAX_EDIT_STATES) {
        if (edit_states[i].inuse && edit_states[i].number == active_edit_number) {
            return edit_states + i;
        }
    }
    return NULL;
}

void SCR_TextInput(LPCSTR text) {
    editState_t *state = SCR_ActiveEditState();
    size_t len;
    size_t add_len;
    DWORD cursor;

    if (!state || !text || !*text) {
        return;
    }

    len = strlen(state->text);
    add_len = strlen(text);
    cursor = MIN(state->cursor, (DWORD)len);
    if (state->maxChars > 0 && len >= state->maxChars) {
        return;
    }
    if (state->maxChars > 0 && len + add_len > state->maxChars) {
        add_len = state->maxChars - len;
    }
    if (len + add_len >= sizeof(state->text)) {
        add_len = sizeof(state->text) - len - 1;
    }
    if (add_len == 0) {
        return;
    }

    memmove(state->text + cursor + add_len,
            state->text + cursor,
            len - cursor + 1);
    memcpy(state->text + cursor, text, add_len);
    state->cursor = cursor + (DWORD)add_len;
}

BOOL SCR_EditKey(int key) {
    editState_t *state = SCR_ActiveEditState();
    size_t len;
    DWORD cursor;

    if (!state) {
        return false;
    }

    len = strlen(state->text);
    cursor = MIN(state->cursor, (DWORD)len);
    switch (key) {
        case SDLK_BACKSPACE:
            if (cursor > 0) {
                memmove(state->text + cursor - 1,
                        state->text + cursor,
                        len - cursor + 1);
                state->cursor = cursor - 1;
            }
            return true;
        case SDLK_DELETE:
            if (cursor < len) {
                memmove(state->text + cursor,
                        state->text + cursor + 1,
                        len - cursor);
            }
            return true;
        case SDLK_LEFT:
            if (cursor > 0) {
                state->cursor = cursor - 1;
            }
            return true;
        case SDLK_RIGHT:
            if (cursor < len) {
                state->cursor = cursor + 1;
            }
            return true;
        case SDLK_HOME:
            state->cursor = 0;
            return true;
        case SDLK_END:
            state->cursor = (DWORD)len;
            return true;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            active_edit_number = 0;
            return true;
        default:
            break;
    }
    return false;
}
