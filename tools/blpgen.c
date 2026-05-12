/*
 * blpgen - deterministic BLP2 test texture generator
 *
 * Generates minimal BLP2 files for use as test fixtures in tests.mpq.
 * No third-party dependencies; outputs valid BLP2 that the runtime renderer
 * can load via the existing r_blp2.c path.
 *
 * Usage:
 *   blpgen solid        <w> <h> <RRGGBBAA> <out.blp>
 *   blpgen checker      <w> <h> <cell>     <out.blp>
 *   blpgen alpha_ring   <w> <h>            <out.blp>
 *   blpgen panel_border <w> <h> <border>   <out.blp>
 *   blpgen paletted     <w> <h> <cell>     <out.blp>
 *
 * All non-paletted presets use BLP2 BGRA (encoding=3, alphaDepth=8).
 * The "paletted" preset uses BLP2 paletted+alpha8 (encoding=1, alphaDepth=8)
 * to exercise the palette decode path in r_blp2.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* -------------------------------------------------------------------------
 * BLP2 on-disk header (148 bytes before the 1024-byte palette block)
 * Total header = 148 + 1024 = 1172 bytes
 * -------------------------------------------------------------------------*/
#define BLP2_MAGIC         0x32504c42u   /* "BLP2" little-endian */
#define BLP2_HEADER_SIZE   1172u         /* sizeof fixed header incl. palette */
#define BLP2_TYPE_RAW      1u
#define BLP2_ENC_PALETTED  1u
#define BLP2_ENC_BGRA      3u

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t type;
    uint8_t  encoding;
    uint8_t  alphaDepth;
    uint8_t  alphaEncoding;
    uint8_t  hasMipLevels;
    uint32_t width;
    uint32_t height;
    uint32_t offsets[16];
    uint32_t lengths[16];
    uint32_t palette[256];   /* BGRA */
} blp2_header_t;
#pragma pack(pop)

/* -------------------------------------------------------------------------
 * Pixel helpers
 * -------------------------------------------------------------------------*/
typedef struct { uint8_t b, g, r, a; } bgra_t;

static bgra_t bgra(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    bgra_t c; c.r = r; c.g = g; c.b = b; c.a = a;
    return c;
}

static int parse_color(const char *hex, bgra_t *out) {
    unsigned int r = 0xff, g = 0xff, b = 0xff, a = 0xff;
    if (sscanf(hex, "%02x%02x%02x%02x", &r, &g, &b, &a) < 3)
        return 0;
    *out = bgra((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    return 1;
}

/* -------------------------------------------------------------------------
 * Write BLP2 BGRA (encoding=3, alphaDepth=8)
 * pixels: w*h bgra_t values, top-left first
 * -------------------------------------------------------------------------*/
static int write_bgra_blp(const char *path, int w, int h, const bgra_t *pixels) {
    blp2_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic        = BLP2_MAGIC;
    hdr.type         = BLP2_TYPE_RAW;
    hdr.encoding     = BLP2_ENC_BGRA;
    hdr.alphaDepth   = 8;
    hdr.alphaEncoding= 0;
    hdr.hasMipLevels = 0;
    hdr.width        = (uint32_t)w;
    hdr.height       = (uint32_t)h;

    uint32_t data_size = (uint32_t)(w * h * 4);
    hdr.offsets[0] = BLP2_HEADER_SIZE;
    hdr.lengths[0] = data_size;

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "blpgen: cannot open %s for writing\n", path); return 0; }
    fwrite(&hdr, sizeof(hdr), 1, f);
    fwrite(pixels, 4, (size_t)(w * h), f);
    fclose(f);
    return 1;
}

/* -------------------------------------------------------------------------
 * Write BLP2 paletted + alpha8 (encoding=1, alphaDepth=8)
 * indices: w*h palette index bytes
 * alphas:  w*h alpha bytes
 * palette: 256 bgra_t colours
 * -------------------------------------------------------------------------*/
