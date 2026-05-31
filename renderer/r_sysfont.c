#include "r_local.h"

#define SYSFONT_TEXTURE "share/fonts/conchars.pcx"

static LPTEXTURE R_LoadSysFontTextureFile(void) {
    FILE *file;
    long size;
    LPBYTE data;
    LPTEXTURE texture = NULL;

    file = fopen(SYSFONT_TEXTURE, "rb");
    if (!file) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size <= 0 || size > INT32_MAX || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = ri.MemAlloc((DWORD)size);
    if (!data) {
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, (size_t)size, file) == (size_t)size) {
        texture = R_LoadTexturePCX(data, (DWORD)size);
    }
    ri.MemFree(data);
    fclose(file);
    return texture;
}

LPTEXTURE R_MakeSysFontTexture(void) {
    LPTEXTURE texture = R_LoadSysFontTextureFile();

    if (!texture) {
        texture = R_LoadTexture(SYSFONT_TEXTURE);
    }

    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}
