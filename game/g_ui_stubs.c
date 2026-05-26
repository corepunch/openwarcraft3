/*
 * g_ui_stubs.c — Server-authored in-game HUD proxy frames.
 *
 * Glue/FDF menus are client-side, but the gameplay HUD is data-heavy and
 * depends on server-owned unit state.  These helpers generate compact
 * uiFrame_t trees and send them through svc_layout, reusing the old client
 * layout renderer in cl_scrn/cl_menu.
 */

#include "g_local.h"

#define HUD_FONT_SIZE 10
#define UI_BASE_WIDTH 0.8f
#define UI_BASE_HEIGHT 0.6f

#define INFO_PANEL_X 0.310f
#define INFO_PANEL_Y 0.486f
#define INFO_PANEL_W 0.180f
#define INFO_PANEL_H 0.105f
#define BUILDQUEUE_OFFSET 0.0281f
#define BUILDQUEUE_BACKDROP_X INFO_PANEL_X
#define BUILDQUEUE_BACKDROP_Y (INFO_PANEL_Y + INFO_PANEL_H - 0.1000f)
#define BUILDQUEUE_BACKDROP_W INFO_PANEL_W
#define BUILDQUEUE_BACKDROP_H 0.1000f
#define BUILDQUEUE_FIRST_X (INFO_PANEL_X + 0.0100f)
#define BUILDQUEUE_FIRST_Y (INFO_PANEL_Y + 0.0390f)
#define BUILDQUEUE_FIRST_W 0.0280f
#define BUILDQUEUE_FIRST_H 0.0310f
#define BUILDQUEUE_LIST_X (INFO_PANEL_X + 0.0095f)
#define BUILDQUEUE_LIST_Y (INFO_PANEL_Y + 0.0800f)
#define BUILDQUEUE_ITEM_W 0.0200f
#define BUILDQUEUE_ITEM_H 0.0215f
#define BUILDQUEUE_TIMER_X (INFO_PANEL_X + 0.061250f)
#define BUILDQUEUE_TIMER_Y (INFO_PANEL_Y + 0.038125f)
#define BUILDQUEUE_TIMER_W 0.1450f
#define BUILDQUEUE_TIMER_H 0.0120f
#define BUILDQUEUE_ACTION_X (INFO_PANEL_X + 0.061250f)
#define BUILDQUEUE_ACTION_Y (INFO_PANEL_Y + 0.022875f)
#define BUILDQUEUE_ACTION_W 0.1050f
#define BUILDQUEUE_ACTION_H 0.0140f
#define PORTRAIT_X 0.215f
#define PORTRAIT_Y 0.490f
#define PORTRAIT_SIZE 0.080f
#define COMMAND_BUTTON_SIZE 0.039f
#define COMMAND_BUTTON_CENTER_X(x) (UI_BASE_WIDTH * 0.5f + 0.2365f + (FLOAT)(x) * 0.0434f)
#define COMMAND_BUTTON_CENTER_Y(y) (UI_BASE_HEIGHT - (0.1131f - (FLOAT)(y) * 0.0434f))
#define INVENTORY_BUTTON_SIZE 0.033f
#define INVENTORY_BUTTON_CENTER_X(x) (UI_BASE_WIDTH * 0.5f + 0.1315f + (FLOAT)(x) * 0.0394f)
#define INVENTORY_BUTTON_CENTER_Y(y) (UI_BASE_HEIGHT - (0.0971f - (FLOAT)(y) * 0.0384f))
#define MULTISELECT_X 0.314f
#define MULTISELECT_Y 0.500f
#define MULTISELECT_SIZE 0.025f
static DWORD ui_next_frame_number;
static LPGAMECLIENT ui_current_client;

void UI_SetCurrentClient(LPGAMECLIENT client) {
    ui_current_client = client;
}

static void UI_SetFramePoint(uiFramePoint_t *point, uiFramePointPos_t target, DWORD relative, FLOAT offset, BOOL y_axis) {
    point->used = 1;
    point->targetPos = target;
    point->relativeTo = (BYTE)relative;
    point->offset = (SHORT)((y_axis ? -offset : offset) * UI_FRAMEPOINT_SCALE);
}