static int write_paletted_blp(const char *path, int w, int h,
                               const uint8_t *indices, const uint8_t *alphas,
                               const bgra_t *palette)
{
    blp2_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic        = BLP2_MAGIC;
    hdr.type         = BLP2_TYPE_RAW;
    hdr.encoding     = BLP2_ENC_PALETTED;
    hdr.alphaDepth   = 8;
    hdr.alphaEncoding= 0;
    hdr.hasMipLevels = 0;
    hdr.width        = (uint32_t)w;
    hdr.height       = (uint32_t)h;

    uint32_t n        = (uint32_t)(w * h);
    uint32_t data_size= n * 2u;  /* indices + alphas interleaved per BLP2 spec */
    hdr.offsets[0]    = BLP2_HEADER_SIZE;
    hdr.lengths[0]    = data_size;

    /* Copy palette into header as BGRA uint32 */
    for (int i = 0; i < 256; i++) {
        hdr.palette[i] = ((uint32_t)palette[i].b)
                       | ((uint32_t)palette[i].g << 8)
                       | ((uint32_t)palette[i].r << 16)
                       | ((uint32_t)palette[i].a << 24);
    }

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "blpgen: cannot open %s for writing\n", path); return 0; }
    fwrite(&hdr, sizeof(hdr), 1, f);
    /* BLP2 paletted: all indices first, then all alpha bytes */
    fwrite(indices, 1, n, f);
    fwrite(alphas,  1, n, f);
    fclose(f);
    return 1;
}

/* =========================================================================
 * Preset generators
 * =========================================================================*/

/* solid <w> <h> <RRGGBBAA> <out.blp> */
static int gen_solid(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: blpgen solid <w> <h> <RRGGBBAA> <out.blp>\n");
        return 1;
    }
    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    bgra_t col = bgra(255, 255, 255, 255);
    parse_color(argv[3], &col);
    const char *path = argv[4];

    if (w <= 0 || h <= 0) { fprintf(stderr, "blpgen: invalid dimensions\n"); return 1; }

    bgra_t *px = malloc((size_t)(w * h) * sizeof(bgra_t));
    if (!px) { fprintf(stderr, "blpgen: out of memory\n"); return 1; }
    for (int i = 0; i < w * h; i++) px[i] = col;
    int ok = write_bgra_blp(path, w, h, px);
    free(px);
    return ok ? 0 : 1;
}

/* checker <w> <h> <cell_size> <out.blp>
 * Alternates black(0,0,0,255) and white(255,255,255,255) cells. */
static int gen_checker(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: blpgen checker <w> <h> <cell_size> <out.blp>\n");
        return 1;
    }
    int w    = atoi(argv[1]);
    int h    = atoi(argv[2]);
    int cell = atoi(argv[3]);
    const char *path = argv[4];

    if (w <= 0 || h <= 0 || cell <= 0) { fprintf(stderr, "blpgen: invalid dimensions\n"); return 1; }

    bgra_t *px = malloc((size_t)(w * h) * sizeof(bgra_t));
    if (!px) { fprintf(stderr, "blpgen: out of memory\n"); return 1; }
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int tile = (x / cell + y / cell) & 1;
            px[y * w + x] = tile ? bgra(255, 255, 255, 255) : bgra(0, 0, 0, 255);
        }
    }
    int ok = write_bgra_blp(path, w, h, px);
    free(px);
    return ok ? 0 : 1;
}

/* alpha_ring <w> <h> <out.blp>
 * White opaque ring; transparent inside the ring, transparent outside. */
static int gen_alpha_ring(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: blpgen alpha_ring <w> <h> <out.blp>\n");
        return 1;
    }
    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    const char *path = argv[3];

    if (w <= 0 || h <= 0) { fprintf(stderr, "blpgen: invalid dimensions\n"); return 1; }

    bgra_t *px = malloc((size_t)(w * h) * sizeof(bgra_t));
    if (!px) { fprintf(stderr, "blpgen: out of memory\n"); return 1; }

    float cx = (w - 1) * 0.5f;
    float cy = (h - 1) * 0.5f;
    float r_out = (cx < cy ? cx : cy) * 0.9f;  /* outer radius */
    float r_in  = r_out * 0.5f;                 /* inner radius */

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float dx = x - cx;
            float dy = y - cy;
            float r  = sqrtf(dx * dx + dy * dy);
            uint8_t a = (r >= r_in && r <= r_out) ? 255 : 0;
            px[y * w + x] = bgra(255, 255, 255, a);
        }
    }
    int ok = write_bgra_blp(path, w, h, px);
    free(px);
    return ok ? 0 : 1;
}

/* panel_border <w> <h> <border_px> <out.blp>
 * Solid white border of <border_px> thickness; transparent interior.
 * Useful as a backdrop edge/corner atlas stand-in. */
