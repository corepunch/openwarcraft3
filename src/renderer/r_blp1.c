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

enum tBLPFormat
{
    BLP_FORMAT_JPEG = 0,

    BLP_FORMAT_PALETTED_NO_ALPHA = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_0 << 8),
    BLP_FORMAT_PALETTED_ALPHA_1  = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_1 << 8),
    BLP_FORMAT_PALETTED_ALPHA_4  = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_4 << 8),
    BLP_FORMAT_PALETTED_ALPHA_8  = (BLP_ENCODING_UNCOMPRESSED << 16) | (BLP_ALPHA_DEPTH_8 << 8),
};

// A description of the BLP1 format can be found in the file doc/MagosBformat.txt
struct tBLP1Header
{
    DWORD    magic;
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


// Internal representation of any BLP header
struct tInternalBLPInfos {
    struct tBLP1Header header;
    struct tBLP1Infos  infos;
};

LPCOLOR32 blp1_convert_jpeg(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD size);
LPCOLOR32 blp1_convert_paletted_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height);
LPCOLOR32 blp1_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height);
LPCOLOR32 blp1_convert_paletted_separated_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height);

//struct tInternalBLPInfos *blp_processFile(FILE* pFile);
//void blp_release(tBLPInfos binfos);
//
//BYTE blp_version(tBLPInfos binfos);
//tBLPFormat blp_format(tBLPInfos binfos);
//
//DWORD blp_width(tBLPInfos binfos, DWORD mipLevel = 0);
//DWORD blp_height(tBLPInfos binfos, DWORD mipLevel = 0);
//DWORD blp_nbMipLevels(tBLPInfos binfos);
//
//color32* blp_convert(FILE* pFile, tBLPInfos binfos, DWORD mipLevel = 0);


void blp1_release(struct tInternalBLPInfos* pBLPInfos) {
    if (pBLPInfos->header.type == 0)
        ri.MemFree(pBLPInfos->infos.jpeg.header);
    ri.MemFree(pBLPInfos);
}

enum tBLPFormat blp1_format(struct tInternalBLPInfos* pBLPInfos) {
    if (pBLPInfos->header.type == 0)
        return BLP_FORMAT_JPEG;
    if ((pBLPInfos->header.flags & 0x8) != 0)
        return BLP_FORMAT_PALETTED_ALPHA_8;
    return BLP_FORMAT_PALETTED_NO_ALPHA;
}


DWORD blp1_width(struct tInternalBLPInfos* pBLPInfos, DWORD mipLevel) {
    // Check the mip level
    if (mipLevel >= pBLPInfos->infos.nbMipLevels)
        mipLevel = pBLPInfos->infos.nbMipLevels - 1;
    return (pBLPInfos->header.width >> mipLevel);
}


DWORD blp1_height(struct tInternalBLPInfos* pBLPInfos, DWORD mipLevel) {
    // Check the mip level
    if (mipLevel >= pBLPInfos->infos.nbMipLevels)
        mipLevel = pBLPInfos->infos.nbMipLevels - 1;
    return (pBLPInfos->header.height >> mipLevel);
}


DWORD blp1_nbMipLevels(struct tInternalBLPInfos* pBLPInfos) {
    return pBLPInfos->infos.nbMipLevels;
}

