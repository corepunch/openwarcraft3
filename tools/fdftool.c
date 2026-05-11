#include "client/renderer.h"
#include "game/g_local.h"
#include "tools/viewer_common.h"
#include "common/mpq.h"
#include "common/net.h"

#include <SDL2/SDL.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATHSTR
#define PATHSTR char[512]
#endif

#define MAX_TOOL_ARCHIVES 64
#define MAX_TOOL_FDFS 64
#define MAX_TOOL_MODELS MAX_MODELS
#define MAX_TOOL_IMAGES MAX_IMAGES
#define MAX_TOOL_FONTS MAX_FONTSTYLES
#define UI_VIEW_WIDTH 0.8f
#define UI_VIEW_HEIGHT 0.6f
#define TEXCOORD_SCALE 255.0f

typedef struct {
    LPCUIFRAME frame;
    RECT rect;
    bool calculated;
} runtime_frame_t;

struct game_import gi;
struct game_locals game;
struct game_export globals;
struct level_locals level;
struct edict_s *g_edicts;
extern FRAMEDEF frames[MAX_LAYOUT_OBJECTS];

static HANDLE archives[MAX_TOOL_ARCHIVES] = { 0 };
static LPCSTR fdfs[MAX_TOOL_FDFS] = { 0 };
static DWORD num_fdfs = 0;
static LPCSTR root_name = NULL;

static LPCMODEL models[MAX_TOOL_MODELS] = { 0 };
static char model_names[MAX_TOOL_MODELS][512] = { { 0 } };
static DWORD num_models = 1;

static LPCTEXTURE images[MAX_TOOL_IMAGES] = { 0 };
static char image_names[MAX_TOOL_IMAGES][512] = { { 0 } };
static DWORD num_images = 1;

static LPCFONT fonts[MAX_TOOL_FONTS] = { 0 };
static char font_names[MAX_TOOL_FONTS][512] = { { 0 } };
static DWORD font_sizes[MAX_TOOL_FONTS] = { 0 };
static DWORD num_fonts = 1;
static DWORD default_font = 0;

static runtime_frame_t runtimes[MAX_LAYOUT_OBJECTS];
static UIFRAME scene_frames[MAX_LAYOUT_OBJECTS];
static DWORD num_scene_frames = 1;
static RECT screen_rect = { 0, 0, UI_VIEW_WIDTH, UI_VIEW_HEIGHT };

#define TOOL_LAYOUT_MAX (4 * 1024 * 1024)
static BYTE tool_layout_data[TOOL_LAYOUT_MAX];
static sizeBuf_t tool_layout = {
    .data = tool_layout_data,
    .maxsize = sizeof(tool_layout_data),
    .cursize = 0,
    .readcount = 0,
};

static refExport_t re;
static DWORD g_debug_draw_pass = 0;

