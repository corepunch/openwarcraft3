#include "r_local.h"

enum tBLPEncoding
{
    BLP_ENCODING_UNCOMPRESSED = 1,
    BLP_ENCODING_DXT = 2,
    BLP_ENCODING_UNCOMPRESSED_RAW_BGRA = 3
};

enum tBLPAlphaDepth
{
    BLP_ALPHA_DEPTH_0 = 0,
    BLP_ALPHA_DEPTH_1 = 1,
    BLP_ALPHA_DEPTH_4 = 4,
    BLP_ALPHA_DEPTH_8 = 8,
};

enum tBLPAlphaEncoding
{
    BLP_ALPHA_ENCODING_DXT1 = 0,
    BLP_ALPHA_ENCODING_DXT3 = 1,
    BLP_ALPHA_ENCODING_DXT5 = 7,
};

enum tBLPFormat {
    BLP_FORMAT_JPEG = 0,

    BLP_FORMAT_PALETTED_NO_ALPHA = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_0 << 8),
    BLP_FORMAT_PALETTED_ALPHA_1  = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_1 << 8),
    BLP_FORMAT_PALETTED_ALPHA_4  = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_4 << 8),
    BLP_FORMAT_PALETTED_ALPHA_8  = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_8 << 8),

    BLP_FORMAT_RAW_BGRA = (BLP_ENCODING_UNCOMPRESSED_RAW_BGRA << 16),

    BLP_FORMAT_DXT1_NO_ALPHA = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_0 << 8) | BLP_ALPHA_ENCODING_DXT1,
    BLP_FORMAT_DXT1_ALPHA_1  = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_1 << 8) | BLP_ALPHA_ENCODING_DXT1,
    BLP_FORMAT_DXT3_ALPHA_4  = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_4 << 8) | BLP_ALPHA_ENCODING_DXT3,
    BLP_FORMAT_DXT3_ALPHA_8  = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_8 << 8) | BLP_ALPHA_ENCODING_DXT3,
    BLP_FORMAT_DXT5_ALPHA_8  = (BLP_ENCODING_DXT << 16) | (BLP_ALPHA_DEPTH_8 << 8) | BLP_ALPHA_ENCODING_DXT5,
};

// A description of the BLP2 format can be found on Wikipedia: http://en.wikipedia.org/wiki/.BLP
struct tBLP2Header {
    DWORD    magic;
    DWORD    type;           // 0: JPEG, 1: see encoding
    BYTE     encoding;       // 1: Uncompressed, 2: DXT compression, 3: Uncompressed BGRA
    BYTE     alphaDepth;     // 0, 1, 4 or 8 bits
    BYTE     alphaEncoding;  // 0: DXT1, 1: DXT3, 7: DXT5

    union {
        BYTE     hasMipLevels;   // In BLP file: 0 or 1
        BYTE     nbMipLevels;    // For convenience, replaced with the number of mip levels
    };

    DWORD    width;          // In pixels, power-of-two
    DWORD    height;
    DWORD    offsets[16];
    DWORD    lengths[16];
    COLOR32 palette[256];   // 256 BGRA colors
};

