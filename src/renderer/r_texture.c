#include "r_local.h"

static LPTEXTURE g_textures = NULL;

int R_RegisterTextureFile(char const *textureFileName) {
    LPTEXTURE tex = R_LoadTexture(textureFileName);
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
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture ? texture->texid : tr.whiteTexture->texid);
}

LPTEXTURE R_AllocateTexture(DWORD width, DWORD height) {
    LPTEXTURE texture = ri.MemAlloc(sizeof(TEXTURE));
    texture->width = width;
    texture->height = height;
    glGenTextures(1, &texture->texid);
    glBindTexture(GL_TEXTURE_2D, texture->texid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

void R_LoadTextureMipLevel(LPCTEXTURE pTexture, DWORD level, LPCCOLOR32 pPixels, DWORD width, DWORD height) {
    if (width == 0 || height == 0)
        return;
    glBindTexture(GL_TEXTURE_2D, pTexture->texid);
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
    if (level > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
}
