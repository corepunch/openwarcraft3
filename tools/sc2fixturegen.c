/*
 * sc2fixturegen - deterministic StarCraft II map binary fixture generator.
 *
 * Generates the small terrain/pathing files that SC2_MapLoad already parses:
 * t3HeightMap, t3CellFlags, t3SyncCliffLevel, and t3TextureMasks.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void wr_u32le(unsigned char *p, uint32_t v) {
    p[0] = (unsigned char)(v & 0xff);
    p[1] = (unsigned char)((v >> 8) & 0xff);
    p[2] = (unsigned char)((v >> 16) & 0xff);
    p[3] = (unsigned char)((v >> 24) & 0xff);
}

static void wr_u16le(unsigned char *p, uint16_t v) {
    p[0] = (unsigned char)(v & 0xff);
    p[1] = (unsigned char)((v >> 8) & 0xff);
}

static int write_file(const char *path, const unsigned char *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "sc2fixturegen: cannot open %s\n", path);
        return 0;
    }
    if (fwrite(data, 1, size, f) != size) {
        fprintf(stderr, "sc2fixturegen: short write %s\n", path);
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

static int write_cell_flags(const char *path) {
    unsigned char data[32 + 12];
    memset(data, 0, sizeof(data));
    memcpy(data, "LFCT", 4);
    wr_u32le(data + 24, 4);
    wr_u32le(data + 28, 3);
    for (unsigned int i = 0; i < 12; i++) {
        data[32 + i] = (unsigned char)(0x10 + i);
    }
    return write_file(path, data, sizeof(data));
}

static int write_height_map(const char *path) {
    unsigned char data[32 + 5 * 4 * 6];
    memset(data, 0, sizeof(data));
    memcpy(data, "HMAP", 4);
    wr_u32le(data + 4, 101);
    wr_u32le(data + 8, 5);
    wr_u32le(data + 12, 4);
    for (unsigned int i = 0; i < 5 * 4; i++) {
        wr_u16le(data + 32 + i * 6, 10);
        wr_u16le(data + 32 + i * 6 + 2, (uint16_t)(4 + i));
        wr_u16le(data + 32 + i * 6 + 4, 0);
    }
    return write_file(path, data, sizeof(data));
}

static int write_cliff_levels(const char *path) {
    unsigned char data[32 + 12 * 2];
    memset(data, 0, sizeof(data));
    memcpy(data, "CLIF", 4);
    wr_u32le(data + 8, 4);
    wr_u32le(data + 12, 3);
    for (unsigned int i = 0; i < 12; i++) {
        wr_u16le(data + 32 + i * 2, (uint16_t)(i + 1));
    }
    return write_file(path, data, sizeof(data));
}

static int write_texture_masks(const char *path) {
    unsigned char data[64 + 8 * 2];
    memset(data, 0, sizeof(data));
    memcpy(data, "MASK", 4);
    wr_u32le(data + 12, 4);
    wr_u32le(data + 16, 4);
    memset(data + 64, 0x12, 8);
    memset(data + 64 + 8, 0xab, 8);
    return write_file(path, data, sizeof(data));
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: sc2fixturegen <height-map|cell-flags|cliff-levels|texture-masks> <out>\n");
        return 1;
    }
    if (!strcmp(argv[1], "cell-flags")) {
        return write_cell_flags(argv[2]) ? 0 : 1;
    }
    if (!strcmp(argv[1], "height-map")) {
        return write_height_map(argv[2]) ? 0 : 1;
    }
    if (!strcmp(argv[1], "cliff-levels")) {
        return write_cliff_levels(argv[2]) ? 0 : 1;
    }
    if (!strcmp(argv[1], "texture-masks")) {
        return write_texture_masks(argv[2]) ? 0 : 1;
    }
    fprintf(stderr, "sc2fixturegen: unknown fixture kind %s\n", argv[1]);
    return 1;
}
