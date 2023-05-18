#include "r_local.h"

#include <jpeglib.h>
#include <jerror.h>

// Opaque type representing a BLP file
typedef void* tBLPInfos;

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

enum tBLPFormat
{
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

// A description of the BLP1 format can be found in the file doc/MagosBlpFormat.txt
struct tBLP1Header
{
    DWORD    type;           // 0: JPEG, 1: palette
    DWORD    flags;          // 8: Alpha
    DWORD    width;          // In pixels, power-of-two
    DWORD    height;
    DWORD    alphaEncoding;  // 3, 4: Alpha list, 5: Alpha from palette
    DWORD    flags2;         // Unused
    DWORD    offsets[16];
    DWORD    lengths[16];
};


// Additional informations about a BLP1 file
struct tBLP1Infos
{
    BYTE nbMipLevels;    // The number of mip levels

    union {
        COLOR32  palette[256];   // 256 BGRA colors

        struct {
            DWORD headerSize;
            BYTE* header;        // Shared between all mipmap levels
        } jpeg;
    };
};


// A description of the BLP2 format can be found on Wikipedia: http://en.wikipedia.org/wiki/.BLP
struct tBLP2Header
{
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


// Internal representation of any BLP header
struct tInternalBLPInfos
{
    BYTE version;     // 1 or 2

    union {
        struct {
            struct tBLP1Header header;
            struct tBLP1Infos  infos;
        } blp1;

