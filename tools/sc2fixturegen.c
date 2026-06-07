/*
 * sc2fixturegen - deterministic StarCraft II map binary fixture generator.
 *
 * Generates the small terrain/pathing files that SC2_MapLoad already parses:
 * MapInfo, t3HeightMap, t3SyncHeightMap, t3CellFlags, t3SyncCliffLevel,
 * and t3TextureMasks.
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
    unsigned char data[32 + 8 * 6];
    memset(data, 0, sizeof(data));
    memcpy(data, "LFCT", 4);
    wr_u32le(data + 24, 8);
    wr_u32le(data + 28, 6);
    for (unsigned int i = 0; i < 8 * 6; i++) {
        data[32 + i] = (unsigned char)(0x10 + i);
    }
    return write_file(path, data, sizeof(data));
}

static int write_map_info(const char *path) {
    static const char name[] = "SC2 Tiny Fixture";
    unsigned char data[16 + sizeof(name) + 1 + 16];
    unsigned int offset = 16;

    memset(data, 0, sizeof(data));
    memcpy(data, "IpaM", 4);
    wr_u32le(data + 4, 33);
    wr_u32le(data + 8, 8);
    wr_u32le(data + 12, 6);
    memcpy(data + offset, name, sizeof(name));
    offset += (unsigned int)sizeof(name);
    offset++;
    wr_u32le(data + offset, 2);
    wr_u32le(data + offset + 4, 1);
    wr_u32le(data + offset + 8, 6);
    wr_u32le(data + offset + 12, 4);
    return write_file(path, data, sizeof(data));
}

static unsigned int playable_height(unsigned int x, unsigned int y) {
    static unsigned int const heights[4][5] = {
        {  0,  8,  8,  8,  8 },
        {  8,  8,  8,  8,  8 },
        {  8, 10, 10, 10, 10 },
        { 10, 10, 10, 10, 12 },
    };
    return heights[y][x];
}

static int write_height_map(const char *path) {
    unsigned char data[32 + 9 * 7 * 6];
    memset(data, 0, sizeof(data));
    memcpy(data, "HMAP", 4);
    wr_u32le(data + 4, 101);
    wr_u32le(data + 8, 9);
    wr_u32le(data + 12, 7);
    for (unsigned int y = 0; y < 7; y++) {
        for (unsigned int x = 0; x < 9; x++) {
            unsigned int i = x + y * 9;
            unsigned int h = 0;
            if (x >= 2 && x <= 6 && y >= 1 && y <= 4) {
                h = playable_height(x - 2, y - 1);
            }
            wr_u16le(data + 32 + i * 6, (uint16_t)(h + 1));
            wr_u16le(data + 32 + i * 6 + 2, 0);
            wr_u16le(data + 32 + i * 6 + 4, 0);
        }
    }
    return write_file(path, data, sizeof(data));
}

static int write_sync_height_map(const char *path) {
    unsigned char data[64 + 9 * 7 * 4];
    memset(data, 0, sizeof(data));
    memcpy(data, "SMAP", 4);
    wr_u32le(data + 4, 102);
    wr_u32le(data + 8, 9);
    wr_u32le(data + 12, 7);
    for (unsigned int i = 0; i < 9 * 7; i++) {
        wr_u16le(data + 64 + i * 4, 0);
        wr_u16le(data + 64 + i * 4 + 2, 0);
    }
    wr_u16le(data + 64 + (6 + 4 * 9) * 4, 128);
    return write_file(path, data, sizeof(data));
}

static int write_cliff_levels(const char *path) {
    unsigned char data[32 + 8 * 6 * 2];
    memset(data, 0, sizeof(data));
    memcpy(data, "CLIF", 4);
    wr_u32le(data + 8, 8);
    wr_u32le(data + 12, 6);
    for (unsigned int i = 0; i < 8 * 6; i++) {
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
        fprintf(stderr, "Usage: sc2fixturegen <map-info|height-map|sync-height-map|cell-flags|cliff-levels|texture-masks> <out>\n");
        return 1;
    }
    if (!strcmp(argv[1], "map-info")) {
        return write_map_info(argv[2]) ? 0 : 1;
    }
    if (!strcmp(argv[1], "cell-flags")) {
        return write_cell_flags(argv[2]) ? 0 : 1;
    }
    if (!strcmp(argv[1], "height-map")) {
        return write_height_map(argv[2]) ? 0 : 1;
    }
    if (!strcmp(argv[1], "sync-height-map")) {
        return write_sync_height_map(argv[2]) ? 0 : 1;
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