static void FDF_DEBUGF(LPCSTR fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[fdf-debug] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static bool is_mainmenu_root(LPCSTR root) {
    return root && (!strcmp(root, "MainMenuFrame") ||
                    !strcmp(root, "ControlLayer") ||
                    !strcmp(root, "RealmSelect"));
}

static void Tool_Unicast(edict_t *ent) {
    (void)ent;
}

static void Tool_WriteByte(LONG c) {
    MSG_WriteByte(&tool_layout, c);
}

static void Tool_WriteShort(LONG c) {
    MSG_WriteShort(&tool_layout, c);
}

static void Tool_WriteLong(LONG c) {
    MSG_WriteLong(&tool_layout, c);
}

static void Tool_WriteFloat(FLOAT f) {
    MSG_WriteFloat(&tool_layout, f);
}

static void Tool_WriteString(LPCSTR s) {
    MSG_WriteString(&tool_layout, s);
}

extern DWORD layoutBytesWritten;

static void Tool_WriteUIFrame(LPCUIFRAME frame) {
    DWORD before = tool_layout.cursize;
    uiFrame_t empty;
    memset(&empty, 0, sizeof(empty));
    empty.tex.coord[1] = 0xff;
    empty.tex.coord[3] = 0xff;
    MSG_WriteDeltaUIFrame(&tool_layout, &empty, frame, true);
    MSG_WriteShort(&tool_layout, frame->buffer.size);
    MSG_Write(&tool_layout, frame->buffer.data, frame->buffer.size);
    if (tool_layout.cursize >= before) {
        layoutBytesWritten += tool_layout.cursize - before;
    }
}

static void Tool_ResetLayoutBuffer(void) {
    tool_layout.cursize = 0;
    tool_layout.readcount = 0;
    layoutBytesWritten = 0;
}

static bool Tool_DecodeLayoutBuffer(void) {
    sizeBuf_t msg = {
        .data = tool_layout.data,
        .cursize = tool_layout.cursize,
        .readcount = 0,
    };
    DWORD layer = 0;

    memset(runtimes, 0, sizeof(runtimes));
    memset(scene_frames, 0, sizeof(scene_frames));
    scene_frames[0].number = 0;
    scene_frames[0].flags.type = FT_SCREEN;
    scene_frames[0].size.width = UI_VIEW_WIDTH;
    scene_frames[0].size.height = UI_VIEW_HEIGHT;
    scene_frames[0].tex.coord[1] = 0xff;
    scene_frames[0].tex.coord[3] = 0xff;
    num_scene_frames = 1;
    runtimes[0].frame = &scene_frames[0];
    runtimes[0].rect = screen_rect;
    runtimes[0].calculated = true;

    if (msg.cursize < 2 || MSG_ReadByte(&msg) != svc_layout) {
        FDF_DEBUGF("decode_layout: missing svc_layout header");
        return false;
    }
    layer = MSG_ReadByte(&msg);
    FDF_DEBUGF("decode_layout: layer=%u bytes=%u", (unsigned)layer, (unsigned)msg.cursize);

    while (true) {
        DWORD bits = 0;
        DWORD nument;
        LPUIFRAME ent;
        if (msg.readcount + sizeof(WORD) * 2 > msg.cursize) {
            break;
        }
        nument = MSG_ReadEntityBits(&msg, &bits);
        if (nument == 0 && bits == 0) {
            break;
        }
        if (nument >= MAX_LAYOUT_OBJECTS) {
            FDF_DEBUGF("decode_layout: frame index out of bounds: %u", (unsigned)nument);
            break;
        }

        ent = &scene_frames[nument];
        ent->tex.coord[1] = 0xff;
        ent->tex.coord[3] = 0xff;
        MSG_ReadDeltaUIFrame(&msg, ent, nument, bits);

        if (msg.readcount + sizeof(WORD) > msg.cursize) {
            break;
        }
        ent->buffer.size = MSG_ReadShort(&msg);
        if (msg.readcount + ent->buffer.size > msg.cursize) {
            FDF_DEBUGF("decode_layout: typedata out of bounds frame=%u size=%u", (unsigned)nument, (unsigned)ent->buffer.size);
            break;
        }
        ent->buffer.data = msg.data + msg.readcount;
        msg.readcount += ent->buffer.size;
        num_scene_frames = MAX(num_scene_frames, nument + 1);
    }

    FOR_LOOP(i, num_scene_frames) {
        runtimes[i].frame = &scene_frames[i];
    }
    return true;
}

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  fdftool -mpq <archive.mpq> [-mpq <archive.mpq> ...] -fdf <file.fdf> [-fdf <file.fdf> ...] -root <FrameName>\n"
            "\n"
            "Note: both '\\' and '/' path separators are accepted.\n"
            "\n"
            "Examples:\n"
            "  fdftool -mpq War3.mpq -fdf UI\\FrameDef\\Glue\\MainMenu.fdf -root MainMenuFrame\n"
            "  fdftool -mpq War3.mpq -fdf UI\\FrameDef\\UI\\ConsoleUI.fdf -fdf UI\\FrameDef\\UI\\ResourceBar.fdf -root ConsoleUI\n");
}

