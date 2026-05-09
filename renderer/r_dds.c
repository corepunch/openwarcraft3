#include "r_local.h"

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT  0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

LPCTEXTURE dds=NULL;

LPTEXTURE R_LoadTextureDDS(HANDLE data, DWORD filesize) {
    unsigned int blockSize;
    unsigned int format;
    BYTE const *buf = data;
    
    // extract height, width, and amount of mipmaps - yes it is stored height then width
    unsigned int headerSize = (buf[4]) | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
    unsigned int height = (buf[12]) | (buf[13] << 8) | (buf[14] << 16) | (buf[15] << 24);
    unsigned int width = (buf[16]) | (buf[17] << 8) | (buf[18] << 16) | (buf[19] << 24);
    unsigned int mipMapCount = (buf[28]) | (buf[29] << 8) | (buf[30] << 16) | (buf[31] << 24);
    
    // figure out what format to use for what fourCC file type it is
    // block size is about physical chunk storage of compressed data in file (important)
    if(buf[84] == 'D') {
        switch(buf[87]) {
            case '1': // DXT1
                format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                blockSize = 8;
                break;
            case '3': // DXT3
                format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                blockSize = 16;
                break;
            case '5': // DXT5
                format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                blockSize = 16;
                break;
            case '0': // DX10
                // unsupported, else will error
                // as it adds sizeof(struct DDS_HEADER_DXT10) between pixels
                // so, buffer = malloc((file_size - 128) - sizeof(struct DDS_HEADER_DXT10));
            default:
                fprintf(stderr, "DX10 unsupported\n");
                return NULL;
        }
    } else {
        fprintf(stderr, "BC4U/BC4S/ATI2/BC55/R8G8_B8G8/G8R8_G8B8/UYVY-packed/YUY2-packed unsupported\n");
        return NULL;
    }
    
    LPTEXTURE texture = ri.MemAlloc(sizeof(TEXTURE));
    
    R_Call(glGenTextures, 1, &texture->texid);
    R_Call(glBindTexture, GL_TEXTURE_2D, texture->texid);

    // make it complete by specifying all needed parameters and ensuring all mipmaps are filled
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount - 1); // opengl likes array length of mipmaps
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // don't forget to enable mipmaping
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // prepare some variables
    unsigned int offset = 0;
    unsigned int size = 0;
    unsigned int w = width;
    unsigned int h = height;
    
    // loop through sending block at a time with the magic formula
    // upload to opengl properly, note the offset transverses the pointer
    // assumes each mipmap is 1/2 the size of the previous mipmap
    for (unsigned int i=0; i<mipMapCount; i++) {
        if(w == 0 || h == 0) { // discard any odd mipmaps 0x1 0x2 resolutions
            mipMapCount--;
            continue;
        }
        size = ((w+3)/4) * ((h+3)/4) * blockSize;
        R_Call(glCompressedTexImage2D, GL_TEXTURE_2D, i, format, w, h, 0, size, buf + offset + (headerSize + 4));
        offset += size;
        w /= 2;
        h /= 2;
    }
    // discard any odd mipmaps, ensure a complete texture
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount-1);
    
    
    if (!dds) dds = texture;
    texture->width = width;
    texture->height = height;

    return texture;
}
