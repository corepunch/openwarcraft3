#include "wc3_save.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct { DWORD checksum, commit; } SAVEFOOTER;
typedef struct { DWORD magic, version, size; } RECORDHEADER;
static DWORD const record_commit = MAKEFOURCC('W', '3', 'O', 'K');

BOOL save_bytes(FILE *f, LPCVOID data, size_t size) { return fwrite(data, 1, size, f) == size; }
BOOL load_bytes(FILE *f, void *data, size_t size) { return fread(data, 1, size, f) == size; }

DWORD save_hash(DWORD hash, LPCVOID data, size_t size) {
    BYTE const *bytes = data;
    while (size--) hash = (hash ^ *bytes++) * 16777619u;
    return hash;
}

/* A committed checksum rejects truncation and corruption before ReadGame mutates live state. */
BOOL save_footer(FILE *f) {
    BYTE bytes[4096];
    long payload;
    DWORD checksum = 2166136261u;
    SAVEFOOTER footer;
    if (fflush(f) || (payload = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) return false;
    while (payload > 0) {
        size_t size = MIN((size_t)payload, sizeof(bytes));
        if (fread(bytes, 1, size, f) != size) return false;
        checksum = save_hash(checksum, bytes, size); payload -= (long)size;
    }
    footer = (SAVEFOOTER){ .checksum = checksum, .commit = record_commit };
    return fseek(f, 0, SEEK_END) == 0 && save_bytes(f, &footer, sizeof(footer));
}

BOOL load_footer(FILE *f) {
    BYTE bytes[4096];
    long payload;
    DWORD checksum = 2166136261u;
    SAVEFOOTER footer;
    if (fseek(f, 0, SEEK_END) || (payload = ftell(f)) < (long)sizeof(footer)) return false;
    payload -= sizeof(footer);
    if (fseek(f, payload, SEEK_SET) || !load_bytes(f, &footer, sizeof(footer)) || footer.commit != record_commit ||
        fseek(f, 0, SEEK_SET)) return false;
    for (long remaining = payload; remaining > 0;) {
        size_t size = MIN((size_t)remaining, sizeof(bytes));
        if (fread(bytes, 1, size, f) != size) return false;
        checksum = save_hash(checksum, bytes, size); remaining -= (long)size;
    }
    return checksum == footer.checksum && fseek(f, 0, SEEK_SET) == 0;
}

static BOOL save_io(LPSAVEIO io, void *data, size_t size) {
    return io->reading ? load_bytes(io->file, data, size) : save_bytes(io->file, data, size);
}

/* Walk special fields and counted records; pointer-free value blocks retain their native layout. */
BOOL save_fields(LPSAVEIO io, field_t const *fields, void *object) {
    BYTE *base = object;
    for (field_t const *f = fields; f->name; f++) {
        DWORD count = f->array_size ? f->array_size : 1;
        size_t size = f->array_size ? f->size / f->array_size : f->size;
        BYTE *ptr = base + f->ofs;
        if (f->type == F_IGNORE) continue;
        if (f->count_ofs != UINT32_MAX) {
            if (!io->reading) count = *(DWORD *)(base + f->count_ofs);
            if ((!io->reading && count > f->array_size) || !save_io(io, &count, sizeof(count)) || count > f->array_size)
                goto fail;
            if (io->reading) *(DWORD *)(base + f->count_ofs) = count;
        }
        switch (f->type) {
        case F_STRUCT:
            FOR_LOOP(i, count)
                if (!save_fields(io, (field_t const *)f->flags, ptr + i * size)) goto fail;
            break;
        case F_STRUCT_RING: {
            SAVERING const *ring = (SAVERING const *)f->flags;
            DWORD read = io->reading ? 0 : *(DWORD *)(base + ring->read_ofs);
            count = io->reading ? 0 : *(DWORD *)(base + ring->write_ofs) - read;
            if ((!io->reading && count > f->array_size) || !save_io(io, &count, sizeof(count)) || count > f->array_size)
                goto fail;
            if (io->reading) {
                *(DWORD *)(base + ring->read_ofs) = 0;
                *(DWORD *)(base + ring->write_ofs) = count;
            }
            FOR_LOOP(i, count)
                if (!save_fields(io, ring->fields, ptr + ((read + i) % f->array_size) * size)) goto fail;
            break;
        }
        case F_STRING:
            if ((!io->reading && !memchr(ptr, 0, f->size)) || !save_io(io, ptr, f->size) || !memchr(ptr, 0, f->size))
                goto fail;
            break;
        case F_INT: case F_FLOAT: case F_VECTOR: case F_REGION: case F_ANGLEHACK: case F_BYTES:
            if (!save_io(io, ptr, size * count)) goto fail;
            break;
        default:
            if (!io->special || !io->special(io, f, base)) goto fail;
            break;
        }
        continue;
fail:
        fprintf(stderr, "WC3 %s: invalid or incomplete field %s\n", io->reading ? "load" : "save", f->name);
        return false;
    }
    return true;
}

/* Small persistent records share the save checksum and stage changes before replacing the committed file. */
BOOL save_record(LPCSTR path, LPCSAVERECORD record, void *data) {
    if (record->valid && !record->valid(data)) {
        fprintf(stderr, "WC3 save: invalid record %s\n", path);
        return false;
    }
    PATHSTR tmp, backup;
    RECORDHEADER head = { .magic = record->magic, .version = record->version, .size = (DWORD)record->size };
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= sizeof(tmp) ||
        snprintf(backup, sizeof(backup), "%s.bak", path) >= sizeof(backup)) {
        fprintf(stderr, "WC3 save: record path is too long: %s\n", path);
        return false;
    }
    FILE *f = fopen(tmp, "w+b");
    if (!f) { fprintf(stderr, "WC3 save: cannot open %s: %s\n", tmp, strerror(errno)); return false; }
    SAVEIO io = { .file = f };
    BOOL ok = save_bytes(f, &head, sizeof(head)) && save_fields(&io, record->fields, data) && save_footer(f);
    if (fclose(f)) ok = false;
    if (!ok) goto fail;
    BOOL previous = false;
    if (rename(path, backup) == 0) previous = true;
    else if (errno != ENOENT) goto fail;
    if (rename(tmp, path)) {
        if (previous && rename(backup, path)) fprintf(stderr, "WC3 save: cannot restore backup %s: %s\n", backup, strerror(errno));
        goto fail;
    }
    if (previous && remove(backup)) fprintf(stderr, "WC3 save: cannot remove backup %s: %s\n", backup, strerror(errno));
    return true;
fail:
    fprintf(stderr, "WC3 save: failed to commit %s\n", path);
    remove(tmp);
    return false;
}

/* Validate into scratch storage: corrupt files must neither partially load nor destroy the caller's state. */
SAVERESULT load_record(LPCSTR path, LPCSAVERECORD record, void *data) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) return SAVE_MISSING;
        fprintf(stderr, "WC3 load: cannot open %s: %s\n", path, strerror(errno));
        return SAVE_INVALID;
    }
    RECORDHEADER head;
    void *temp = calloc(1, record->size);
    SAVEIO io = { .file = f, .reading = true };
    BOOL ok = temp && load_footer(f) && load_bytes(f, &head, sizeof(head)) && head.magic == record->magic &&
        head.version == record->version && head.size == record->size && save_fields(&io, record->fields, temp);
    long end = ftell(f);
    if (fseek(f, 0, SEEK_END) || end != ftell(f) - (long)sizeof(SAVEFOOTER)) ok = false;
    if (ok && record->valid) ok = record->valid(temp);
    if (ok) memcpy(data, temp, record->size);
    else fprintf(stderr, "WC3 load: invalid, incompatible or incomplete record %s\n", path);
    free(temp); fclose(f);
    return ok ? SAVE_LOADED : SAVE_INVALID;
}