static void UI_SetFrameRect(LPUIFRAME frame, FLOAT x, FLOAT y, FLOAT w, FLOAT h) {
    UI_SetFramePoint(&frame->points.x[FPP_MIN], FPP_MIN, 0, x, false);
    UI_SetFramePoint(&frame->points.y[FPP_MIN], FPP_MIN, 0, y, true);
    frame->size.width = w;
    frame->size.height = h;
}

static void UI_WriteProxyFrame(LPUIFRAME frame, HANDLE data, DWORD data_size) {
    frame->number = ui_next_frame_number++;
    frame->parent = 0;
    frame->color = frame->color.a ? frame->color : COLOR32_WHITE;
    frame->tex.coord[1] = 0xff;
    frame->tex.coord[3] = 0xff;
    frame->buffer.data = data;
    frame->buffer.size = data_size;
    gi.WriteUIFrame(frame);
}

static void UI_WriteTextFrame(FLOAT x, FLOAT y, FLOAT w, FLOAT h, LPCSTR text, COLOR32 color,
                              uiFontJustificationH_t align) {
    uiFrame_t frame;
    uiLabel_t label;

    memset(&frame, 0, sizeof(frame));
    memset(&label, 0, sizeof(label));
    frame.flags.type = FT_STRING;
    frame.text = text;
    frame.color = color;
    label.font = gi.FontIndex("Fonts\\FRIZQT__.TTF", HUD_FONT_SIZE);
    label.textalignx = align;
    label.textaligny = FONT_JUSTIFYTOP;
    UI_SetFrameRect(&frame, x, y, w, h);
    UI_WriteProxyFrame(&frame, &label, sizeof(label));
}

static void UI_WriteServerConsoleShell(LPEDICT ent) {
    if (!ent) {
        return;
    }

    gi.WriteByte(svc_layout);
    gi.WriteByte(LAYER_CONSOLE);
    ui_next_frame_number = 1;

    UI_WriteTextFrame(0.004f, 0.006f, 0.085f, 0.014f, "Quests (F9)", COLOR32_WHITE, FONT_JUSTIFYCENTER);
    UI_WriteTextFrame(0.089f, 0.006f, 0.085f, 0.014f, "Menu (F10)", COLOR32_WHITE, FONT_JUSTIFYCENTER);
    UI_WriteTextFrame(0.174f, 0.006f, 0.085f, 0.014f, "Allies (F11)", COLOR32_WHITE, FONT_JUSTIFYCENTER);
    UI_WriteTextFrame(0.259f, 0.006f, 0.085f, 0.014f, "Chat (F12)", COLOR32_WHITE, FONT_JUSTIFYCENTER);

    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);
}

static void UI_WritePortraitFrame(LPEDICT ent) {
    uiFrame_t frame;

    if (!ent || !ent->s.model) {
        return;
    }
    memset(&frame, 0, sizeof(frame));
    frame.flags.type = FT_PORTRAIT;
    frame.color = COLOR32_WHITE;
    frame.tex.index = ent->s.model;
    UI_SetFrameRect(&frame, PORTRAIT_X, PORTRAIT_Y, PORTRAIT_SIZE, PORTRAIT_SIZE);
    UI_WriteProxyFrame(&frame, NULL, 0);
}

static RECT UI_CommandButtonRect(BYTE x, BYTE y) {
    return MAKE(RECT,
                COMMAND_BUTTON_CENTER_X(x) - COMMAND_BUTTON_SIZE * 0.5f,
                COMMAND_BUTTON_CENTER_Y(y) - COMMAND_BUTTON_SIZE * 0.5f,
                COMMAND_BUTTON_SIZE,
                COMMAND_BUTTON_SIZE);
}

static RECT UI_InventoryButtonRect(BYTE slot) {
    return MAKE(RECT,
                INVENTORY_BUTTON_CENTER_X(slot % 2) - INVENTORY_BUTTON_SIZE * 0.5f,
                INVENTORY_BUTTON_CENTER_Y(slot / 2) - INVENTORY_BUTTON_SIZE * 0.5f,
                INVENTORY_BUTTON_SIZE,
                INVENTORY_BUTTON_SIZE);
}

