#include "r_local.h"

#include "../share/fonts/conchars_sysfont.h"

static void R_DecodeSysFontPixels(BYTE pixels[CONCHARS_SYSFONT_WIDTH * CONCHARS_SYSFONT_HEIGHT * 4]) {
    DWORD out = 0;

    for (DWORD i = 0; i + 5 < CONCHARS_SYSFONT_RLE_SIZE; i += 6) {
        DWORD run = conchars_sysfont_rle[i] | (conchars_sysfont_rle[i + 1] << 8);
        BYTE r = conchars_sysfont_rle[i + 2];
        BYTE g = conchars_sysfont_rle[i + 3];
        BYTE b = conchars_sysfont_rle[i + 4];
        BYTE a = conchars_sysfont_rle[i + 5];

        while (run-- && out + 3 < CONCHARS_SYSFONT_WIDTH * CONCHARS_SYSFONT_HEIGHT * 4) {
            pixels[out++] = r;
            pixels[out++] = g;
            pixels[out++] = b;
            pixels[out++] = a;
        }
    }
}

LPTEXTURE R_MakeSysFontTexture(void) {
    LPTEXTURE texture = ri.MemAlloc(sizeof(TEXTURE));
    BYTE pixels[CONCHARS_SYSFONT_WIDTH * CONCHARS_SYSFONT_HEIGHT * 4] = { 0 };

    texture->width = CONCHARS_SYSFONT_WIDTH;
    texture->height = CONCHARS_SYSFONT_HEIGHT;
    R_DecodeSysFontPixels(pixels);
    R_Call(glGenTextures, 1, &texture->texid);
    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return texture;
}
