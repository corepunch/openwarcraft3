#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../renderer/stb/stb_image.h"

typedef struct {
    unsigned char *pixels;
    int width;
    int height;
} image_t;

static void usage(void) {
    fprintf(stderr,
            "Usage: img2sysfont <input.png|input.pcx> <output.h> <symbol> [cell_width cell_height]\n"
            "\n"
            "Converts a PNG or 8-bit PCX atlas into a C header containing RLE RGBA bytes.\n"
            "The atlas is expected to contain a 16-column character grid.\n");
}

static unsigned int read_le16(unsigned char const *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static int has_extension(char const *path, char const *ext) {
    size_t path_len = strlen(path);
    size_t ext_len = strlen(ext);

    if (path_len < ext_len) {
        return 0;
    }
    path += path_len - ext_len;
    for (size_t i = 0; i < ext_len; i++) {
        if (tolower((unsigned char)path[i]) != tolower((unsigned char)ext[i])) {
            return 0;
        }
    }
    return 1;
}

static unsigned char *read_file(char const *path, size_t *size_out) {
    FILE *fp;
    long size;
    unsigned char *data;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    if (size < 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fclose(fp);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fclose(fp);
        return NULL;
    }

    data = malloc((size_t)size);
    if (!data) {
        fprintf(stderr, "%s: out of memory\n", path);
        fclose(fp);
        return NULL;
    }
    if (fread(data, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "%s: read failed\n", path);
        free(data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    *size_out = (size_t)size;
    return data;
}

static int load_png(char const *path, image_t *image) {
    int channels;
    int count;
    unsigned char *loaded;

    loaded = stbi_load(path, &image->width, &image->height, &channels, 4);
    if (!loaded) {
        fprintf(stderr, "%s: %s\n", path, stbi_failure_reason());
        return 0;
    }

    count = image->width * image->height * 4;
    image->pixels = malloc((size_t)count);
    if (!image->pixels) {
        fprintf(stderr, "%s: out of memory\n", path);
        stbi_image_free(loaded);
        return 0;
    }
    memcpy(image->pixels, loaded, (size_t)count);
    stbi_image_free(loaded);
    return 1;
}

static int load_pcx(char const *path, image_t *image) {
    size_t file_size;
    size_t palette_pos;
    unsigned char const *src;
    unsigned char const *src_end;
    unsigned char *data;
    unsigned char *raw;
    unsigned char palette[256][3];
    int width;
    int height;
    int planes;
    int bits_per_pixel;
    int bytes_per_line;
    int raw_count;
    int out = 0;

    data = read_file(path, &file_size);
    if (!data) {
        return 0;
    }
    if (file_size < 128) {
        fprintf(stderr, "%s: truncated PCX\n", path);
        free(data);
        return 0;
    }

    width = (int)(read_le16(data + 8) - read_le16(data + 4) + 1);
    height = (int)(read_le16(data + 10) - read_le16(data + 6) + 1);
    planes = data[65];
    bits_per_pixel = data[3];
    bytes_per_line = (int)read_le16(data + 66);

    if (data[0] != 0x0a || data[2] != 1 || bits_per_pixel != 8 || planes != 1 ||
        width <= 0 || height <= 0 || bytes_per_line < width) {
        fprintf(stderr, "%s: unsupported PCX format\n", path);
        free(data);
        return 0;
    }

    for (int i = 0; i < 256; i++) {
        palette[i][0] = (unsigned char)i;
        palette[i][1] = (unsigned char)i;
        palette[i][2] = (unsigned char)i;
    }
    palette_pos = file_size;
    if (file_size >= 897 && data[file_size - 769] == 0x0c) {
        palette_pos = file_size - 769;
        memcpy(palette, data + palette_pos + 1, sizeof(palette));
    }

    raw_count = bytes_per_line * height;
    raw = malloc((size_t)raw_count);
    image->pixels = malloc((size_t)width * height * 4);
    if (!raw || !image->pixels) {
        fprintf(stderr, "%s: out of memory\n", path);
        free(raw);
        free(image->pixels);
        free(data);
        return 0;
    }

    src = data + 128;
    src_end = data + palette_pos;
    while (out < raw_count && src < src_end) {
        int run;
        int value;
        int b = *src++;

        if ((b & 0xc0) == 0xc0) {
            run = b & 0x3f;
            if (src >= src_end) {
                break;
            }
            value = *src++;
        } else {
            run = 1;
            value = b;
        }
        while (run-- && out < raw_count) {
            raw[out++] = (unsigned char)value;
        }
    }
    if (out < raw_count) {
        fprintf(stderr, "%s: truncated PCX image data\n", path);
        free(raw);
        free(image->pixels);
        free(data);
        return 0;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = raw[y * bytes_per_line + x];
            unsigned char *p = image->pixels + (y * width + x) * 4;

            p[0] = palette[index][0];
            p[1] = palette[index][1];
            p[2] = palette[index][2];
            p[3] = index == 255 ? 0 : 255;
        }
    }

    image->width = width;
    image->height = height;
    free(raw);
    free(data);
    return 1;
}

static int load_image(char const *path, image_t *image) {
    memset(image, 0, sizeof(*image));
    if (has_extension(path, ".pcx")) {
        return load_pcx(path, image);
    }
    return load_png(path, image);
}

static int parse_positive_int(char const *text, int *out) {
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno || !end || *end || value <= 0 || value > 65535) {
        return 0;
    }

    *out = (int)value;
    return 1;
}

static void make_macro_name(char *dst, size_t dst_size, char const *symbol, char const *suffix) {
    size_t i;
    size_t out = 0;

    for (i = 0; symbol[i] && out + 1 < dst_size; i++) {
        unsigned char ch = (unsigned char)symbol[i];
        dst[out++] = (char)(isalnum(ch) ? toupper(ch) : '_');
    }

    if (suffix) {
        for (i = 0; suffix[i] && out + 1 < dst_size; i++) {
            unsigned char ch = (unsigned char)suffix[i];
            dst[out++] = (char)(isalnum(ch) ? toupper(ch) : '_');
        }
    }

    dst[out] = 0;
}

static int same_pixel(unsigned char const *a, unsigned char const *b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

int main(int argc, char **argv) {
    char const *input;
    char const *output;
    char const *symbol;
    char guard[256];
    char macro[256];
    FILE *fp;
    image_t image;
    int cell_width = 16;
    int cell_height = 16;
    int cols;
    int rows;
    int count;
    int rle_size;
    int i;
    int line_runs;

    if (argc != 4 && argc != 6) {
        usage();
        return 1;
    }

    input = argv[1];
    output = argv[2];
    symbol = argv[3];

    if (argc == 6) {
        if (!parse_positive_int(argv[4], &cell_width) ||
            !parse_positive_int(argv[5], &cell_height)) {
            fprintf(stderr, "Invalid cell size\n");
            return 1;
        }
    }

    if (!load_image(input, &image)) {
        return 1;
    }

    if (image.width <= 0 || image.height <= 0 || image.width % cell_width || image.height % cell_height) {
        fprintf(stderr, "%s: image size %dx%d is not divisible by cell size %dx%d\n",
                input, image.width, image.height, cell_width, cell_height);
        free(image.pixels);
        return 1;
    }

    cols = image.width / cell_width;
    rows = image.height / cell_height;
    if (cols != 16) {
        fprintf(stderr, "%s: expected 16 columns, got %d\n", input, cols);
        free(image.pixels);
        return 1;
    }

    fp = fopen(output, "w");
    if (!fp) {
        fprintf(stderr, "%s: %s\n", output, strerror(errno));
        free(image.pixels);
        return 1;
    }

    make_macro_name(guard, sizeof(guard), symbol, "_h");

    fprintf(fp, "#ifndef %s\n", guard);
    fprintf(fp, "#define %s\n\n", guard);
    fprintf(fp, "/* Generated by tools/img2sysfont.c from %s. */\n", input);

    make_macro_name(macro, sizeof(macro), symbol, "_width");
    fprintf(fp, "#define %s %d\n", macro, image.width);
    make_macro_name(macro, sizeof(macro), symbol, "_height");
    fprintf(fp, "#define %s %d\n", macro, image.height);
    make_macro_name(macro, sizeof(macro), symbol, "_cols");
    fprintf(fp, "#define %s %d\n", macro, cols);
    make_macro_name(macro, sizeof(macro), symbol, "_rows");
    fprintf(fp, "#define %s %d\n", macro, rows);
    make_macro_name(macro, sizeof(macro), symbol, "_cell_width");
    fprintf(fp, "#define %s %d\n", macro, cell_width);
    make_macro_name(macro, sizeof(macro), symbol, "_cell_height");
    fprintf(fp, "#define %s %d\n\n", macro, cell_height);

    count = image.width * image.height;
    rle_size = 0;
    for (i = 0; i < count;) {
        int run = 1;
        while (i + run < count &&
               run < 65535 &&
               same_pixel(image.pixels + i * 4, image.pixels + (i + run) * 4)) {
            run++;
        }
        rle_size += 6;
        i += run;
    }

    make_macro_name(macro, sizeof(macro), symbol, "_rle_size");
    fprintf(fp, "#define %s %d\n\n", macro, rle_size);

    fprintf(fp, "static const unsigned char %s_rle[%d] = {\n", symbol, rle_size);
    line_runs = 0;
    for (i = 0; i < count;) {
        unsigned char const *p = image.pixels + i * 4;
        int run = 1;
        unsigned char bytes[6];
        int b;

        while (i + run < count &&
               run < 65535 &&
               same_pixel(p, image.pixels + (i + run) * 4)) {
            run++;
        }

        bytes[0] = (unsigned char)(run & 0xff);
        bytes[1] = (unsigned char)((run >> 8) & 0xff);
        bytes[2] = p[0];
        bytes[3] = p[1];
        bytes[4] = p[2];
        bytes[5] = p[3];

        if (line_runs == 0) {
            fprintf(fp, "    ");
        }

        for (b = 0; b < 6; b++) {
            fprintf(fp, "0x%02x", bytes[b]);
            if (i + run < count || b + 1 < 6) {
                fprintf(fp, ",");
            }
            if (b + 1 < 6) {
                fprintf(fp, " ");
            }
        }
        i += run;
        line_runs++;
        if (line_runs == 4 || i >= count) {
            fprintf(fp, "\n");
            line_runs = 0;
        } else {
            fprintf(fp, " ");
        }
    }
    fprintf(fp, "};\n\n");
    fprintf(fp, "#endif\n");

    if (fclose(fp) != 0) {
        fprintf(stderr, "%s: %s\n", output, strerror(errno));
        free(image.pixels);
        return 1;
    }

    free(image.pixels);
    return 0;
}