static void UI_WriteCommandButton(LPCSTR code, BOOL research, DWORD level) {
    gameCommandButton_t buttons[1];
    uiFrame_t frame;
    RECT rect;
    char onclick[128];
    LPCSTR command = code;
    LPEDICT ent = G_GetMainSelectedUnit(ui_current_client);

    if (!ent || !code || !*code) {
        return;
    }
    if (!G_BuildCommandButton(ent, code, research, level, buttons)) {
        return;
    }

    rect = UI_CommandButtonRect(buttons[0].x, buttons[0].y);
    memset(&frame, 0, sizeof(frame));
    frame.flags.type = FT_COMMANDBUTTON;
    frame.color = COLOR32_WHITE;
    frame.tex.index = gi.ImageIndex(buttons[0].art);
    frame.stat = buttons[0].active;
    frame.tooltip = buttons[0].ubertip[0] ? buttons[0].ubertip : buttons[0].tooltip;
    snprintf(onclick, sizeof(onclick), "%s %s", research ? "research" : "button", command);
    frame.onclick = onclick;
    UI_SetFrameRect(&frame, rect.x, rect.y, rect.w, rect.h);
    UI_WriteProxyFrame(&frame, NULL, 0);
}

void UI_ClearLayer(LPEDICT ent, DWORD layer) {
    if (!ent) {
        return;
    }
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);
}

static DWORD UI_SelectedUnits(LPGAMECLIENT client, LPEDICT *out, DWORD max_out) {
    DWORD count = 0;

    FOR_SELECTED_UNITS(client, ent) {
        if (count < max_out) {
            out[count] = ent;
        }
        count++;
    }
    return MIN(count, max_out);
}

static void UI_WriteSingleInfo(LPEDICT ent) {
    char buffer[128];
    LPCSTR name = UNIT_PROPER_NAMES(ent->class_id);
    LPCSTR unit_name = UNIT_NAME(ent->class_id);
    DWORD level = MAX(1, UNIT_LEVEL(ent->class_id));
    LONG dice = UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(ent->class_id);
    LONG min_damage = dice ? UNIT_ATTACK1_DAMAGE_BASE(ent->class_id) + dice : 0;
    LONG max_damage = dice ? UNIT_ATTACK1_DAMAGE_BASE(ent->class_id) + dice * UNIT_ATTACK1_DAMAGE_SIDES_PER_DIE(ent->class_id) : 0;

    if (!name || !*name) {
        name = unit_name ? unit_name : GetClassName(ent->class_id);
    }

    UI_WriteTextFrame(INFO_PANEL_X, INFO_PANEL_Y, INFO_PANEL_W, 0.018f, name, COLOR32_WHITE, FONT_JUSTIFYCENTER);
    snprintf(buffer, sizeof(buffer), "Level %lu %s", (unsigned long)level, unit_name ? unit_name : "");
    UI_WriteTextFrame(INFO_PANEL_X, INFO_PANEL_Y + 0.020f, INFO_PANEL_W, 0.014f, buffer,
                      MAKE(COLOR32, 252, 210, 18, 255), FONT_JUSTIFYCENTER);

    UI_WriteTextFrame(INFO_PANEL_X + 0.010f, INFO_PANEL_Y + 0.042f, 0.055f, 0.012f, "Damage:",
                      MAKE(COLOR32, 252, 210, 18, 255), FONT_JUSTIFYLEFT);
    snprintf(buffer, sizeof(buffer), "%d - %d", (int)min_damage, (int)max_damage);
    UI_WriteTextFrame(INFO_PANEL_X + 0.062f, INFO_PANEL_Y + 0.042f, 0.050f, 0.012f, buffer,
                      COLOR32_WHITE, FONT_JUSTIFYLEFT);

    UI_WriteTextFrame(INFO_PANEL_X + 0.010f, INFO_PANEL_Y + 0.066f, 0.055f, 0.012f, "Armor:",
                      MAKE(COLOR32, 252, 210, 18, 255), FONT_JUSTIFYLEFT);
    snprintf(buffer, sizeof(buffer), "%d", (int)UNIT_DEFENSE(ent->class_id));
    UI_WriteTextFrame(INFO_PANEL_X + 0.062f, INFO_PANEL_Y + 0.066f, 0.050f, 0.012f, buffer,
                      COLOR32_WHITE, FONT_JUSTIFYLEFT);
}

