#include "r_local.h"

static void R_FileReadShadowMap(HANDLE hMpq, LPTERRAIN  pWorld) {
    HANDLE hFile;
    SFileOpenFileEx(hMpq, "war3map.shd", SFILE_OPEN_FROM_MPQ, &hFile);
    int const w = (pWorld->size.width - 1) * 4;
    int const h = (pWorld->size.height - 1) * 4;
    LPSTR lpShadows = MemAlloc(w * h);
    SFileReadFile(hFile, lpShadows, w * h, NULL, NULL);
    LPTEXTURE pShadowmap = R_AllocateTexture(w, h);
    struct color32 *lpPixels = MemAlloc(w * h * sizeof(struct color32));
    FOR_LOOP(i, w * h) {
        lpPixels[i].r = 255-lpShadows[i];
        lpPixels[i].g = 255-lpShadows[i];
        lpPixels[i].b = 255-lpShadows[i];
        lpPixels[i].a = 255;
    }
    R_LoadTextureMipLevel(pShadowmap, 0, lpPixels, w, h);
    SFileCloseFile(hFile);
    
    tr.shadowmap = pShadowmap;
}

void R_RegisterMap(char const *szMapFilename) {
    HANDLE hMpq;
    FS_ExtractFile(szMapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &hMpq);
    
    LPTERRAIN  pWorld = FileReadTerrain(hMpq);

    R_FileReadShadowMap(hMpq, pWorld);
    
    SFileCloseArchive(hMpq);
    
    tr.world = pWorld;
}