static void errorf(LPCSTR fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void TextRemoveComments(LPSTR buffer) {
    BOOL in_single_line_comment = false;
    BOOL in_block_comment = false;
    DWORD num_quotes = 0;
    char *src = buffer;
    char *dest = buffer;
    while (*src != '\0') {
        if (!in_single_line_comment && !in_block_comment) {
            if (*src == '"') {
                num_quotes++;
                *dest++ = *src++;
            } else if (num_quotes & 1) {
                *dest++ = *src++;
            } else if (*src == '/' && *(src + 1) == '/') {
                in_single_line_comment = true;
                src += 2;
            } else if (*src == '/' && *(src + 1) == '*') {
                in_block_comment = true;
                src += 2;
            } else {
                *dest++ = *src++;
            }
        } else if (in_single_line_comment && *src == '\n') {
            in_single_line_comment = false;
            *dest++ = *src++;
        } else if (in_block_comment && *src == '*' && *(src + 1) == '/') {
            in_block_comment = false;
            src += 2;
        } else {
            src++;
        }
    }
    *dest = '\0';
}

BOMStatus TextRemoveBom(LPSTR buffer) {
    unsigned char utf8_bom[] = { 0xEF, 0xBB, 0xBF };
    unsigned char utf16le_bom[] = { 0xFF, 0xFE };
    unsigned char utf16be_bom[] = { 0xFE, 0xFF };

    if (memcmp(buffer, utf8_bom, 3) == 0) {
        memmove(buffer, buffer + 3, strlen(buffer + 3) + 1);
        return UTF8_BOM_FOUND;
    } else if (memcmp(buffer, utf16le_bom, 2) == 0) {
        memmove(buffer, buffer + 2, strlen(buffer + 2) + 1);
        return UTF16LE_BOM_FOUND;
    } else if (memcmp(buffer, utf16be_bom, 2) == 0) {
        memmove(buffer, buffer + 2, strlen(buffer + 2) + 1);
        return UTF16BE_BOM_FOUND;
    } else {
        return NO_BOM;
    }
}

HANDLE MemAlloc(long size) {
    return Viewer_MemAlloc(size);
}

void MemFree(HANDLE mem) {
    Viewer_MemFree(mem);
}

HANDLE FS_OpenFile(LPCSTR fileName) {
    return Viewer_OpenFile(archives, sizeof(archives) / sizeof(archives[0]), fileName);
}

void FS_CloseFile(HANDLE file) {
    Viewer_CloseFile(file);
}

bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted) {
    return Viewer_ExtractFile(archives, sizeof(archives) / sizeof(archives[0]), toExtract, extracted);
}

static HANDLE Tool_ReadFile(LPCSTR filename, LPDWORD size) {
    HANDLE fp = FS_OpenFile(filename);
    HANDLE buffer = NULL;
    DWORD file_size;
    if (!fp) {
        return NULL;
    }
    file_size = SFileGetFileSize(fp, NULL);
    buffer = MemAlloc(file_size);
    SFileReadFile(fp, buffer, file_size, NULL, NULL);
    FS_CloseFile(fp);
    *size = file_size;
    return buffer;
}

LPSTR ReadFileIntoString(LPCSTR fileName) {
    HANDLE fp = FS_OpenFile(fileName);
    LPSTR buffer = NULL;
    DWORD file_size;
    if (!fp) {
        return NULL;
    }
    file_size = SFileGetFileSize(fp, NULL);
    buffer = MemAlloc(file_size + 1);
    SFileReadFile(fp, buffer, file_size, NULL, NULL);
    buffer[file_size] = '\0';
    FS_CloseFile(fp);
    return buffer;
}

static int RegisterModel(LPCSTR modelName) {
    if (!modelName || !*modelName) {
        return 0;
    }
    FOR_LOOP(i, num_models) {
        if (!strcmp(model_names[i], modelName)) {
            return i;
        }
    }
    if (num_models >= MAX_TOOL_MODELS) {
        return 0;
    }
    strncpy(model_names[num_models], modelName, sizeof(model_names[0]) - 1);
    models[num_models] = re.LoadModel(modelName);
    if (!models[num_models]) {
        FDF_DEBUGF("RegisterModel: failed model=%s slot=%u", modelName, (unsigned)num_models);
    }
    return num_models++;
}

static int RegisterImage(LPCSTR imageName) {
    if (!imageName || !*imageName) {
        return 0;
    }
    FOR_LOOP(i, num_images) {
        if (!strcmp(image_names[i], imageName)) {
            return i;
        }
    }
    if (num_images >= MAX_TOOL_IMAGES) {
        return 0;
    }
    strncpy(image_names[num_images], imageName, sizeof(image_names[0]) - 1);
    images[num_images] = re.LoadTexture(imageName);
    if (!images[num_images]) {
        FDF_DEBUGF("RegisterImage: failed image=%s slot=%u", imageName, (unsigned)num_images);
    }
    return num_images++;
}

static int RegisterFont(LPCSTR fontName, DWORD fontSize) {
    if (!fontName || !*fontName) {
        return default_font;
    }
    FOR_LOOP(i, num_fonts) {
        if (font_sizes[i] == fontSize && !strcmp(font_names[i], fontName)) {
            return i;
        }
    }
    if (num_fonts >= MAX_TOOL_FONTS) {
        return default_font;
    }
    strncpy(font_names[num_fonts], fontName, sizeof(font_names[0]) - 1);
    font_sizes[num_fonts] = fontSize;
    fonts[num_fonts] = re.LoadFont(fontName, fontSize ? fontSize : 16);
    if (!fonts[num_fonts]) {
        FDF_DEBUGF("RegisterFont: failed font=%s size=%u slot=%u", fontName, (unsigned)fontSize, (unsigned)num_fonts);
    }
    return num_fonts++;
}

