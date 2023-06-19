#include "r_local.h"
#include "stb/stb_truetype.h"

#define MAX_GLYPHSET 256

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
    font_t *font = ri.MemAlloc(sizeof(font_t));
    font->size = size;
    
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

void R_ReleaseFont(font_t *font) {
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

float R_GetFontWidth(font_t *font, LPCSTR text) {
    float x = 0;
    LPCSTR p = text;
    unsigned codepoint;
    while (*p) {
        p = utf8_to_codepoint(p, &codepoint);
        glyphSet_t *set = R_GetGlyphSet(font, codepoint);
        stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
        x += g->xadvance / 1000.f;
    }
    return x;
}


float R_GetFontHeight(font_t *font) {
    return font->height / 1000.f;
}

void R_DrawText(drawText_t const *arg) {
    LPCSTR p = arg->text;
    float x = 0;
    float y = 0;
    float w = R_GetFontWidth((LPFONT)arg->font, arg->text);
    float h = R_GetFontHeight((LPFONT)arg->font);
    unsigned codepoint;
    switch (arg->halign) {
        case FONT_JUSTIFYRIGHT: x = arg->rect.x + arg->rect.w - w; break;
        case FONT_JUSTIFYCENTER: x = arg->rect.x + (arg->rect.w - w) / 2; break;
        case FONT_JUSTIFYLEFT: x = arg->rect.y; break;
    }
    switch (arg->valign) {
        case FONT_JUSTIFYBOTTOM: y = arg->rect.y + arg->rect.h - h; break;
        case FONT_JUSTIFYMIDDLE: y = arg->rect.y + (arg->rect.h - h) / 2; break;
        case FONT_JUSTIFYTOP: y = arg->rect.y; break;
    }
    while (*p) {
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
            .x = x + g->xoff / 1000.f,
            .y = y + g->yoff / 1000.f,
            .w = (g->x1 - g->x0) / 1000.f,
            .h = (g->y1 - g->y0) / 1000.f,
        };
        
        R_DrawImage(set->image, &screen, &uv_rect);
        x += g->xadvance / 1000.f;
    }
}
