#include "r_local.h"

static LPTEXTURE g_textures = NULL;

int R_RegisterTextureFile(char const *textureFileName) {
    LPTEXTURE tex = (LPTEXTURE)R_LoadTexture(textureFileName);
    if (tex) {
        ADD_TO_LIST(tex, g_textures);
        return tex->texid;
    } else {
        return -1;
    }
}

struct texture const* R_FindTextureByID(DWORD textureID) {
    for (LPCTEXTURE tex = g_textures; tex; tex = tex->next) {
        if (tex->texid == textureID)
            return tex;
    }
    return NULL;
}

void R_BindTexture(LPCTEXTURE texture, DWORD unit) {
    R_Call(glActiveTexture, GL_TEXTURE0 + unit);
    R_Call(glBindTexture, GL_TEXTURE_2D, texture ? texture->texid : tr.whiteTexture->texid);
}

LPTEXTURE R_AllocateTexture(DWORD width, DWORD height) {
    LPTEXTURE texture = ri.MemAlloc(sizeof(TEXTURE));
    R_Call(glGenTextures, 1, &texture->texid);
    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);
    texture->width = width;
    texture->height = height;
    return texture;
}

void R_ReleaseTexture(LPTEXTURE texture) {
    R_Call(glDeleteTextures, 1, texture->texid);
}

void R_LoadTextureMipLevel(LPCTEXTURE pTexture, DWORD level, LPCCOLOR32 pPixels, DWORD width, DWORD height) {
    if (width == 0 || height == 0)
        return;
    R_Call(glBindTexture, GL_TEXTURE_2D, pTexture->texid);
    R_Call(glTexImage2D, GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, pPixels);
    if (level > 0) {
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}