static void UI_WriteInventory(LPEDICT ent) {
    gameInventoryItem_t items[MAX_INVENTORY];
    BYTE count = G_GetInventory(ent, items, MAX_INVENTORY);

    FOR_LOOP(i, count) {
        RECT rect = UI_InventoryButtonRect(items[i].slot);
        uiFrame_t frame;
        char onclick[128];

        memset(&frame, 0, sizeof(frame));
        frame.flags.type = FT_COMMANDBUTTON;
        frame.color = COLOR32_WHITE;
        frame.tex.index = gi.ImageIndex(items[i].art);
        frame.tooltip = items[i].ubertip[0] ? items[i].ubertip : items[i].tooltip;
        snprintf(onclick, sizeof(onclick), "inventory %u", (unsigned)items[i].slot);
        frame.onclick = onclick;
        UI_SetFrameRect(&frame, rect.x, rect.y, rect.w, rect.h);
        UI_WriteProxyFrame(&frame, NULL, 0);
    }
}

static void UI_WriteBuildQueue(LPEDICT ent) {
    gameQueueItem_t queue[MAX_BUILD_QUEUE];
    BYTE count = G_GetBuildQueue(ent, queue, MAX_BUILD_QUEUE);
    DWORD size;
    LPBYTE buffer;
    uiBuildQueue_t *buildqueue;
    uiFrame_t backdrop;
    uiFrame_t firstitem;
    uiFrame_t buildtimer;
    uiFrame_t list;

    if (!count) {
        return;
    }

    UI_WriteTextFrame(INFO_PANEL_X,
                      INFO_PANEL_Y,
                      INFO_PANEL_W,
                      0.018f,
                      UNIT_NAME(ent->class_id) ? UNIT_NAME(ent->class_id) : GetClassName(ent->class_id),
                      COLOR32_WHITE,
                      FONT_JUSTIFYCENTER);
    UI_WriteTextFrame(BUILDQUEUE_ACTION_X,
                      BUILDQUEUE_ACTION_Y,
                      BUILDQUEUE_ACTION_W,
                      BUILDQUEUE_ACTION_H,
                      ent->currentmove && ent->currentmove->think == ai_birth ? "Constructing" : "Training",
                      MAKE(COLOR32, 252, 210, 18, 255),
                      FONT_JUSTIFYCENTER);

    if (!ent->currentmove || ent->currentmove->think != ai_birth) {
        memset(&backdrop, 0, sizeof(backdrop));
        backdrop.flags.type = FT_TEXTURE;
        backdrop.color = COLOR32_WHITE;
        backdrop.tex.index = gi.ImageIndex("BuildQueueBackdrop");
        UI_SetFrameRect(&backdrop,
                        BUILDQUEUE_BACKDROP_X,
                        BUILDQUEUE_BACKDROP_Y,
                        BUILDQUEUE_BACKDROP_W,
                        BUILDQUEUE_BACKDROP_H);
        UI_WriteProxyFrame(&backdrop, NULL, 0);
    }

    memset(&firstitem, 0, sizeof(firstitem));
    firstitem.flags.type = FT_TEXTURE;
    firstitem.color = COLOR32_WHITE;
    firstitem.tex.index = gi.ImageIndex(queue[0].art);
    UI_SetFrameRect(&firstitem, BUILDQUEUE_FIRST_X, BUILDQUEUE_FIRST_Y, BUILDQUEUE_FIRST_W, BUILDQUEUE_FIRST_H);
    UI_WriteProxyFrame(&firstitem, NULL, 0);

    memset(&buildtimer, 0, sizeof(buildtimer));
    buildtimer.flags.type = FT_SIMPLESTATUSBAR;
    buildtimer.color = MAKE(COLOR32, 160, 0, 160, 255);
    buildtimer.tex.index = gi.ImageIndex("SimpleBuildTimeIndicator");
    buildtimer.tex.index2 = gi.ImageIndex("SimpleBuildTimeIndicatorBorder");
    UI_SetFrameRect(&buildtimer, BUILDQUEUE_TIMER_X, BUILDQUEUE_TIMER_Y, BUILDQUEUE_TIMER_W, BUILDQUEUE_TIMER_H);
    UI_WriteProxyFrame(&buildtimer, NULL, 0);

    size = sizeof(uiBuildQueue_t) + sizeof(uiBuildQueueItem_t) * count;
    buffer = gi.MemAlloc(size);
    memset(buffer, 0, size);
    buildqueue = (uiBuildQueue_t *)buffer;
    buildqueue->firstitem = (USHORT)firstitem.number;
    buildqueue->buildtimer = (USHORT)buildtimer.number;
    buildqueue->itemoffset = BUILDQUEUE_OFFSET;
    buildqueue->numitems = count;
    FOR_LOOP(i, count) {
        buildqueue->items[i].image = (USHORT)gi.ImageIndex(queue[i].art);
        buildqueue->items[i].starttime = queue[i].starttime;
        buildqueue->items[i].endtime = queue[i].endtime;
    }

    memset(&list, 0, sizeof(list));
    list.flags.type = FT_BUILDQUEUE;
    list.color = COLOR32_WHITE;
    UI_SetFrameRect(&list, BUILDQUEUE_LIST_X, BUILDQUEUE_LIST_Y, BUILDQUEUE_ITEM_W, BUILDQUEUE_ITEM_H);
    UI_WriteProxyFrame(&list, buffer, size);
    gi.MemFree(buffer);
}