bool Com_InMenuMode(void) {
    return true;
}

void Com_SetMenuMode(bool enabled) {
    (void)enabled;
}

static LPCRECT layout_rect(LPCUIFRAME frame);

static VECTOR2 rect_x_bounds(LPCRECT rect) {
    return (VECTOR2) { rect->x, rect->x + rect->w };
}

static VECTOR2 rect_y_bounds(LPCRECT rect) {
    return (VECTOR2) { rect->y, rect->y + rect->h };
}

static LPCRECT relative_rect(LPCUIFRAME frame, uiFramePoint_t const *point) {
    if (point->relativeTo == UI_PARENT) {
        if (frame->parent < MAX_LAYOUT_OBJECTS) {
            return layout_rect(scene_frames + frame->parent);
        }
    } else if ((DWORD)point->relativeTo < MAX_LAYOUT_OBJECTS) {
        return layout_rect(scene_frames + point->relativeTo);
    }
    return &screen_rect;
}

static FLOAT get_anchor(LPCUIFRAME frame,
                        uiFramePoint_t const *point,
                        VECTOR2 (*get)(LPCRECT),
                        bool is_x)
{
    LPCRECT relative = relative_rect(frame, point);
    VECTOR2 base = get(relative);
    FLOAT offset = (is_x ? point->offset : -point->offset) / UI_FRAMEPOINT_SCALE;
    if (point->targetPos == FPP_MID) {
        return (base.x + base.y) * 0.5f + offset;
    } else if (point->targetPos == FPP_MAX) {
        return base.y + offset;
    }
    return base.x + offset;
}

static VECTOR2 layout_axis(LPCUIFRAME frame,
                           uiFramePoints_t const points,
                           FLOAT width,
                           VECTOR2 (*get)(LPCRECT),
                           bool is_x)
{
    uiFramePoint_t const *pmin = points + FPP_MIN;
    uiFramePoint_t const *pmid = points + FPP_MID;
    uiFramePoint_t const *pmax = points + FPP_MAX;

    if (pmid->used) {
        FLOAT center = get_anchor(frame, pmid, get, is_x);
        return (VECTOR2) { center - width * 0.5f, width };
    }
    if (pmin->used && pmax->used) {
        FLOAT min = get_anchor(frame, pmin, get, is_x);
        FLOAT max = get_anchor(frame, pmax, get, is_x);
        return (VECTOR2) { min, max - min };
    }
    if (pmax->used) {
        FLOAT max = get_anchor(frame, pmax, get, is_x);
        return (VECTOR2) { max - width, width };
    }
    if (pmin->used) {
        FLOAT min = get_anchor(frame, pmin, get, is_x);
        return (VECTOR2) { min, width };
    }
    return (VECTOR2) { 0, width };
}

static bool has_axis_points(uiFramePoints_t const points) {
    return points[FPP_MIN].used || points[FPP_MID].used || points[FPP_MAX].used;
}

static LPCRECT layout_rect(LPCUIFRAME frame) {
    runtime_frame_t *runtime;
    VECTOR2 x;
    VECTOR2 y;
    VECTOR2 text_size;
    DRAWTEXT drawtext;
    uiLabel_t const *label;
    DWORD font;
    LPCSTR text;
    FLOAT width;
    FLOAT height;

    if (!frame || frame->number >= MAX_LAYOUT_OBJECTS) {
        return &screen_rect;
    }

    runtime = &runtimes[frame->number];
    if (runtime->calculated) {
        return &runtime->rect;
    }
    runtime->calculated = true;

    width = frame->size.width;
    height = frame->size.height;

    if ((frame->flags.type == FT_STRING || frame->flags.type == FT_TEXT) && (width <= 0 || height <= 0)) {
        label = frame->buffer.data;
        font = default_font;
        text = frame->text ? frame->text : "";

        if (label && label->font > 0 && label->font < MAX_TOOL_FONTS && fonts[label->font]) {
            font = label->font;
        }

        if (font > 0 && font < MAX_TOOL_FONTS && fonts[font]) {
            drawtext = MAKE(DRAWTEXT,
                            .font = fonts[font],
                            .text = text,
                            .color = frame->color.a ? frame->color : COLOR32_WHITE,
                            .halign = label ? label->textalignx : FONT_JUSTIFYLEFT,
                            .valign = label ? label->textaligny : FONT_JUSTIFYTOP,
                            .lineHeight = 1.33f,
                            .wordWrap = true,
                            .textWidth = width > 0 ? width : UI_VIEW_WIDTH);
            text_size = re.GetTextSize(&drawtext);
            if (width <= 0) {
                width = text_size.x;
            }
            if (height <= 0) {
                height = text_size.y;
            }
        }
    }

    x = has_axis_points(frame->points.x)
        ? layout_axis(frame, frame->points.x, width, rect_x_bounds, true)
        : (VECTOR2) { 0, width };

    y = has_axis_points(frame->points.y)
        ? layout_axis(frame, frame->points.y, height, rect_y_bounds, false)
        : (VECTOR2) { 0, height };

    runtime->rect = (RECT) { x.x, y.x, x.y, y.y };
    return &runtime->rect;
}