LPCOLOR32 blp2_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_alpha1(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_alpha4(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_alpha8(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_raw_bgra(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_dxt(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height, int flags);

DWORD blp2_width(struct tBLP2Header* pBLPInfos, DWORD mipLevel) {
    // Check the mip level
    if (mipLevel >= pBLPInfos->nbMipLevels)
        mipLevel = pBLPInfos->nbMipLevels - 1;
    return (pBLPInfos->width >> mipLevel);
}


DWORD blp2_height(struct tBLP2Header* pBLPInfos, DWORD mipLevel) {
    // Check the mip level
    if (mipLevel >= pBLPInfos->nbMipLevels)
        mipLevel = pBLPInfos->nbMipLevels - 1;
    return (pBLPInfos->height >> mipLevel);
}

DWORD blp2_nbMipLevels(struct tBLP2Header* pBLPInfos) {
    return pBLPInfos->nbMipLevels;
}

enum tBLPFormat blp2_format(struct tBLP2Header* pBLPInfos) {
    if (pBLPInfos->type == 0)
        return BLP_FORMAT_JPEG;

    if (pBLPInfos->encoding == BLP_ENCODING_UNCOMPRESSED)
        return (pBLPInfos->encoding << 16) | (pBLPInfos->alphaDepth << 8);

    if (pBLPInfos->encoding == BLP_ENCODING_UNCOMPRESSED_RAW_BGRA)
        return (pBLPInfos->encoding << 16);

    return (pBLPInfos->encoding << 16) | (pBLPInfos->alphaDepth << 8) | pBLPInfos->alphaEncoding;
}

LPCOLOR32 blp2_convert(HANDLE buffer, DWORD filesize, struct tBLP2Header* pBLPInfos, DWORD mipLevel) {
    // Check the mip level
    if (mipLevel >= pBLPInfos->nbMipLevels)
        mipLevel = pBLPInfos->nbMipLevels - 1;

    // Declarations
    DWORD width  = blp2_width(pBLPInfos, mipLevel);
    DWORD height = blp2_height(pBLPInfos, mipLevel);
    LPCOLOR32 pDst = 0;
    DWORD offset = pBLPInfos->offsets[mipLevel];
    DWORD size   = pBLPInfos->lengths[mipLevel];
    BYTE *pSrc = ri.MemAlloc(size);

    memcpy(pSrc, buffer + offset, size);

    switch (blp2_format(pBLPInfos)) {
        case BLP_FORMAT_JPEG:
            pDst = blp2_convert_paletted_no_alpha(pSrc, pBLPInfos, width, height);
            break;
        case BLP_FORMAT_PALETTED_NO_ALPHA:
            pDst = blp2_convert_paletted_no_alpha(pSrc, pBLPInfos, width, height);
            break;
        case BLP_FORMAT_PALETTED_ALPHA_1:
            pDst = blp2_convert_paletted_alpha1(pSrc, pBLPInfos, width, height);
            break;
        case BLP_FORMAT_PALETTED_ALPHA_4:
            pDst = blp2_convert_paletted_alpha4(pSrc, pBLPInfos, width, height);
            break;
        case BLP_FORMAT_PALETTED_ALPHA_8:
            pDst = blp2_convert_paletted_alpha8(pSrc, pBLPInfos, width, height);
            break;
//        case BLP_FORMAT_RAW_BGRA: pDst = blp2_convert_raw_bgra(pSrc, pBLPInfos, width, height); break;
//        case BLP_FORMAT_DXT1_NO_ALPHA:
//        case BLP_FORMAT_DXT1_ALPHA_1: pDst = blp2_convert_dxt(pSrc, pBLPInfos, width, height, squish::kDxt1); break;
//        case BLP_FORMAT_DXT3_ALPHA_4:
//        case BLP_FORMAT_DXT3_ALPHA_8: pDst = blp2_convert_dxt(pSrc, pBLPInfos, width, height, squish::kDxt3); break;
//        case BLP_FORMAT_DXT5_ALPHA_8: pDst = blp2_convert_dxt(pSrc, pBLPInfos, width, height, squish::kDxt5); break;
        default:
            assert(0);
            break;
    }

    ri.MemFree(pSrc);

    return pDst;
}


LPTEXTURE R_LoadTextureBLP2(HANDLE data, DWORD filesize) {
    struct tBLP2Header* pBLPInfos = ri.MemAlloc(sizeof(struct tBLP2Header));
    memcpy(pBLPInfos, data, sizeof(struct tBLP2Header));
    pBLPInfos->nbMipLevels = 0;
    while ((pBLPInfos->offsets[pBLPInfos->nbMipLevels] != 0) &&
           (pBLPInfos->nbMipLevels < 16))
    {
        ++pBLPInfos->nbMipLevels;
    }
    LPTEXTURE pTexture = R_AllocateTexture(blp2_width(pBLPInfos, 0), blp2_height(pBLPInfos, 0));
    FOR_LOOP(level, blp2_nbMipLevels(pBLPInfos)) {
        DWORD const width = blp2_width(pBLPInfos, level);
        DWORD const height = blp2_height(pBLPInfos, level);
        LPCOLOR32 pPixels = blp2_convert(data, filesize, pBLPInfos, level);
        if (pPixels) {
            R_LoadTextureMipLevel(pTexture, level, pPixels, width, height);
            ri.MemFree(pPixels);
        }
    }
    return pTexture;
}

LPCOLOR32 blp2_convert_paletted_alpha8(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
    LPCOLOR32 pDst = pBuffer;
    BYTE* pIndices = pSrc;
    BYTE* pAlpha = pSrc + width * height;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pHeader->palette[*pIndices];
            pDst->a = *pAlpha;
            ++pIndices;
            ++pAlpha;
            ++pDst;
        }
    }
    return pBuffer;
}

LPCOLOR32 blp2_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
    LPCOLOR32 pDst = pBuffer;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pHeader->palette[*pSrc];
            pDst->a = 0xFF;
            ++pSrc;
            ++pDst;
        }
    }
    return pBuffer;
}

LPCOLOR32 blp2_convert_paletted_alpha1(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
    LPCOLOR32 pDst = pBuffer;
    BYTE* pIndices = pSrc;
    BYTE* pAlpha = pSrc + width * height;
    BYTE counter = 0;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pHeader->palette[*pIndices];
            pDst->a = (*pAlpha & (1 << counter) ? 0xFF : 0x00);
            ++pIndices;
            ++pDst;
            ++counter;
            if (counter == 8) {
                ++pAlpha;
                counter = 0;
            }
        }
    }

    return pBuffer;
}

LPCOLOR32 blp2_convert_paletted_alpha4(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
    LPCOLOR32 pDst = pBuffer;
    BYTE* pIndices = pSrc;
    BYTE* pAlpha = pSrc + width * height;
    BYTE counter = 0;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pHeader->palette[*pIndices];
            pDst->a = (*pAlpha >> counter) & 0xF;
            // convert 4-bit range to 8-bit range
            pDst->a = (pDst->a << 4) | pDst->a;
            ++pIndices;
            ++pDst;
            counter += 4;
            if (counter == 8) {
                ++pAlpha;
                counter = 0;
            }
        }
    }

    return pBuffer;
}
