#include "client/renderer.h"
#include "game/g_local.h"
#include "tools/viewer_common.h"

#include <SDL2/SDL.h>
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

typedef struct {
    LPCFRAMEDEF frame;
    RECT rect;
    bool calculated;
    bool calculating;
} runtime_frame_t;

struct game_import gi;
struct game_locals game;
struct game_export globals;
struct level_locals level;
struct edict_s *g_edicts;

static HANDLE archives[MAX_TOOL_ARCHIVES] = { 0 };
static LPCSTR fdfs[MAX_TOOL_FDFS] = { 0 };
static DWORD num_fdfs = 0;
static LPCSTR root_name = NULL;

static LPCMODEL models[MAX_TOOL_MODELS] = { 0 };
static PATHSTR model_names[MAX_TOOL_MODELS] = { { 0 } };
static DWORD num_models = 1;

static LPCTEXTURE images[MAX_TOOL_IMAGES] = { 0 };
static PATHSTR image_names[MAX_TOOL_IMAGES] = { { 0 } };
static DWORD num_images = 1;

static LPCFONT fonts[MAX_TOOL_FONTS] = { 0 };
static PATHSTR font_names[MAX_TOOL_FONTS] = { { 0 } };
static DWORD font_sizes[MAX_TOOL_FONTS] = { 0 };
static DWORD num_fonts = 1;
static DWORD default_font = 0;

static runtime_frame_t runtimes[MAX_LAYOUT_OBJECTS];
static LPCFRAMEDEF scene_frames[MAX_LAYOUT_OBJECTS];
static DWORD num_scene_frames = 0;
static RECT screen_rect = { 0, 0, UI_VIEW_WIDTH, UI_VIEW_HEIGHT };

static refExport_t re;