static RECT get_uvrect(uint8_t const *texcoord) {
    RECT const uv = {
        texcoord[0],
        texcoord[2],
        texcoord[1] - texcoord[0],
        texcoord[3] - texcoord[2],
    };
    return uv;
}

typedef enum {
    BACKDROPINSET_RIGHT,
    BACKDROPINSET_TOP,
    BACKDROPINSET_BOTTOM,
    BACKDROPINSET_LEFT,
} BACKDROPINSET;

#define NUM_BACKDROP_CORNERS 8

static void backdrop_rects(LPCRECT screen, LPRECT rects, FLOAT corner_size) {
    FLOAT x[] = { 0, corner_size, screen->w - corner_size, screen->w };
    FLOAT y[] = { 0, corner_size, screen->h - corner_size, screen->h };

    FOR_LOOP(i, BACKDROP_SIZE) {
        rects[i].x = screen->x + x[i % 3];
        rects[i].y = screen->y + y[i / 3];
        rects[i].w = x[(i % 3) + 1] - x[i % 3];
        rects[i].h = y[(i / 3) + 1] - y[i / 3];
    }
}

static FLOAT backdrop_edge_tile(LPCRECT rect, BACKDROPCORNER edge, FLOAT imagesize) {
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

static BOOL backdrop_edge_flip(BACKDROPCORNER edge) {
    switch (edge) {
        case BACKDROP_TOP_EDGE:
        case BACKDROP_BOTTOM_EDGE:
            return true;
        default:
            return false;
    }
}

static void draw_texture(LPCUIFRAME frame, LPCRECT rect) {
    RECT uv;
    RECT suv;

    if (!frame || frame->tex.index <= 0 || frame->tex.index >= MAX_TOOL_IMAGES)
        return;
    if (!images[frame->tex.index])
        return;

    uv = get_uvrect(frame->tex.coord);
    suv = Rect_div(&uv, 0xff);
    re.DrawImage(images[frame->tex.index], rect, &suv, frame->color);
}

static void draw_backdrop(LPCUIFRAME frame, LPCRECT rect, uiBackdrop_t const *backdrop) {
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
    size2_t back_size;
    size2_t edge_size;
    RECT uv;
    RECT background;

    if (!frame || !backdrop)
        return;
    if (backdrop->Background <= 0 || backdrop->Background >= MAX_TOOL_IMAGES)
        return;
    if (backdrop->EdgeFile <= 0 || backdrop->EdgeFile >= MAX_TOOL_IMAGES)
        return;
    if (!images[backdrop->Background] || !images[backdrop->EdgeFile])
        return;

    backdrop_rects(rect, rects, backdrop->CornerSize);
    back_size = re.GetTextureSize(images[backdrop->Background]);
    edge_size = re.GetTextureSize(images[backdrop->EdgeFile]);

    FOR_LOOP(i, NUM_BACKDROP_CORNERS) {
        FLOAT const k = 1.0f / NUM_BACKDROP_CORNERS;
        FLOAT const h = edge_size.height / 1000.f;
        FLOAT const tile = backdrop_edge_tile(rects + corners[i], corners[i], h);
        RECT const edge_uv = { i * k, 0, k, tile };
        BOOL const flip = backdrop_edge_flip(corners[i]);

        if ((backdrop->CornerFlags & (1 << corners[i])) == 0)
            continue;

        re.DrawImageEx(&MAKE(DRAWIMAGE,
                             .texture = images[backdrop->EdgeFile],
                             .alphamode = BLEND_MODE_BLEND,
                             .screen = rects[corners[i]],
                             .uv = edge_uv,
                             .color = frame->color,
                             .rotate = flip,
                             .shader = SHADER_UI));
    }

    uv = MAKE(RECT, 0, 0, 1, 1);
    background = *rect;
    background.x += backdrop->BackgroundInsets[BACKDROPINSET_LEFT];
    background.y += backdrop->BackgroundInsets[BACKDROPINSET_TOP];
    background.w -= backdrop->BackgroundInsets[BACKDROPINSET_LEFT];
    background.w -= backdrop->BackgroundInsets[BACKDROPINSET_RIGHT];
    background.h -= backdrop->BackgroundInsets[BACKDROPINSET_TOP];
    background.h -= backdrop->BackgroundInsets[BACKDROPINSET_BOTTOM];

    if (backdrop->TileBackground) {
        uv.w = background.w / (back_size.width / 1000.f);
        uv.h = background.h / (back_size.height / 1000.f);
    }

    re.DrawImageEx(&MAKE(DRAWIMAGE,
                         .texture = images[backdrop->Background],
                         .alphamode = BLEND_MODE_BLEND,
                         .screen = background,
                         .uv = uv,
                         .color = frame->color,
                         .rotate = false,
                         .shader = SHADER_UI));
}

static void draw_simple_button(LPCUIFRAME frame, LPCRECT rect) {
    uiSimpleButton_t const *button = frame->buffer.data;
    if (!button) {
        return;
    }
    if (button->normal.texture > 0 && button->normal.texture < MAX_TOOL_IMAGES && images[button->normal.texture]) {
        RECT const uv = get_uvrect((BYTE const *)&button->normal.texcoord);
        RECT const suv = Rect_div(&uv, 0xff);
        re.DrawImage(images[button->normal.texture], rect, &suv, COLOR32_WHITE);
    }
    if (button->normal.font > 0 && button->normal.font < MAX_TOOL_FONTS && fonts[button->normal.font] && frame->text) {
        re.DrawText(&MAKE(DRAWTEXT,
                          .rect = *rect,
                          .font = fonts[button->normal.font],
                          .text = frame->text,
                          .color = button->normal.fontcolor,
                          .textWidth = rect->w,
                          .wordWrap = true,
                          .lineHeight = 1.33f));
    }
}

static void draw_text(LPCUIFRAME frame, LPCRECT rect) {
    uiLabel_t const *label = frame->buffer.data;
    DWORD font = default_font;
    RECT draw_rect = *rect;
    LPCSTR text = frame->text ? frame->text : "";
    if (label && label->font > 0 && label->font < MAX_TOOL_FONTS && fonts[label->font]) {
        font = label->font;
        draw_rect.x += label->offsetx;
        draw_rect.y += label->offsety;
    }
    if (!fonts[font]) {
        return;
    }
    re.DrawText(&MAKE(DRAWTEXT,
                      .font = fonts[font],
                      .text = text,
                      .rect = draw_rect,
                      .color = frame->color.a ? frame->color : COLOR32_WHITE,
                      .halign = label ? label->textalignx : FONT_JUSTIFYLEFT,
                      .valign = label ? label->textaligny : FONT_JUSTIFYTOP,
                      .lineHeight = 1.33f,
                      .wordWrap = true,
                      .textWidth = draw_rect.w));
}

static void draw_textarea(LPCUIFRAME frame, LPCRECT rect) {
    uiTextArea_t const *textArea = frame->buffer.data;
    RECT scr = *rect;
    DWORD font = default_font;
    if (!textArea) {
        return;
    }
    if (textArea->font > 0 && textArea->font < MAX_TOOL_FONTS && fonts[textArea->font]) {
        font = textArea->font;
    }
    if (!fonts[font]) {
        return;
    }
    scr.x += textArea->inset;
    scr.y += textArea->inset;
    scr.w -= textArea->inset * 2;
    scr.h -= textArea->inset * 2;
    re.DrawText(&MAKE(DRAWTEXT,
                      .font = fonts[font],
                      .text = frame->text ? frame->text : "",
                      .color = frame->color.a ? frame->color : COLOR32_WHITE,
                      .halign = FONT_JUSTIFYLEFT,
                      .valign = FONT_JUSTIFYTOP,
                      .lineHeight = 1.33f,
                      .textWidth = scr.w,
                      .rect = scr,
                      .wordWrap = true));
}

static void draw_portrait(LPCUIFRAME frame, LPCRECT rect) {
    DWORD model = frame->tex.index;
    RECT viewport;
    if (model == 0 || model >= MAX_TOOL_MODELS || !models[model]) {
        return;
    }
    viewport = (RECT) {
        rect->x / UI_VIEW_WIDTH,
        1.0f - (rect->y + rect->h) / UI_VIEW_HEIGHT,
        rect->w / UI_VIEW_WIDTH,
        rect->h / UI_VIEW_HEIGHT,
    };
    re.DrawPortrait(models[model], &viewport);
}

static void draw_frame(LPCUIFRAME frame) {
    LPCRECT rect = layout_rect(frame);
    switch (frame->flags.type) {
        case FT_BACKDROP:
        case FT_TOOLTIPTEXT:
            if (frame->buffer.size >= sizeof(uiBackdrop_t)) {
                draw_backdrop(frame, rect, frame->buffer.data);
            }
            break;
        case FT_GLUEBUTTON:
        case FT_GLUETEXTBUTTON:
            if (frame->buffer.size >= sizeof(uiGlueTextButton_t)) {
                uiGlueTextButton_t const *glue = frame->buffer.data;
                draw_backdrop(frame, rect, &glue->normal);
            }
            break;
        case FT_SIMPLEBUTTON:
            draw_simple_button(frame, rect);
            break;
        case FT_TEXTURE:
        case FT_SIMPLESTATUSBAR:
            draw_texture(frame, rect);
            break;
        case FT_TEXT:
        case FT_STRING:
            draw_text(frame, rect);
            break;
        case FT_TEXTAREA:
            draw_textarea(frame, rect);
            break;
        case FT_MODEL:
        case FT_PORTRAIT:
        case FT_SPRITE:
            draw_portrait(frame, rect);
            break;
        default:
            if (frame->tex.index) {
                draw_texture(frame, rect);
            }
            break;
    }
}

static void draw_scene(void) {
    size2_t const window = re.GetWindowSize();
    g_debug_draw_pass++;
    if (g_debug_draw_pass <= 6) {
        FDF_DEBUGF("draw_scene_begin: pass=%u frames=%u window=%ux%u",
                   (unsigned)g_debug_draw_pass,
                   (unsigned)num_scene_frames,
                   (unsigned)window.width,
                   (unsigned)window.height);
    }
    for (DWORD i = 1; i < num_scene_frames; i++) {
        if (scene_frames[i].number == 0 && scene_frames[i].flags.type == 0) {
            continue;
        }
        draw_frame(&scene_frames[i]);
    }
    for (DWORD i = 1; i < num_scene_frames; i++) {
        if (scene_frames[i].number == 0 && scene_frames[i].flags.type == 0) {
            continue;
        }
        LPCRECT rect = layout_rect(&scene_frames[i]);
        RECT pixel = {
            rect->x * window.width / UI_VIEW_WIDTH,
            rect->y * window.height / UI_VIEW_HEIGHT,
            rect->w * window.width / UI_VIEW_WIDTH,
            rect->h * window.height / UI_VIEW_HEIGHT,
        };
        re.DrawSelectionRect(&pixel, MAKE(COLOR32, 0, 255, 0, 200));
    }
    if (g_debug_draw_pass <= 6) {
        FDF_DEBUGF("draw_scene_end: pass=%u", (unsigned)g_debug_draw_pass);
    }
}

static bool parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "-mpq=", 5)) {
            if (Viewer_AddArchive(archives, MAX_TOOL_ARCHIVES, argv[i] + 5) == NULL) {
                return false;
            }
        } else if (!strcmp(argv[i], "-mpq")) {
            if (++i >= argc) {
                return false;
            }
            if (Viewer_AddArchive(archives, MAX_TOOL_ARCHIVES, argv[i]) == NULL) {
                return false;
            }
        } else if (!strncmp(argv[i], "-fdf=", 5)) {
            if (num_fdfs >= MAX_TOOL_FDFS) {
                return false;
            }
            fdfs[num_fdfs++] = argv[i] + 5;
        } else if (!strcmp(argv[i], "-fdf")) {
            if (++i >= argc || num_fdfs >= MAX_TOOL_FDFS) {
                return false;
            }
            fdfs[num_fdfs++] = argv[i];
        } else if (!strncmp(argv[i], "-root=", 6)) {
            root_name = argv[i] + 6;
        } else if (!strcmp(argv[i], "-root")) {
            if (++i >= argc) {
                return false;
            }
            root_name = argv[i];
        } else {
            return false;
        }
    }
    return num_fdfs > 0 && root_name != NULL;
}