        struct tBLP2Header blp2;
    };
};

LPCOLOR32 blp1_convert_jpeg(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD size);
LPCOLOR32 blp1_convert_paletted_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height);
LPCOLOR32 blp1_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height);
LPCOLOR32 blp1_convert_paletted_separated_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_alpha1(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_alpha4(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_paletted_alpha8(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_raw_bgra(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height);
LPCOLOR32 blp2_convert_dxt(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height, int flags);

//struct tInternalBLPInfos *blp_processFile(FILE* pFile);
//void blp_release(tBLPInfos blpInfos);
//
//BYTE blp_version(tBLPInfos blpInfos);
//tBLPFormat blp_format(tBLPInfos blpInfos);
//
//DWORD blp_width(tBLPInfos blpInfos, DWORD mipLevel = 0);
//DWORD blp_height(tBLPInfos blpInfos, DWORD mipLevel = 0);
//DWORD blp_nbMipLevels(tBLPInfos blpInfos);
//
//color32* blp_convert(FILE* pFile, tBLPInfos blpInfos, DWORD mipLevel = 0);


struct tInternalBLPInfos *blp_processFile(HANDLE* hFile) {
    struct tInternalBLPInfos* pBLPInfos = ri.MemAlloc(sizeof(struct tInternalBLPInfos));
    char magic[4];

    SFileReadFile(hFile, magic, 4, NULL, NULL);

    if (strncmp(magic, "BLP2", 4) == 0) {
        SFileReadFile(hFile, &pBLPInfos->blp2, sizeof(struct tBLP2Header), NULL, NULL);
        pBLPInfos->version = 2;
        pBLPInfos->blp2.nbMipLevels = 0;
        while ((pBLPInfos->blp2.offsets[pBLPInfos->blp2.nbMipLevels] != 0) &&
               (pBLPInfos->blp2.nbMipLevels < 16))
        {
            ++pBLPInfos->blp2.nbMipLevels;
        }
    } else if (strncmp(magic, "BLP1", 4) == 0) {
        SFileReadFile(hFile, &pBLPInfos->blp1, sizeof(struct tBLP1Header), NULL, NULL);
        pBLPInfos->version = 1;
        pBLPInfos->blp1.infos.nbMipLevels = 0;
        while ((pBLPInfos->blp1.header.offsets[pBLPInfos->blp1.infos.nbMipLevels] != 0) &&
               (pBLPInfos->blp1.infos.nbMipLevels < 16))
        {
            ++pBLPInfos->blp1.infos.nbMipLevels;
        }
        if (pBLPInfos->blp1.header.type == 0) {
            SFileReadFile(hFile, &pBLPInfos->blp1.infos.jpeg.headerSize, sizeof(DWORD), NULL, NULL);
            if (pBLPInfos->blp1.infos.jpeg.headerSize > 0) {
                pBLPInfos->blp1.infos.jpeg.header = ri.MemAlloc(pBLPInfos->blp1.infos.jpeg.headerSize);
                SFileReadFile(hFile, pBLPInfos->blp1.infos.jpeg.header, pBLPInfos->blp1.infos.jpeg.headerSize, NULL, NULL);
            } else {
                pBLPInfos->blp1.infos.jpeg.header = 0;
            }
        } else {
            SFileReadFile(hFile, &pBLPInfos->blp1.infos.palette, sizeof(pBLPInfos->blp1.infos.palette), NULL, NULL);
        }
    }
    else
    {
        ri.MemFree(pBLPInfos);
        return 0;
    }

    return pBLPInfos;
}

void blp_release(struct tInternalBLPInfos* pBLPInfos) {
    if ((pBLPInfos->version == 1) && (pBLPInfos->blp1.header.type == 0))
        ri.MemFree(pBLPInfos->blp1.infos.jpeg.header);

    ri.MemFree(pBLPInfos);
}


BYTE blp_version(struct tInternalBLPInfos* pBLPInfos)
{
    return pBLPInfos->version;
}


enum tBLPFormat blp_format(struct tInternalBLPInfos* pBLPInfos)
{
    if (pBLPInfos->version == 2)
    {
        if (pBLPInfos->blp2.type == 0)
            return BLP_FORMAT_JPEG;

        if (pBLPInfos->blp2.encoding == BLP_ENCODING_UNCOMPRESSED)
            return (pBLPInfos->blp2.encoding << 16) | (pBLPInfos->blp2.alphaDepth << 8);

        if (pBLPInfos->blp2.encoding == BLP_ENCODING_UNCOMPRESSED_RAW_BGRA)
            return (pBLPInfos->blp2.encoding << 16);

        return (pBLPInfos->blp2.encoding << 16) | (pBLPInfos->blp2.alphaDepth << 8) | pBLPInfos->blp2.alphaEncoding;
    }
    else
    {
        if (pBLPInfos->blp1.header.type == 0)
            return BLP_FORMAT_JPEG;

        if ((pBLPInfos->blp1.header.flags & 0x8) != 0)
            return BLP_FORMAT_PALETTED_ALPHA_8;

        return BLP_FORMAT_PALETTED_NO_ALPHA;
    }
}


DWORD blp_width(struct tInternalBLPInfos* pBLPInfos, DWORD mipLevel)
{
    if (pBLPInfos->version == 2)
    {
        // Check the mip level
        if (mipLevel >= pBLPInfos->blp2.nbMipLevels)
            mipLevel = pBLPInfos->blp2.nbMipLevels - 1;

        return (pBLPInfos->blp2.width >> mipLevel);
    }
    else
    {
        // Check the mip level
        if (mipLevel >= pBLPInfos->blp1.infos.nbMipLevels)
            mipLevel = pBLPInfos->blp1.infos.nbMipLevels - 1;

        return (pBLPInfos->blp1.header.width >> mipLevel);
    }
}


DWORD blp_height(struct tInternalBLPInfos* pBLPInfos, DWORD mipLevel)
{
    if (pBLPInfos->version == 2)
    {
        // Check the mip level
        if (mipLevel >= pBLPInfos->blp2.nbMipLevels)
            mipLevel = pBLPInfos->blp2.nbMipLevels - 1;

        return (pBLPInfos->blp2.height >> mipLevel);
    }
    else
    {
        // Check the mip level
        if (mipLevel >= pBLPInfos->blp1.infos.nbMipLevels)
            mipLevel = pBLPInfos->blp1.infos.nbMipLevels - 1;

        return (pBLPInfos->blp1.header.height >> mipLevel);
    }
}


DWORD blp_nbMipLevels(struct tInternalBLPInfos* pBLPInfos)
{
    if (pBLPInfos->version == 2)
        return pBLPInfos->blp2.nbMipLevels;
    else
        return pBLPInfos->blp1.infos.nbMipLevels;
}

LPCOLOR32 blp_convert(HANDLE* hFile, struct tInternalBLPInfos* pBLPInfos, DWORD mipLevel)
{
    // Check the mip level
    if (pBLPInfos->version == 2)
    {
        if (mipLevel >= pBLPInfos->blp2.nbMipLevels)
            mipLevel = pBLPInfos->blp2.nbMipLevels - 1;
    }
    else
    {
        if (mipLevel >= pBLPInfos->blp1.infos.nbMipLevels)
            mipLevel = pBLPInfos->blp1.infos.nbMipLevels - 1;
    }

    // Declarations
    DWORD width  = blp_width(pBLPInfos, mipLevel);
    DWORD height = blp_height(pBLPInfos, mipLevel);
    LPCOLOR32 pDst = 0;
    BYTE* pSrc = 0;
    DWORD offset;
    DWORD size;

    if (pBLPInfos->version == 2)
    {
        offset = pBLPInfos->blp2.offsets[mipLevel];
        size   = pBLPInfos->blp2.lengths[mipLevel];
    }
    else
    {
        offset = pBLPInfos->blp1.header.offsets[mipLevel];
        size   = pBLPInfos->blp1.header.lengths[mipLevel];

//        for (int i = 0; i < 8; i++) {
//            printf("%d %d\n", pBLPInfos->blp1.header.offsets[i],  pBLPInfos->blp1.header.lengths[i]);
//        }
    }

    pSrc = ri.MemAlloc(size);

    // Read the data from the file
    SFileSetFilePointer(hFile, offset, NULL, FILE_BEGIN);
    SFileReadFile(hFile, pSrc, size, NULL, NULL);

    switch (blp_format(pBLPInfos))
    {
        case BLP_FORMAT_JPEG:
            // if (pBLPInfos->version == 2)
            //     pDst = blp2_convert_paletted_no_alpha(pSrc, &pBLPInfos->blp2, width, height);
            // else
                pDst = blp1_convert_jpeg(pSrc, &pBLPInfos->blp1.infos, size);
//
//            if (pBLPInfos->blp1.header.alphaEncoding == 3) {
//                int a = 0;
//            }
            break;
            
        case BLP_FORMAT_PALETTED_NO_ALPHA:
            if (pBLPInfos->version == 2)
                pDst = blp2_convert_paletted_no_alpha(pSrc, &pBLPInfos->blp2, width, height);
            else
                pDst = blp1_convert_paletted_no_alpha(pSrc, &pBLPInfos->blp1.infos, width, height);
            break;

        case BLP_FORMAT_PALETTED_ALPHA_1:
            pDst = blp2_convert_paletted_alpha1(pSrc, &pBLPInfos->blp2, width, height);
            break;

        case BLP_FORMAT_PALETTED_ALPHA_4:
            pDst = blp2_convert_paletted_alpha4(pSrc, &pBLPInfos->blp2, width, height);
            break;

        case BLP_FORMAT_PALETTED_ALPHA_8:
            if (pBLPInfos->version == 2) {
                pDst = blp2_convert_paletted_alpha8(pSrc, &pBLPInfos->blp2, width, height);
            } else {
                if (pBLPInfos->blp1.header.alphaEncoding == 5) {
                    pDst = blp1_convert_paletted_alpha(pSrc, &pBLPInfos->blp1.infos, width, height);
                } else {
                    pDst = blp1_convert_paletted_separated_alpha(pSrc, &pBLPInfos->blp1.infos, width, height);
                }
            }
            break;

//        case BLP_FORMAT_RAW_BGRA: pDst = blp2_convert_raw_bgra(pSrc, &pBLPInfos->blp2, width, height); break;
//
//        case BLP_FORMAT_DXT1_NO_ALPHA:
//        case BLP_FORMAT_DXT1_ALPHA_1:      pDst = blp2_convert_dxt(pSrc, &pBLPInfos->blp2, width, height, squish::kDxt1); break;
//        case BLP_FORMAT_DXT3_ALPHA_4:
//        case BLP_FORMAT_DXT3_ALPHA_8:      pDst = blp2_convert_dxt(pSrc, &pBLPInfos->blp2, width, height, squish::kDxt3); break;
//        case BLP_FORMAT_DXT5_ALPHA_8:      pDst = blp2_convert_dxt(pSrc, &pBLPInfos->blp2, width, height, squish::kDxt5); break;
        default:
            assert(0);
            break;
    }

    ri.MemFree(pSrc);

    return pDst;
}

LPTEXTURE R_LoadTexture(LPCSTR szTextureFilename) {
    HANDLE hFile = ri.FileOpen(szTextureFilename);
//    DWORD dwFileSize = SFileGetFileSize(hFile, NULL);
//    HANDLE lpBuffer = ri.MemAlloc(dwFileSize);
//    SFileReadFile(hFile, lpBuffer, dwFileSize, NULL, NULL);

    struct tInternalBLPInfos *pBLPInfos = blp_processFile(hFile);
    if (!pBLPInfos)
        return NULL;

    LPTEXTURE pTexture = R_AllocateTexture(blp_width(pBLPInfos, 0), blp_height(pBLPInfos, 0));
    
    FOR_LOOP(dwLevel, blp_nbMipLevels(pBLPInfos)) {
        LPCOLOR32 pPixels = blp_convert(hFile, pBLPInfos, dwLevel);
        DWORD dwWidth = blp_width(pBLPInfos, dwLevel);
        DWORD dwHeight = blp_height(pBLPInfos, dwLevel);
        R_LoadTextureMipLevel(pTexture, dwLevel, pPixels, dwWidth, dwHeight);
        ri.MemFree(pPixels);
    }

    ri.FileClose(hFile);
    
    return pTexture;
}

struct jpeg_imageinfo {
    int width;
    int height;
    int channels;
    DWORD size;
    int num_components;
    BYTE *data;
};

static struct jpeg_imageinfo
jpeg_readimage(HANDLE buf, DWORD size) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, buf, size);
    jpeg_read_header(&cinfo, true);
    cinfo.out_color_space = JCS_YCCK;
    jpeg_start_decompress(&cinfo);
    struct jpeg_imageinfo image = (struct jpeg_imageinfo) {
        .width = cinfo.output_width,
        .height = cinfo.output_height,
        .channels = cinfo.num_components,
        .size = cinfo.output_width * cinfo.output_height * cinfo.num_components,
        .num_components = cinfo.num_components,
        .data = ri.MemAlloc(cinfo.output_width * cinfo.output_height * cinfo.num_components),
    };
    BYTE* p1 = image.data;
    BYTE** p2 = &p1;
    while (cinfo.output_scanline < image.height) {
        unsigned long const numlines = jpeg_read_scanlines(&cinfo, p2, 1);
        *p2 += numlines * image.channels * image.width;
    }
    jpeg_finish_decompress(&cinfo);
    return image;
}

LPCOLOR32 blp1_convert_jpeg(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD dwDataSize) {
    BYTE* pSrcBuffer = ri.MemAlloc(pInfos->jpeg.headerSize + dwDataSize);

    memcpy(pSrcBuffer, pInfos->jpeg.header, pInfos->jpeg.headerSize);
    memcpy(pSrcBuffer + pInfos->jpeg.headerSize, pSrc, dwDataSize);
    
    struct jpeg_imageinfo const image = jpeg_readimage(pSrcBuffer, pInfos->jpeg.headerSize + dwDataSize);
    
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof (COLOR32) * image.width * image.height);
    
    for (DWORD p = 0; p < image.width * image.height; ++p){
        BYTE const *c = &image.data[p * image.num_components];
        pBuffer[p].r = c[2];
        pBuffer[p].g = c[1];
        pBuffer[p].b = c[0];
        pBuffer[p].a = image.num_components == 4 ? c[3] : 0xff;
    }

    ri.MemFree(image.data);
    ri.MemFree(pSrcBuffer);

    return pBuffer;
}

