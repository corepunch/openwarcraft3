#include "g_local.h"

#pragma pack (push, 1)
typedef struct {
    BYTE id_length, colormap_type, image_type;
    WORD colormap_index, colormap_length;
    BYTE colormap_size;
    WORD x_origin, y_origin, width, height;
    BYTE pixel_size, attributes;
} tgaHeader_t;
#pragma pack (pop)

#define TGA_ORIGIN_MASK 0x30

pathTex_t *LoadTGA(const BYTE* mem, size_t size) {
    tgaHeader_t *header = (tgaHeader_t*)mem;
    const BYTE *tga = mem + sizeof(tgaHeader_t) + header->id_length;
    DWORD columns = header->width;
    DWORD rows = header->height;
    DWORD numPixels = columns * rows;
    switch (header->image_type) {
        case 2:  break;
        case 3:  break;
//        case 10: break;
        default: return NULL;
    }
    switch (header->colormap_type) {
        case 0:  break;
        default: return NULL;
    }
    switch (header->pixel_size) {
        case 32: break;
        case 24: break;
        case 8:  break;
        default: return NULL;
    }
    // Allocate memory for decoded image
    pathTex_t *pathTex = gi.MemAlloc(numPixels * sizeof(COLOR32) + sizeof(pathTex_t));
    if (!pathTex)
        return NULL;
    pathTex->width = columns;
    pathTex->height = rows;
    // Uncompressed RGB image
    if (header->image_type==2 || header->image_type==3) {
        for (int row=rows-1; row>=0; row--) {
            for (int column=0; column<columns; column++) {
                LPCCOLOR32 pcolor = &pathTex->map[column + row * columns];
                LPBYTE dest = (LPBYTE)pcolor;
                BYTE value;
                switch (header->pixel_size) {
                    case 8:
                        value = *tga++;
                        *dest++ = value;
                        *dest++ = value;
                        *dest++ = value;
                        *dest++ = 0xff;
                        break;
                    case 24:
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = 0xff;
                        break;
                    case 32:
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        *dest++ = *(tga++);
                        break;
                }
            }
        }
        return pathTex;
    }
    gi.MemFree(pathTex);
    return NULL;
}