static void usage(void) {
    fprintf(stderr,
            "Usage:\n"
            "  fdftool -mpq <archive.mpq> [-mpq <archive.mpq> ...] -fdf <file.fdf> [-fdf <file.fdf> ...] -root <FrameName>\n"
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

void UI_WriteStart(DWORD layer) {
    (void)layer;
}

void UI_WriteFrame(LPCFRAMEDEF frame) {
    if (!frame || num_scene_frames >= MAX_LAYOUT_OBJECTS) {
        return;
    }
    scene_frames[num_scene_frames++] = frame;
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

HANDLE FS_ReadFile(LPCSTR filename, LPDWORD size) {
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

static DWORD RegisterModel(LPCSTR modelName) {
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
    return num_models++;
}

static DWORD RegisterImage(LPCSTR imageName) {
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
    return num_images++;
}

static DWORD RegisterFont(LPCSTR fontName, DWORD fontSize) {
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
    return num_fonts++;
}

bool Com_InMenuMode(void) {
    return true;
}

void Com_SetMenuMode(bool enabled) {
    (void)enabled;
}

static LPCRECT layout_rect(LPCFRAMEDEF frame);

static VECTOR2 rect_x_bounds(LPCRECT rect) {
    return (VECTOR2) { rect->x, rect->x + rect->w };
}

static VECTOR2 rect_y_bounds(LPCRECT rect) {
    return (VECTOR2) { rect->y, rect->y + rect->h };
}

static LPCRECT relative_rect(LPCFRAMEDEF frame, FRAMEPOINT const *point) {
    if (point->relativeTo) {
        return layout_rect(point->relativeTo);
    }
    if (frame && frame->Parent) {
        return layout_rect(frame->Parent);
    }
    return &screen_rect;
}

static FLOAT get_anchor(LPCFRAMEDEF frame,
                        FRAMEPOINT const *point,
                        VECTOR2 (*get)(LPCRECT),
                        bool is_x)
{
    LPCRECT relative = relative_rect(frame, point);
    VECTOR2 base = get(relative);
    SHORT offset = is_x ? point->offset : -point->offset;
    if (point->targetPos == FPP_MID) {
        return (base.x + base.y) * 0.5f + offset;
    } else if (point->targetPos == FPP_MAX) {
        return base.y + offset;
    } else {
        return base.x + offset;
    }
}

static VECTOR2 layout_axis(LPCFRAMEDEF frame,
                           FRAMEPOINT const *points,
                           FLOAT width,
                           VECTOR2 (*get)(LPCRECT),
                           bool is_x)
{
    FRAMEPOINT const *pmin = points + FPP_MIN;
    FRAMEPOINT const *pmid = points + FPP_MID;
    FRAMEPOINT const *pmax = points + FPP_MAX;

    if (pmid->used) {
        FLOAT center = get_anchor(frame, pmid, get, is_x);
        return (VECTOR2) { center - width * 0.5f, width };
    } else if (pmin->used && pmax->used) {
        FLOAT min = get_anchor(frame, pmin, get, is_x);
        FLOAT max = get_anchor(frame, pmax, get, is_x);
        return (VECTOR2) { min, max - min };
    } else if (pmax->used) {
        FLOAT max = get_anchor(frame, pmax, get, is_x);
        return (VECTOR2) { max - width, width };
    } else if (pmin->used) {
        FLOAT min = get_anchor(frame, pmin, get, is_x);
        return (VECTOR2) { min, width };
    } else {
        return (VECTOR2) { 0, width };
    }
}

static LPCRECT layout_rect(LPCFRAMEDEF frame) {
    runtime_frame_t *runtime = NULL;
    VECTOR2 x;
    VECTOR2 y;
    FLOAT width;
    FLOAT height;
    DRAWTEXT drawtext;
    VECTOR2 text_size = { 0, 0 };
    DWORD font = default_font;

    if (!frame) {
        return &screen_rect;
    }

    FOR_LOOP(i, num_scene_frames) {
        if (runtimes[i].frame == frame) {
            runtime = &runtimes[i];
            break;
        }
    }

    if (!runtime) {
        return &screen_rect;
    }
    if (runtime->calculated) {
        return &runtime->rect;
    }
    if (runtime->calculating) {
        return &screen_rect;
    }
    runtime->calculating = true;

    width = frame->Width;
    height = frame->Height;

    if ((frame->Type == FT_TEXT || frame->Type == FT_STRING) && (!width || !height)) {
        if (frame->Font.Index > 0 && frame->Font.Index < MAX_TOOL_FONTS && fonts[frame->Font.Index]) {
            font = frame->Font.Index;
        }
        drawtext = MAKE(DRAWTEXT,
                        .font = fonts[font],
                        .text = frame->Text ? frame->Text : frame->Name,
                        .color = frame->Font.Color.a ? frame->Font.Color : COLOR32_WHITE,
                        .textWidth = width > 0 ? width : screen_rect.w,
                        .lineHeight = 1.33f,
                        .wordWrap = true,
                        .halign = frame->Font.Justification.Horizontal,
                        .valign = frame->Font.Justification.Vertical);
        text_size = re.GetTextSize(&drawtext);
        if (!width) {
            width = text_size.x;
        }
        if (!height) {
            height = text_size.y;
        }
    }

    x = layout_axis(frame, frame->Points.x, width, rect_x_bounds, true);
    y = layout_axis(frame, frame->Points.y, height, rect_y_bounds, false);
    runtime->rect = (RECT) { x.x, y.x, x.y, y.y };
    runtime->calculating = false;
    runtime->calculated = true;
    return &runtime->rect;
}

static void draw_simple_backdrop(LPCFRAMEDEF frame, LPCRECT rect) {
    RECT uv = { 0, 0, 1, 1 };
    COLOR32 color = frame->Color.a ? frame->Color : COLOR32_WHITE;
    DWORD bg = frame->Backdrop.Background ? frame->Backdrop.Background : frame->Texture.Image;
    if (bg > 0 && bg < MAX_TOOL_IMAGES && images[bg]) {
        re.DrawImage(images[bg], rect, &uv, color);
    } else if (images[0]) {
        re.DrawImage(images[0], rect, &uv, color);
    }
}

static void draw_texture(LPCFRAMEDEF frame, LPCRECT rect) {
    RECT uv = Rect_div(&(RECT) {
        frame->Texture.TexCoord.min.x * 0xff,
        frame->Texture.TexCoord.min.y * 0xff,
        (frame->Texture.TexCoord.max.x - frame->Texture.TexCoord.min.x) * 0xff,
        (frame->Texture.TexCoord.max.y - frame->Texture.TexCoord.min.y) * 0xff,
    }, 0xff);
    DWORD image = frame->Texture.Image;
    if (image > 0 && image < MAX_TOOL_IMAGES && images[image]) {
        re.DrawImage(images[image], rect, &uv, frame->Color.a ? frame->Color : COLOR32_WHITE);
    }
}

static void draw_text(LPCFRAMEDEF frame, LPCRECT rect) {
    DWORD font = default_font;
    LPCSTR text = frame->Text ? frame->Text : frame->Name;
    if (frame->Font.Index > 0 && frame->Font.Index < MAX_TOOL_FONTS && fonts[frame->Font.Index]) {
        font = frame->Font.Index;
    }
    if (!fonts[font]) {
        return;
    }
    re.DrawText(&MAKE(DRAWTEXT,
                      .font = fonts[font],
                      .text = text,
                      .rect = *rect,
                      .color = frame->Font.Color.a ? frame->Font.Color : (frame->Color.a ? frame->Color : COLOR32_WHITE),
                      .halign = frame->Font.Justification.Horizontal,
                      .valign = frame->Font.Justification.Vertical,
                      .lineHeight = 1.33f,
                      .wordWrap = true,
                      .textWidth = rect->w));
}

static void draw_portrait(LPCFRAMEDEF frame, LPCRECT rect) {
    DWORD model = frame->Portrait.model;
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

static void draw_frame(LPCFRAMEDEF frame) {
    LPCRECT rect = layout_rect(frame);
    switch (frame->Type) {
        case FT_BACKDROP:
        case FT_TOOLTIPTEXT:
            draw_simple_backdrop(frame, rect);
            break;
        case FT_GLUEBUTTON:
        case FT_GLUETEXTBUTTON: {
            LPFRAMEDEF normal = UI_FindFrame(frame->Control.Backdrop.Normal);
            if (normal) {
                draw_simple_backdrop(normal, rect);
            } else {
                draw_simple_backdrop(frame, rect);
            }
            draw_text(frame, rect);
            break;
        }
        case FT_TEXTURE:
        case FT_SIMPLESTATUSBAR:
            draw_texture(frame, rect);
            break;
        case FT_TEXT:
        case FT_STRING:
            draw_text(frame, rect);
            break;
        case FT_MODEL:
        case FT_PORTRAIT:
        case FT_SPRITE:
            draw_portrait(frame, rect);
            break;
        default:
            if (frame->Texture.Image) {
                draw_texture(frame, rect);
            }
            break;
    }
}

static void draw_scene(void) {
    FOR_LOOP(i, num_scene_frames) {
        draw_frame(scene_frames[i]);
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
    viewDef_t viewdef = { 0 };

    if (!parse_args(argc, argv)) {
        usage();
        return 1;
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

    re.Init(1280, 720);
    images[0] = re.LoadTexture("");
    default_font = RegisterFont("Fonts\\FRIZQT__.TTF", 16);

    gi = (struct game_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = RegisterModel,
        .ImageIndex = RegisterImage,
        .FontIndex = RegisterFont,
        .ReadFileIntoString = ReadFileIntoString,
        .ReadFile = FS_ReadFile,
        .ReadSheet = FS_ParseSLK,
        .ReadConfig = FS_ParseINI,
        .FindSheetCell = FS_FindSheetCell,
        .TextRemoveComments = TextRemoveComments,
        .TextRemoveBom = TextRemoveBom,
    };

    game.config.theme = gi.ReadConfig("UI\\war3skins.txt");
    UI_ClearTemplates();
    FOR_LOOP(i, num_fdfs) {
        UI_ParseFDF(fdfs[i]);
    }

    root = UI_FindFrame(root_name);
    if (!root) {
        fprintf(stderr, "fdftool: root frame not found: %s\n", root_name);
        re.Shutdown();
        Viewer_CloseArchives(archives, MAX_TOOL_ARCHIVES);
        return 1;
    }

    memset(runtimes, 0, sizeof(runtimes));
    memset(scene_frames, 0, sizeof(scene_frames));
    num_scene_frames = 0;
    UI_WriteFrameWithChildren(root, NULL);
    FOR_LOOP(i, num_scene_frames) {
        runtimes[i].frame = scene_frames[i];
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
