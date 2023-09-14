#include "r_local.h"
#include "stb/stb_truetype.h"

#define MAX_GLYPHSET 256
#define FONT_SCALE 2
#define INV_SCALE(x) ((x) / (FONT_SCALE * 1000.f))

typedef struct {
    LPTEXTURE image;
    stbtt_bakedchar glyphs[MAX_GLYPHSET];
} glyphSet_t;

typedef struct  font {
    void *data;
    stbtt_fontinfo stbfont;
    glyphSet_t *sets[MAX_GLYPHSET];
    FLOAT size;
    int height;
} font_t;

static const char* utf8_to_codepoint(const char *p, unsigned *dst) {
    unsigned res, n;
    switch (*p & 0xf0) {
        case 0xf0 :  res = *p & 0x07;  n = 3;  break;
        case 0xe0 :  res = *p & 0x0f;  n = 2;  break;
        case 0xd0 :
        case 0xc0 :  res = *p & 0x1f;  n = 1;  break;
        default   :  res = *p;         n = 0;  break;
    }
    while (n--) {
        res = (res << 6) | (*(++p) & 0x3f);
    }
    *dst = res;
    return p + 1;
}

static glyphSet_t* R_LoadGlyphSet(font_t *font, int idx) {
    glyphSet_t *set = ri.MemAlloc(sizeof(glyphSet_t));
    
    /* init image */
    int width = 128;
    int height = 128;
    uint8_t *fontimage;
    FLOAT s;
    int res;
    
retry:
    fontimage = ri.MemAlloc(width * height);
    /* load glyphs */
    s = stbtt_ScaleForMappingEmToPixels(&font->stbfont, 1) /
    stbtt_ScaleForPixelHeight(&font->stbfont, 1);
    res = stbtt_BakeFontBitmap(font->data, 0, font->size * s, fontimage,
                               width, height, idx * 256, 256, set->glyphs);
    
    /* retry with a larger image buffer if the buffer wasn't large enough */
    if (res < 0) {
        width *= 2;
        height *= 2;
        ri.MemFree(fontimage);
        goto retry;
    }
    
    /* adjust glyph yoffsets and xadvance */
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
    FLOAT scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
    int scaled_ascent = ascent * scale + 0.5;
    for (int i = 0; i < 256; i++) {
        set->glyphs[i].yoff += scaled_ascent;
        set->glyphs[i].xadvance = floor(set->glyphs[i].xadvance);
    }
    
    LPCOLOR32 pixels = ri.MemAlloc(sizeof(COLOR32) * width * height);
    /* convert 8bit data to 32bit */
    for (int i = 0; i < width * height; i++) {
        uint8_t n = fontimage[i];
        pixels[i] = (COLOR32) { .r = 255, .g = 255, .b = 255, .a = n };
    }
    set->image = R_AllocateTexture(width, height);
    
    R_LoadTextureMipLevel(set->image, 0, pixels, width, height);
    
    return set;
}


static glyphSet_t* R_GetGlyphSet(font_t *font, int codepoint) {
    int idx = (codepoint >> 8) % MAX_GLYPHSET;
    if (!font->sets[idx]) {
        font->sets[idx] = R_LoadGlyphSet(font, idx);
    }
    return font->sets[idx];
}


LPFONT R_LoadFont(LPCSTR filename, DWORD size) {
    size = MAX(9, size);
    font_t *font = ri.MemAlloc(sizeof(font_t));
    font->size = size * FONT_SCALE;
    
    /* load font into buffer */
    HANDLE file = ri.FileOpen(filename);
    if (!file) { return NULL; }
    /* get size */
    DWORD buf_size = SFileGetFileSize(file, NULL);
    /* load */
    font->data = ri.MemAlloc(buf_size);
    SFileReadFile(file, font->data, buf_size, NULL, NULL);
    ri.FileClose(file);
    
    /* init stbfont */
    int ok = stbtt_InitFont(&font->stbfont, font->data, 0);
    if (!ok) { goto fail; }
    
    /* get height and scale */
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
    FLOAT scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, size);
    font->height = (ascent - descent + linegap) * scale + 0.5;
    
    /* make tab and newline glyphs invisible */
    stbtt_bakedchar *g = R_GetGlyphSet(font, '\n')->glyphs;
    g['\t'].x1 = g['\t'].x0;
    g['\n'].x1 = g['\n'].x0;
    
    return font;
    
fail:
    if (font) { ri.MemFree(font->data); }
    ri.MemFree(font);
    return NULL;
}

void R_ReleaseFont(LPFONT font) {
    for (int i = 0; i < MAX_GLYPHSET; i++) {
        glyphSet_t *set = font->sets[i];
        if (set) {
            R_ReleaseTexture(set->image);
            ri.MemFree(set);
        }
    }
    ri.MemFree(font->data);
    ri.MemFree(font);
}

FLOAT R_GetFontWidth(LPFONT font, LPCSTR text) {
    FLOAT x = 0;
    LPCSTR p = text;
    unsigned codepoint;
    while (*p) {
        p = utf8_to_codepoint(p, &codepoint);
        glyphSet_t *set = R_GetGlyphSet(font, codepoint);
        stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
        x += INV_SCALE(g->xadvance);
    }
    return x;
}


FLOAT R_GetFontHeight(LPFONT font) {
    return FONT_SCALE * INV_SCALE(font->height);
}