static void UI_WriteMultiselect(LPEDICT *ents, DWORD count) {
    DWORD size = sizeof(uiMultiselect_t) + sizeof(uiMultiselectItem_t) * count;
    LPBYTE buffer = gi.MemAlloc(size);
    uiMultiselect_t *multi = (uiMultiselect_t *)buffer;
    uiFrame_t frame;

    memset(buffer, 0, size);
    multi->hp_bar = gi.ImageIndex(Theme_String("SimpleHpBarConsole", "UI\\Widgets\\Console\\Human\\human-statbar-fill.blp"));
    multi->mana_bar = gi.ImageIndex(Theme_String("SimpleManaBarConsole", "UI\\Widgets\\Console\\Human\\human-statbar-fill.blp"));
    multi->offset = MAKE(VECTOR2, 0.031f, 0.050f);
    multi->numcolumns = 6;
    multi->numitems = count;
    FOR_LOOP(i, count) {
        multi->items[i].entity = ents[i]->s.number;
        multi->items[i].image = gi.ImageIndex(FindConfigValue(GetClassName(ents[i]->class_id), STR_ART));
    }

    memset(&frame, 0, sizeof(frame));
    frame.flags.type = FT_MULTISELECT;
    frame.color = COLOR32_WHITE;
    UI_SetFrameRect(&frame, MULTISELECT_X, MULTISELECT_Y, MULTISELECT_SIZE, MULTISELECT_SIZE);
    UI_WriteProxyFrame(&frame, buffer, size);
    gi.MemFree(buffer);
}

void Get_Commands_f(LPEDICT ent) {
    LPEDICT selected = ent && ent->client ? G_GetMainSelectedUnit(ent->client) : NULL;
    gameCommandButton_t buttons[12];
    BYTE count;

    if (!ent || !ent->client || !selected) {
        UI_ClearLayer(ent, LAYER_COMMANDBAR);
        return;
    }
    memset(&ent->client->menu, 0, sizeof(ent->client->menu));

    gi.WriteByte(svc_layout);
    gi.WriteByte(LAYER_COMMANDBAR);
    ui_next_frame_number = 1;
    count = G_GetCommandButtons(selected, buttons, 12);
    FOR_LOOP(i, count) {
        RECT rect = UI_CommandButtonRect(buttons[i].x, buttons[i].y);
        uiFrame_t frame;
        char onclick[128];

        memset(&frame, 0, sizeof(frame));
        frame.flags.type = FT_COMMANDBUTTON;
        frame.color = COLOR32_WHITE;
        frame.tex.index = gi.ImageIndex(buttons[i].art);
        frame.stat = buttons[i].active;
        frame.tooltip = buttons[i].ubertip[0] ? buttons[i].ubertip : buttons[i].tooltip;
        snprintf(onclick, sizeof(onclick), "%s %s", buttons[i].research ? "research" : "button", buttons[i].command);
        frame.onclick = onclick;
        UI_SetFrameRect(&frame, rect.x, rect.y, rect.w, rect.h);
        UI_WriteProxyFrame(&frame, NULL, 0);
    }
    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);
}