LPCOLOR32 blp1_convert(HANDLE buffer, DWORD filesize, struct tInternalBLPInfos* pBLPInfos, DWORD mipLevel) {
    // Check the mip level
    if (mipLevel >= pBLPInfos->infos.nbMipLevels)
        mipLevel = pBLPInfos->infos.nbMipLevels - 1;
    // Declarations
    DWORD width  = blp1_width(pBLPInfos, mipLevel);
    DWORD height = blp1_height(pBLPInfos, mipLevel);
    LPCOLOR32 pDst = 0;
    DWORD offset = pBLPInfos->header.offsets[mipLevel];
    DWORD size   = pBLPInfos->header.lengths[mipLevel];
    BYTE* pSrc = ri.MemAlloc(size);
    memcpy(pSrc, buffer + offset, size);
    switch (blp1_format(pBLPInfos)) {
        case BLP_FORMAT_JPEG:
            pDst = blp1_convert_jpeg(pSrc, &pBLPInfos->infos, size);
            break;
        case BLP_FORMAT_PALETTED_NO_ALPHA:
            pDst = blp1_convert_paletted_no_alpha(pSrc, &pBLPInfos->infos, width, height);
            break;
        case BLP_FORMAT_PALETTED_ALPHA_8:
            if (pBLPInfos->header.alphaEncoding == 5) {
                pDst = blp1_convert_paletted_alpha(pSrc, &pBLPInfos->infos, width, height);
            } else {
                pDst = blp1_convert_paletted_separated_alpha(pSrc, &pBLPInfos->infos, width, height);
            }
            break;
        default:
            assert(0);
            break;
    }

    ri.MemFree(pSrc);

    return pDst;
}

LPTEXTURE R_LoadTextureBLP1(HANDLE data, DWORD filesize) {
    struct tInternalBLPInfos* pBLPInfos = ri.MemAlloc(sizeof(struct tInternalBLPInfos));
    memcpy(&pBLPInfos->header, data, sizeof(struct tBLP1Header));
    pBLPInfos->infos.nbMipLevels = 0;
    while ((pBLPInfos->header.offsets[pBLPInfos->infos.nbMipLevels] != 0) &&
           (pBLPInfos->infos.nbMipLevels < 16))
    {
        ++pBLPInfos->infos.nbMipLevels;
    }
    if (pBLPInfos->header.type == 0) {
        memcpy(&pBLPInfos->infos.jpeg.headerSize, data + sizeof(struct tBLP1Header), sizeof(DWORD));
        if (pBLPInfos->infos.jpeg.headerSize > 0) {
            pBLPInfos->infos.jpeg.header = ri.MemAlloc(pBLPInfos->infos.jpeg.headerSize);
            memcpy(pBLPInfos->infos.jpeg.header, data + sizeof(struct tBLP1Header) + sizeof(DWORD), pBLPInfos->infos.jpeg.headerSize);
        } else {
            pBLPInfos->infos.jpeg.header = 0;
        }
    } else {
        memcpy(&pBLPInfos->infos.palette, data + sizeof(struct tBLP1Header), sizeof(pBLPInfos->infos.palette));
    }

    LPTEXTURE pTexture = R_AllocateTexture(blp1_width(pBLPInfos, 0), blp1_height(pBLPInfos, 0));

    FOR_LOOP(level, blp1_nbMipLevels(pBLPInfos)) {
        DWORD const width = blp1_width(pBLPInfos, level);
        DWORD const height = blp1_height(pBLPInfos, level);
        LPCOLOR32 pPixels = blp1_convert(data, filesize, pBLPInfos, level);
        if (pPixels) {
            R_LoadTextureMipLevel(pTexture, level, pPixels, width, height);
            ri.MemFree(pPixels);
        }
    }

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

LPCOLOR32 blp1_convert_jpeg(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD dataSize) {
    BYTE* pSrcBuffer = ri.MemAlloc(pInfos->jpeg.headerSize + dataSize);

    memcpy(pSrcBuffer, pInfos->jpeg.header, pInfos->jpeg.headerSize);
    memcpy(pSrcBuffer + pInfos->jpeg.headerSize, pSrc, dataSize);

    struct jpeg_imageinfo const image = jpeg_readimage(pSrcBuffer, pInfos->jpeg.headerSize + dataSize);

    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof (COLOR32) * image.width * image.height);

    for (DWORD p = 0; p < image.width * image.height; ++p){
        LPCCOLOR32 c = (LPCCOLOR32)&image.data[p * image.num_components];
        pBuffer[p] = *c;
        if (image.num_components != 4) {
            pBuffer[p].a = 0xff;
        }
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
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
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

LPCOLOR32 blp1_convert_paletted_no_alpha(BYTE* pSrc, struct tBLP1Infos* pInfos, DWORD width, DWORD height) {
    LPCOLOR32 pBuffer = ri.MemAlloc(sizeof(COLOR32) * width * height);
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