BOOL will_word_fit(LPCSTR text, FLOAT width, LPCFONT font) {
    for (LPCSTR p = text; *p && !isspace(*p) && *p != '|';) {
        unsigned codepoint;
        p = utf8_to_codepoint(p, &codepoint);
        glyphSet_t *set = R_GetGlyphSet((LPFONT)font, codepoint);
        stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
        width -= INV_SCALE(g->xadvance);
    }
    return width >= -0.001;
}

static VECTOR2 get_position(LPCDRAWTEXT arg) {
    VECTOR2 pos = { 0 };
    VECTOR2 size = R_GetTextSize(arg);
    switch (arg->halign) {
        case FONT_JUSTIFYRIGHT: pos.x = arg->rect.x + arg->rect.w - size.x; break;
        case FONT_JUSTIFYCENTER: pos.x = arg->rect.x + (arg->rect.w - size.x) / 2; break;
        case FONT_JUSTIFYLEFT: pos.x = arg->rect.x; break;
    }
    switch (arg->valign) {
        case FONT_JUSTIFYBOTTOM: pos.y = arg->rect.y + arg->rect.h - size.y; break;
        case FONT_JUSTIFYMIDDLE: pos.y = arg->rect.y + (arg->rect.h - size.y) / 2; break;
        case FONT_JUSTIFYTOP: pos.y = arg->rect.y; break;
    }
    return pos;
}

static RECT get_uvrect(stbtt_bakedchar *g, FLOAT h, FLOAT w) {
    RECT const uv_rect = {
        .x = g->x0 / w,
        .y = g->y0 / h,
        .w = (g->x1 - g->x0) / w,
        .h = (g->y1 - g->y0) / h,
    };
    return uv_rect;
}

static RECT get_screenrect(LPCVECTOR2 cursor, stbtt_bakedchar *g) {
    RECT const screen = {
        .x = cursor->x + INV_SCALE(g->xoff),
        .y = cursor->y + INV_SCALE(g->yoff),
        .w = INV_SCALE(g->x1 - g->x0),
        .h = INV_SCALE(g->y1 - g->y0),
    };
    return screen;
}

static VECTOR2 process_text(LPCDRAWTEXT arg, BOOL draw) {
    if (!arg->font) {
        return MAKE(VECTOR2, 0, 0);
    }
    VECTOR2 pos = draw ? get_position(arg) : MAKE(VECTOR2, 0, 0);
    COLOR32 color = arg->color;
    VECTOR2 cursor = pos;
    FLOAT maxwidth = 0;
    FLOAT linesize = 0.5 * arg->font->size / 1000.f;
    for (LPCSTR p = arg->text; *p;) {
        if (*p == '\n') {
            cursor.x = pos.x;
            cursor.y += linesize * arg->lineHeight * 1.1;
            p++;
            continue;
        }
        if (!strncmp(p, "|n", 2) || !strncmp(p, "|N", 2)) {
        next_line:
            cursor.x = pos.x;
            cursor.y += linesize * arg->lineHeight * 1.1;
            p += 2;
            continue;
        }
        if (*p == '<') {
            DWORD icon = atoi(p+6);
            switch (*(DWORD*)(p+1)) {
                case MAKEFOURCC('I', 'c', 'o', 'n'):
                    if (draw && arg->icons[icon]) {
                        R_DrawImage(arg->icons[icon],
                                    &MAKE(RECT, cursor.x, cursor.y + linesize * 0.1, linesize, linesize),
                                    &MAKE(RECT, 0, 0, 1, 1),
                                    COLOR32_WHITE);
                    }
                    cursor.x += linesize;
                    break;
            }
            p = strchr(p + 1, '>') + 1;
            continue;;
        }
        if (!strncmp(p, "|r", 2) || !strncmp(p, "|R", 2)) {
            color = COLOR32_WHITE;
            p += 2;
            continue;
        }
        if (!strncmp(p, "|c", 2) || !strncmp(p, "|C", 2)) {
            COLOR32 c;
            sscanf(p+2, "%08x", (DWORD *)&c);
            color.a = c.a;
            color.b = c.r;
            color.g = c.g;
            color.r = c.b;
            p += 10;
            continue;
        }
        if (arg->wordWrap && cursor.x > pos.x && !will_word_fit(p, arg->textWidth - (cursor.x - pos.x), arg->font)) {
            cursor.x = pos.x;
            cursor.y += linesize * arg->lineHeight;
        }
        unsigned codepoint;
        p = utf8_to_codepoint(p, &codepoint);
        glyphSet_t *set = R_GetGlyphSet((LPFONT)arg->font, codepoint);
        stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
        if (draw) {
            FLOAT const w = set->image->width;
            FLOAT const h = set->image->height;
            RECT const uv_rect = get_uvrect(g, h, w);
            RECT const screen = get_screenrect(&cursor, g);
            R_DrawImage(set->image, &screen, &uv_rect, color);
        }
        cursor.x += INV_SCALE(g->xadvance);
        maxwidth = MAX(maxwidth, cursor.x);
    }
    return MAKE(VECTOR2, maxwidth, cursor.y + R_GetFontHeight((LPFONT)arg->font));
}


void R_DrawText(LPCDRAWTEXT arg) {
    process_text(arg, true);
    
//    R_DrawWireRect(&arg->rect, MAKE(COLOR32, 255, 0, 255, 255));
}

VECTOR2 R_GetTextSize(LPCDRAWTEXT arg) {
    return process_text(arg, false);
}
