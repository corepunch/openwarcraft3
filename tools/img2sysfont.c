#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(char const *argv0) {
    fprintf(stderr, "usage: %s <input.pcx> <output.h> <symbol>\n", argv0);
}

static int read_file(char const *path, unsigned char **data, size_t *size) {
    FILE *file;
    long end;
    unsigned char *buffer;

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return 0;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fclose(file);
        return 0;
    }
    end = ftell(file);
    if (end < 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fclose(file);
        return 0;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fclose(file);
        return 0;
    }
    buffer = malloc((size_t)end);
    if (!buffer) {
        fprintf(stderr, "out of memory reading %s\n", path);
        fclose(file);
        return 0;
    }
    if (fread(buffer, 1, (size_t)end, file) != (size_t)end) {
        fprintf(stderr, "%s: short read\n", path);
        free(buffer);
        fclose(file);
        return 0;
    }
    fclose(file);
    *data = buffer;
    *size = (size_t)end;
    return 1;
}

static int valid_symbol(char const *symbol) {
    if (!symbol || (!isalpha((unsigned char)symbol[0]) && symbol[0] != '_')) {
        return 0;
    }
    for (size_t i = 1; symbol[i]; i++) {
        if (!isalnum((unsigned char)symbol[i]) && symbol[i] != '_') {
            return 0;
        }
    }
    return 1;
}

static void write_define_name(FILE *out, char const *symbol) {
    for (size_t i = 0; symbol[i]; i++) {
        unsigned char c = (unsigned char)symbol[i];

        fputc((int)(isalnum(c) ? toupper(c) : '_'), out);
    }
}

static int write_header(char const *path, char const *symbol, unsigned char const *data, size_t size) {
    FILE *out = fopen(path, "wb");

    if (!out) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return 0;
    }

    fprintf(out, "#ifndef ");
    write_define_name(out, symbol);
    fprintf(out, "_H\n#define ");
    write_define_name(out, symbol);
    fprintf(out, "_H\n\n");
    fprintf(out, "#define ");
    write_define_name(out, symbol);
    fprintf(out, "_SIZE %zu\n\n", size);
    fprintf(out, "static unsigned char const %s[] = {\n", symbol);
    for (size_t i = 0; i < size; i++) {
        if ((i % 12) == 0) {
            fprintf(out, "    ");
        }
        fprintf(out, "0x%02x", data[i]);
        if (i + 1 < size) {
            fprintf(out, ",");
        }
        fputc((i % 12) == 11 || i + 1 == size ? '\n' : ' ', out);
    }
    fprintf(out, "};\n\n#endif\n");
    fclose(out);
    return 1;
}

int main(int argc, char **argv) {
    unsigned char *data;
    size_t size;
    int ok;

    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }
    if (!valid_symbol(argv[3])) {
        fprintf(stderr, "invalid C symbol: %s\n", argv[3]);
        return 1;
    }
    if (!read_file(argv[1], &data, &size)) {
        return 1;
    }
    ok = write_header(argv[2], argv[3], data, size);
    free(data);
    return ok ? 0 : 1;
}
