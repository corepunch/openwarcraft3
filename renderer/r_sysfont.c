#include "r_local.h"

#include "../share/fonts/fixed_8x13.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 128
#define FONT_GLYPHS FIXED_8X13_GLYPHS
#define FONT_GLYPH_WIDTH FIXED_8X13_WIDTH
#define FONT_GLYPH_HEIGHT FIXED_8X13_HEIGHT
#define FONT_CELL_HEIGHT 16
#define FONT_CELL_BASELINE_Y 1

LPTEXTURE R_MakeSysFontTexture(void) {
    LPTEXTURE texture = ri.MemAlloc(sizeof(TEXTURE));
    COLOR32 color[FONT_WIDTH * FONT_HEIGHT] = { 0 };

    texture->width = FONT_WIDTH;
    texture->height = FONT_HEIGHT;
    FOR_LOOP(codepoint, FONT_GLYPHS) {
        DWORD const glyph_x = (codepoint % 16) * FONT_GLYPH_WIDTH;
        DWORD const glyph_y = (codepoint / 16) * FONT_CELL_HEIGHT + FONT_CELL_BASELINE_Y;

        FOR_LOOP(y, FONT_GLYPH_HEIGHT) {
            BYTE const row = (BYTE)fixed_8x13[codepoint][y];
            FOR_LOOP(x, FONT_GLYPH_WIDTH) {
                DWORD const pixel = (glyph_y + y) * FONT_WIDTH + glyph_x + x;
                COLOR32 col = COLOR32_WHITE;

                col.a = (row & (1 << x)) ? 255 : 0;
                color[pixel] = col;
            }
        }
    }
    R_Call(glGenTextures, 1, &texture->texid);
    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, FONT_WIDTH, FONT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
    return texture;
}