LPCOLOR32 blp1_convert_paletted_separated_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
    LPCOLOR32 pDst = pBuffer;
    BYTE* pIndices = pSrc;
    BYTE* pAlpha = pSrc + width * height;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pInfos->palette[*pIndices];
            pDst->a = *pAlpha;
            ++pIndices;
            ++pAlpha;
            ++pDst;
        }
    }
    return pBuffer;
}

LPCOLOR32 blp1_convert_paletted_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(width * height);
    LPCOLOR32 pDst = pBuffer;
    BYTE* pIndices = pSrc;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pInfos->palette[*pIndices];
            pDst->a = 0xFF - pDst->a;
            ++pIndices;
            ++pDst;
        }
    }
    return pBuffer;
}

LPCOLOR32 blp2_convert_paletted_alpha8(BYTE* pSrc, struct tBLP2Header* pHeader, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(width * height);
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
    LPCOLOR32 pBuffer = ri.MemAlloc(width * height);
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
    LPCOLOR32 pBuffer = ri.MemAlloc(width * height);
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
    LPCOLOR32 pBuffer = ri.MemAlloc(width * height);
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

LPCOLOR32 blp1_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(width * height);
    LPCOLOR32 pDst = pBuffer;
    BYTE* pIndices = pSrc;
    FOR_LOOP(y, height) {
        FOR_LOOP(x, width) {
            *pDst = pInfos->palette[*pIndices];
            pDst->a = 0xFF;
            ++pIndices;
            ++pDst;
        }
    }
    return pBuffer;
}


