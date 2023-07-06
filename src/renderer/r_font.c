#include "r_local.h"
#include "stb/stb_truetype.h"

#define MAX_GLYPHSET 256
#define FONT_SCALE 2
#define INV_SCALE(x) ((x) / (FONT_SCALE * 1000.f))

typedef struct {
    LPTEXTURE image;
    stbtt_bakedchar glyphs[MAX_GLYPHSET];
} glyphSet_t;

typedef struct font {
    void *data;
    stbtt_fontinfo stbfont;
    glyphSet_t *sets[MAX_GLYPHSET];
    float size;
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
    float s;
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
    float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
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
    float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, size);
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

float R_GetFontWidth(LPFONT font, LPCSTR text) {
    float x = 0;
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


float R_GetFontHeight(LPFONT font) {
    return FONT_SCALE * INV_SCALE(font->height);
}

VECTOR2 R_GetTextSize(LPCFONT font, LPCSTR text) {
    return (VECTOR2) {
        R_GetFontWidth((LPFONT)font, text),
        R_GetFontHeight((LPFONT)font),
    };
}

void R_DrawText(drawText_t const *arg) {
    VECTOR2 pos = { 0 };
    VECTOR2 size = R_GetTextSize(arg->font, arg->text);
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
    for (LPCSTR p = arg->text; *p;) {
        unsigned codepoint;
        p = utf8_to_codepoint(p, &codepoint);
        glyphSet_t *set = R_GetGlyphSet((LPFONT)arg->font, codepoint);
        stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
        float w = set->image->width;
        float h = set->image->height;
        RECT const uv_rect = {
            .x = g->x0 / w,
            .y = g->y0 / h,
            .w = (g->x1 - g->x0) / w,
            .h = (g->y1 - g->y0) / h,
        };
        RECT const screen = {
            .x = pos.x + INV_SCALE(g->xoff),
            .y = pos.y + INV_SCALE(g->yoff),
            .w = INV_SCALE(g->x1 - g->x0),
            .h = INV_SCALE(g->y1 - g->y0),
        };
        R_DrawImage(set->image, &screen, &uv_rect, COLOR32_WHITE);
        pos.x += INV_SCALE(g->xadvance);
    }
}