static int gen_panel_border(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: blpgen panel_border <w> <h> <border_px> <out.blp>\n");
        return 1;
    }
    int w   = atoi(argv[1]);
    int h   = atoi(argv[2]);
    int brd = atoi(argv[3]);
    const char *path = argv[4];

    if (w <= 0 || h <= 0 || brd <= 0) { fprintf(stderr, "blpgen: invalid dimensions\n"); return 1; }

    bgra_t *px = malloc((size_t)(w * h) * sizeof(bgra_t));
    if (!px) { fprintf(stderr, "blpgen: out of memory\n"); return 1; }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int on_border = (x < brd || x >= w - brd || y < brd || y >= h - brd);
            px[y * w + x] = on_border ? bgra(255, 255, 255, 255) : bgra(0, 0, 0, 0);
        }
    }
    int ok = write_bgra_blp(path, w, h, px);
    free(px);
    return ok ? 0 : 1;
}

/* paletted <w> <h> <cell_size> <out.blp>
 * Two-colour checker using paletted+alpha8 encoding.
 * Exercises the r_blp2.c palette decode path. */
static int gen_paletted(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: blpgen paletted <w> <h> <cell_size> <out.blp>\n");
        return 1;
    }
    int w    = atoi(argv[1]);
    int h    = atoi(argv[2]);
    int cell = atoi(argv[3]);
    const char *path = argv[4];

    if (w <= 0 || h <= 0 || cell <= 0) { fprintf(stderr, "blpgen: invalid dimensions\n"); return 1; }

    /* Build a minimal 2-entry palette; rest are transparent black */
    bgra_t palette[256];
    memset(palette, 0, sizeof(palette));
    palette[0] = bgra(0,   0,   0,   255);  /* black */
    palette[1] = bgra(255, 255, 255, 255);  /* white */

    uint8_t *indices = malloc((size_t)(w * h));
    uint8_t *alphas  = malloc((size_t)(w * h));
    if (!indices || !alphas) {
        free(indices); free(alphas);
        fprintf(stderr, "blpgen: out of memory\n");
        return 1;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (x / cell + y / cell) & 1;
            indices[y * w + x] = (uint8_t)idx;
            alphas [y * w + x] = palette[idx].a;
        }
    }

    int ok = write_paletted_blp(path, w, h, indices, alphas, palette);
    free(indices);
    free(alphas);
    return ok ? 0 : 1;
}

/* =========================================================================
 * main
 * =========================================================================*/
static void usage(void) {
    fprintf(stderr,
        "blpgen - BLP2 test texture generator\n"
        "\n"
        "Usage:\n"
        "  blpgen solid        <w> <h> <RRGGBBAA> <out.blp>\n"
        "  blpgen checker      <w> <h> <cell>     <out.blp>\n"
        "  blpgen alpha_ring   <w> <h>             <out.blp>\n"
        "  blpgen panel_border <w> <h> <border>    <out.blp>\n"
        "  blpgen paletted     <w> <h> <cell>      <out.blp>\n"
        "\n"
        "Examples:\n"
        "  blpgen solid        1  1  ffffffff  solid_white.blp\n"
        "  blpgen checker      8  8  2         checker_8x8.blp\n"
        "  blpgen alpha_ring   16 16            alpha_ring_16x16.blp\n"
        "  blpgen panel_border 32 32 2         panel_border_32x32.blp\n"
        "  blpgen paletted     8  8  2         paletted_checker_8x8.blp\n"
    );
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }

    const char *cmd = argv[1];
    /* shift so each handler receives argv starting at its own first argument */
    int sub_argc = argc - 1;
    char **sub_argv = argv + 1;

    if (strcmp(cmd, "solid")        == 0) return gen_solid(sub_argc, sub_argv);
    if (strcmp(cmd, "checker")      == 0) return gen_checker(sub_argc, sub_argv);
    if (strcmp(cmd, "alpha_ring")   == 0) return gen_alpha_ring(sub_argc, sub_argv);
    if (strcmp(cmd, "panel_border") == 0) return gen_panel_border(sub_argc, sub_argv);
    if (strcmp(cmd, "paletted")     == 0) return gen_paletted(sub_argc, sub_argv);

    fprintf(stderr, "blpgen: unknown command '%s'\n\n", cmd);
    usage();
    return 1;
}