int main(int argc, char **argv) {
    bool running = true;
    LPFRAMEDEF root = NULL;
    bool scene_built = false;
    viewDef_t viewdef = { 0 };

    if (!parse_args(argc, argv)) {
        usage();
        return 1;
    }

    FDF_DEBUGF("args: root=%s fdf_count=%u", root_name ? root_name : "<null>", (unsigned)num_fdfs);
    FOR_LOOP(i, num_fdfs) {
        FDF_DEBUGF("args: fdf[%u]=%s", (unsigned)i, fdfs[i] ? fdfs[i] : "<null>");
    }

    re = R_GetAPI((refImport_t) {
        .FileOpen = FS_OpenFile,
        .FileExtract = FS_ExtractFile,
        .FileClose = FS_CloseFile,
        .InMenuMode = Com_InMenuMode,
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ReadSheet = FS_ParseSLK,
        .FindSheetCell = FS_FindSheetCell,
        .error = errorf,
    });

    re.Init(VIEWER_WINDOW_WIDTH, VIEWER_WINDOW_HEIGHT);
    images[0] = re.LoadTexture("");
    default_font = RegisterFont("Fonts/FRIZQT__.TTF", 16);

    gi = (struct game_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = RegisterModel,
        .ImageIndex = RegisterImage,
        .FontIndex = RegisterFont,
        .ReadFileIntoString = ReadFileIntoString,
        .ReadFile = Tool_ReadFile,
        .ReadSheet = FS_ParseSLK,
        .ReadConfig = FS_ParseINI,
        .FindSheetCell = FS_FindSheetCell,
        .TextRemoveComments = TextRemoveComments,
        .TextRemoveBom = TextRemoveBom,
        .WriteByte = Tool_WriteByte,
        .WriteShort = Tool_WriteShort,
        .WriteLong = Tool_WriteLong,
        .WriteFloat = Tool_WriteFloat,
        .WriteString = Tool_WriteString,
        .WriteUIFrame = Tool_WriteUIFrame,
        .unicast = Tool_Unicast,
    };

    game.config.theme = gi.ReadConfig("UI\\war3skins.txt");

    Tool_ResetLayoutBuffer();

    if (is_mainmenu_root(root_name)) {
        FDF_DEBUGF("scene_build: mainmenu root path (%s)", root_name);
        UI_Init();
        UI_ShowMainMenu(NULL);
        root = UI_FindFrame(root_name);
        if (root) {
            UI_WriteStart(LAYER_CONSOLE);
            UI_WriteFrameWithChildren(root, NULL);
            gi.WriteLong(0);
            scene_built = Tool_DecodeLayoutBuffer();
        }
    } else {
        FDF_DEBUGF("scene_build: parse-only root path (%s)", root_name);
        UI_ClearTemplates();
        FOR_LOOP(i, num_fdfs) {
            FDF_DEBUGF("scene_build: parse fdf[%u]=%s", (unsigned)i, fdfs[i] ? fdfs[i] : "<null>");
            UI_ParseFDF(fdfs[i]);
        }
        root = UI_FindFrame(root_name);
        if (root) {
            UI_WriteStart(LAYER_CONSOLE);
            UI_WriteFrameWithChildren(root, NULL);
            gi.WriteLong(0);
            scene_built = Tool_DecodeLayoutBuffer();
        }
    }

    if (!root || !scene_built) {
        fprintf(stderr, "fdftool: root frame not found: %s\n", root_name);
        re.Shutdown();
        Viewer_CloseArchives(archives, MAX_TOOL_ARCHIVES);
        return 1;
    }

    FOR_LOOP(i, num_scene_frames) {
        if (i < 64) {
            FDF_DEBUGF("scene_frame[%u]: type=%u parent=%u typedata=%u text=%s",
                       (unsigned)i,
                       (unsigned)scene_frames[i].flags.type,
                       (unsigned)scene_frames[i].parent,
                       (unsigned)scene_frames[i].buffer.size,
                       scene_frames[i].text ? scene_frames[i].text : "");
        }
    }

    viewdef.viewport = (RECT) { 0, 0, 1, 1 };
    viewdef.scissor = (RECT) { 0, 0, 1, 1 };
    viewdef.rdflags = RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL;
    Matrix4_identity(&viewdef.viewProjectionMatrix);
    Matrix4_identity(&viewdef.lightMatrix);
    Matrix4_identity(&viewdef.textureMatrix);

    fprintf(stderr, "fdftool: loaded %u frames from root %s\n", (unsigned)num_scene_frames, root_name);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        viewdef.time = SDL_GetTicks();
        viewdef.deltaTime = 16;
        re.BeginFrame();
        re.RenderFrame(&viewdef);
        draw_scene();
        re.PrintSysText("fdftool: FDF scene viewer", 10, 10, COLOR32_WHITE);
        re.PrintSysText(root_name, 10, 28, COLOR32_WHITE);
        re.EndFrame();
    }

    re.Shutdown();
    Viewer_CloseArchives(archives, MAX_TOOL_ARCHIVES);
    return 0;
}