void Get_Portrait_f(LPEDICT ent) {
    LPEDICT selected[MAX_SELECTED_ENTITIES];
    DWORD count;

    if (!ent || !ent->client) {
        return;
    }

    count = UI_SelectedUnits(ent->client, selected, MAX_SELECTED_ENTITIES);

    gi.WriteByte(svc_layout);
    gi.WriteByte(LAYER_PORTRAIT);
    ui_next_frame_number = 1;
    if (count == 1) {
        UI_WritePortraitFrame(selected[0]);
    }
    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);

    gi.WriteByte(svc_layout);
    gi.WriteByte(LAYER_INFOPANEL);
    ui_next_frame_number = 1;
    if (count == 1) {
        if (selected[0]->build) {
            UI_WriteBuildQueue(selected[0]);
        } else {
            UI_WriteSingleInfo(selected[0]);
        }
    } else if (count > 1) {
        UI_WriteMultiselect(selected, count);
    }
    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);

    gi.WriteByte(svc_layout);
    gi.WriteByte(LAYER_INVENTORY);
    ui_next_frame_number = 1;
    if (count == 1) {
        UI_WriteInventory(selected[0]);
    }
    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);
}

/* Legacy quest/menu hooks not yet ported to generated proxy frames. */
void UI_ShowQuests(LPEDICT ent) { (void)ent; }
void UI_ShowQuest(LPEDICT ent, LPCQUEST quest) { (void)ent; (void)quest; }
void UI_HideQuests(LPEDICT ent) { (void)ent; }
void UI_Init(void) {}
void UI_ShowInterface(LPEDICT ent, BOOL show, FLOAT duration) { (void)ent; (void)show; (void)duration; }
void UI_ShowMainMenu(LPEDICT ent) { (void)ent; }
void UI_ShowGameInterface(LPEDICT ent) {
    UI_WriteServerConsoleShell(ent);
}
void UI_ShowText(LPEDICT ent, LPCVECTOR2 pos, LPCSTR text, FLOAT duration) { (void)ent; (void)pos; (void)text; (void)duration; }

void UI_AddCancelButton(LPEDICT ent) {
    UI_SetCurrentClient(ent ? ent->client : NULL);
    UI_WriteStart(LAYER_COMMANDBAR);
    UI_AddCommandButton(STR_CmdCancel);
    gi.WriteLong(0);
    gi.WriteShort(0);
    gi.unicast(ent);
    UI_SetCurrentClient(NULL);
}

void UI_AddCommandButton(LPCSTR code) {
    UI_AddCommandButtonExtended(code, false, 0);
}

void UI_AddCommandButtonExtended(LPCSTR code, BOOL research, DWORD level) {
    UI_WriteCommandButton(code, research, level);
}

void UI_ClearCreateGameSlots(void) {}
void UI_AddCreateGameSlot(DWORD slot, LPCSTR name, LPCSTR race, LPCSTR color, DWORD team) {
    (void)slot; (void)name; (void)race; (void)color; (void)team;
}

DWORD UI_LoadTexture(LPCSTR path, BOOL forcewrap) {
    (void)forcewrap;
    return gi.ImageIndex(path);
}

LPCSTR Theme_String(LPCSTR key, LPCSTR def) {
    LPCSTR value = NULL;
    if (key && !strstr(key, "\\") && game.config.theme) {
        value = gi.FindSheetCell(game.config.theme, "Default", key);
    }
    return value ? value : def;
}

FLOAT Theme_Float(LPCSTR key, LPCSTR def) {
    (void)key;
    return def ? atof(def) : 0.0f;
}

void UI_WriteStart(DWORD layer) {
    gi.WriteByte(svc_layout);
    gi.WriteByte(layer);
    ui_next_frame_number = 1;
}
