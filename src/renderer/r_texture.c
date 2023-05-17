#include "r_local.h"

static LPTEXTURE g_textures = NULL;

int R_RegisterTextureFile(char const *szTextureFileName) {
    LPTEXTURE tex = R_LoadTexture(szTextureFileName);
    if (tex) {
        tex->lpNext = g_textures;
        g_textures = tex;
        return tex->texid;
    } else {
        return -1;
    }
}

struct texture const* R_FindTextureByID(DWORD dwTextureID) {
    for (LPCTEXTURE tex = g_textures; tex; tex = tex->lpNext) {
        if (tex->texid == dwTextureID)
            return tex;
    }
    return NULL;
}

void R_BindTexture(LPCTEXTURE lpTexture, DWORD dwUnit) {
    glActiveTexture(GL_TEXTURE0 + dwUnit);
    glBindTexture(GL_TEXTURE_2D, lpTexture->texid);
}

LPTEXTURE R_AllocateTexture(DWORD dwWidth, DWORD dwHeight) {
    LPTEXTURE texture = MemAlloc(sizeof(struct texture));
    texture->width = dwWidth;
    texture->height = dwHeight;
    glGenTextures(1, &texture->texid);
    glBindTexture(GL_TEXTURE_2D, texture->texid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

void R_LoadTextureMipLevel(LPTEXTURE pTexture, DWORD dwLevel, LPCCOLOR32 pPixels, DWORD dwWidth, DWORD dwHeight) {
    if (dwWidth == 0 || dwHeight == 0)
        return;
    glBindTexture(GL_TEXTURE_2D, pTexture->texid);
    glTexImage2D(GL_TEXTURE_2D, dwLevel, GL_RGBA, dwWidth, dwHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
    if (dwLevel > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, dwLevel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
}
