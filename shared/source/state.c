#include "common/state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include "common/net_platform.h" /* Shields the engine's WinAPI-style typedefs from SDK declarations. */
#include <io.h>
#else
#include <unistd.h>
#endif

#define BZ_STATE_LIMIT (256u << 20) // bytes; bound untrusted file size and buffer growth

typedef struct { DWORD checksum, commit; } STATEFOOTER;
typedef struct { DWORD magic, version, size; } RECORDHEADER;
typedef struct stateslot_s {
    struct stateslot_s *next;
    char *path;
    STATELIFE life;
    DWORD id, magic, version;
    size_t size;
    void *data;
    BOOL committed;
} STATESLOT;
static STATESLOT *state_slots;
static DWORD state_serial;
static size_t state_bytes;
static BOOL state_in_restore;
static DWORD const state_marker = MAKEFOURCC('S', 'T', 'O', 'K');
static DWORD const slot_magic = MAKEFOURCC('S', 'L', 'O', 'T');

/* The envelope checks integrity; this header identifies the game and destination map. */
BOOL state_save_head(LPSTATEBUFFER buf, LPCSTATESAVE info) {
    STATESAVE head = { .magic = slot_magic, .version = 1 };
    if (!info->game[0] || !info->map[0] || !memchr(info->game, 0, sizeof(PATHSTR)) ||
        !memchr(info->map, 0, sizeof(PATHSTR))) return false;
    memcpy(head.game, info->game, sizeof(PATHSTR)); memcpy(head.map, info->map, sizeof(PATHSTR));
    return save_bytes(buf, &head, sizeof(head));
}

BOOL state_load_head(LPSTATEBUFFER buf, LPCSTR game, LPSTR map) {
    STATESAVE head;
    buf->pos = 0;
    if (!load_bytes(buf, &head, sizeof(head)) || head.magic != slot_magic || head.version != 1 ||
        !memchr(head.game, 0, sizeof(PATHSTR)) || strcmp(head.game, game) || !head.map[0] ||
        !memchr(head.map, 0, sizeof(PATHSTR))) {
        fprintf(stderr, "State: invalid or incompatible save-slot metadata\n"); return false;
    }
    memcpy(map, head.map, sizeof(PATHSTR));
    return true;
}

static BOOL state_is_import(LPSTATEBUFFER buf, STATEIMPORT const *old) {
    return old && buf->size >= sizeof(old->magic) && !memcmp(buf->data, old->magic, sizeof(old->magic));
}

DWORD save_hash(DWORD hash, LPCVOID data, size_t size) {
    BYTE const *bytes = data;
    while (size--) hash = (hash ^ *bytes++) * 16777619u;
    return hash;
}

/* Writes grow bounded scratch storage; gameplay never hands a file to a codec. */
BOOL save_bytes(LPSTATEBUFFER buf, LPCVOID data, size_t size) {
    if (buf->size > BZ_STATE_LIMIT || size > BZ_STATE_LIMIT - buf->size) return false;
    size_t end = buf->size + size;
    if (end > buf->capacity) {
        size_t cap = MAX(end, MIN(BZ_STATE_LIMIT, MAX(4096, buf->capacity * 2)));
        void *mem = realloc(buf->data, cap);
        if (!mem) return false;
        buf->data = mem; buf->capacity = cap;
    }
    if (size) memcpy(buf->data + buf->size, data, size);
    buf->size = end;
    return true;
}

BOOL load_bytes(LPSTATEBUFFER buf, void *data, size_t size) {
    if (buf->pos > buf->size || size > buf->size - buf->pos) return false;
    if (size) memcpy(data, buf->data + buf->pos, size);
    buf->pos += size;
    return true;
}

void state_free(LPSTATEBUFFER buf) { free(buf->data); *buf = (STATEBUFFER){0}; }

/* Validate the entire envelope before any consumer sees its payload. */
static SAVERESULT state_read_envelope(LPCSTR path, LPSTATEBUFFER buf, LPCSTATEDEF def) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) return SAVE_MISSING;
        fprintf(stderr, "State: cannot open %s: %s\n", path, strerror(errno));
        return SAVE_INVALID;
    }
    STATEBUFFER tmp = {0};
    STATEFOOTER tail;
    long len;
    BOOL ok = !fseek(f, 0, SEEK_END) && (len = ftell(f)) >= sizeof(tail) && len <= BZ_STATE_LIMIT + sizeof(tail) && !fseek(f, 0, SEEK_SET);
    if (ok) {
        tmp.size = tmp.capacity = (size_t)len;
        tmp.data = malloc(MAX(tmp.size, 1));
        ok = tmp.data && fread(tmp.data, 1, tmp.size, f) == tmp.size;
        if (ok && !state_is_import(&tmp, def ? def->import : NULL)) {
            DWORD legacy = def ? (def->legacy ? def->legacy->footer : def->record.footer) : 0;
            tmp.size -= sizeof(tail);
            memcpy(&tail, tmp.data + tmp.size, sizeof(tail));
            ok = (tail.commit == state_marker || (legacy && tail.commit == legacy)) &&
                tail.checksum == save_hash(2166136261u, tmp.data, tmp.size);
        }
    }
    if (fclose(f)) ok = false;
    if (!ok) {
        fprintf(stderr, "State: invalid or incomplete record %s\n", path);
        state_free(&tmp); return SAVE_INVALID;
    }
    *buf = tmp;
    return SAVE_LOADED;
}

SAVERESULT state_read(LPCSTR path, LPSTATEBUFFER buf) { return state_read_envelope(path, buf, NULL); }

/* One replacement boundary serves profile records and world snapshots alike. */
BOOL state_write(LPCSTR path, LPSTATEBUFFER buf) {
    char tmp[4096];
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= sizeof(tmp)) return false;
    FILE *f = fopen(tmp, "wb");
    if (!f) { fprintf(stderr, "State: cannot open %s: %s\n", tmp, strerror(errno)); return false; }
    STATEFOOTER tail = { .checksum = save_hash(2166136261u, buf->data, buf->size), .commit = state_marker };
    BOOL ok = fwrite(buf->data, 1, buf->size, f) == buf->size && fwrite(&tail, 1, sizeof(tail), f) == sizeof(tail) && !fflush(f);
#ifdef _WIN32
    if (ok) ok = !_commit(_fileno(f));
#else
    if (ok) ok = !fsync(fileno(f));
#endif
    if (fclose(f)) ok = false;
#ifdef _WIN32
    if (ok) ok = MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    if (ok) ok = !rename(tmp, path);
#endif
    if (!ok) { fprintf(stderr, "State: failed to commit %s: %s\n", path, strerror(errno)); remove(tmp); }
    return ok;
}

static BOOL save_io(LPSAVEIO io, void *data, size_t size) {
    return io->reading ? load_bytes(io->buf, data, size) : save_bytes(io->buf, data, size);
}

/* Walk special fields and counted records; pointer-free value blocks retain their native layout. */
BOOL save_fields(LPSAVEIO io, field_t const *fields, void *object) {
    BYTE *base = object;
    for (field_t const *f = fields; f->name; f++) {
        DWORD count = f->array_size ? f->array_size : 1;
        size_t size = f->array_size ? f->size / f->array_size : f->size;
        BYTE *ptr = base + f->ofs;
        if (f->type == F_IGNORE) {
            if (io->image && f->flags) memset(ptr, 0, f->size);
            continue;
        }
        if (f->count_ofs != UINT32_MAX) {
            if (!io->reading || io->image) count = *(DWORD *)(base + f->count_ofs);
            if ((!io->reading && count > f->array_size) || (!io->image && !save_io(io, &count, sizeof(count))) || count > f->array_size)
                goto fail;
            if (io->reading) *(DWORD *)(base + f->count_ofs) = count;
        }
        switch (f->type) {
        case F_STRUCT:
            FOR_LOOP(i, count)
                if (!save_fields(io, f->child, ptr + i * size)) goto fail;
            break;
        case F_STRUCT_RING: {
            SAVERING const *ring = f->ring;
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
            if (io->image) break;
            if ((!io->reading && !memchr(ptr, 0, f->size)) || !save_io(io, ptr, f->size) || !memchr(ptr, 0, f->size))
                goto fail;
            break;
        case F_INT: case F_FLOAT: case F_VECTOR: case F_REGION: case F_ANGLEHACK: case F_BYTES:
            if (!io->image && !save_io(io, ptr, size * count)) goto fail;
            break;
        default:
            if (!io->special || !io->special(io, f, base)) goto fail;
            break;
        }
        continue;
fail:
        fprintf(stderr, "State %s: invalid or incomplete field %s\n", io->reading ? "load" : "save", f->name);
        return false;
    }
    return true;
}


/* Image and mapped blocks share traversal and the same custom identity callback. */
BOOL state_image(LPSAVEIO io, LPCSTATEBLOCK block) {
    void *base = block->data;
    size_t size = block->size;
    void *image = io->reading ? base : malloc(size);
    if (!image) return false;
    if (!io->reading) memcpy(image, base, size);
    BOOL ok = !io->reading || load_bytes(io->buf, image, size);
    io->image = true;
    if (ok) ok = save_fields(io, block->fields, image);
    io->image = false;
    if (ok && !io->reading) ok = save_bytes(io->buf, image, size);
    if (!io->reading) free(image);
    return ok;
}

BOOL save_record(LPCSTR path, LPCSAVERECORD record, void *data) {
    if (record->valid && !record->valid(data)) return false;
    STATEBUFFER buf = {0};
    RECORDHEADER head = { .magic = record->magic, .version = record->version, .size = (DWORD)record->size };
    SAVEIO io = { .buf = &buf };
    BOOL ok = save_bytes(&buf, &head, sizeof(head)) &&
        (record->fields ? save_fields(&io, record->fields, data) : save_bytes(&buf, data, record->size)) && state_write(path, &buf);
    state_free(&buf);
    return ok;
}

/* Decode only after envelope validation; semantic failures cannot partially overwrite the caller. */
static BOOL state_decode(LPSTATEBUFFER buf, LPCSAVERECORD record, void *data) {
    RECORDHEADER head;
    void *temp = calloc(1, record->size);
    SAVEIO io = { .buf = buf, .reading = true };
    buf->pos = 0;
    BOOL ok = temp && load_bytes(buf, &head, sizeof(head)) && head.magic == record->magic &&
        head.version == record->version && head.size == record->size &&
        (record->fields ? save_fields(&io, record->fields, temp) : load_bytes(buf, temp, record->size)) && buf->pos == buf->size;
    if (ok && record->valid) ok = record->valid(temp);
    if (ok) memcpy(data, temp, record->size);
    free(temp);
    return ok;
}

SAVERESULT load_record(LPCSTR path, LPCSAVERECORD record, void *data) {
    STATEBUFFER buf = {0};
    STATEDEF def = { .record = *record };
    SAVERESULT result = state_read_envelope(path, &buf, &def);
    if (result != SAVE_LOADED) return result;
    BOOL ok = state_decode(&buf, record, data);
    if (!ok) fprintf(stderr, "State: incompatible or invalid record %s\n", path);
    state_free(&buf);
    return ok ? SAVE_LOADED : SAVE_INVALID;
}

/* Slots retain values and numeric contracts only; never retain unloaded module schemas or callbacks. */
SAVERESULT state_acquire(LPCSTR path, LPCSTATEDEF def, LPSTATE state) {
    *state = (STATE){0};
    if (!path || !*path || !def->record.size || def->record.fields || def->record.size > BZ_STATE_LIMIT) return SAVE_INVALID;
    for (STATESLOT *slot = state_slots; slot; slot = slot->next) {
        if (slot->life != def->life || strcmp(slot->path, path)) continue;
        if (slot->size != def->record.size || slot->magic != def->record.magic || slot->version != def->record.version) {
            fprintf(stderr, "State: conflicting live contract %s\n", path); return SAVE_INVALID;
        }
        if (def->record.valid && !def->record.valid(slot->data)) return SAVE_INVALID;
        *state = (STATE){ .id = slot->id, .data = slot->data };
        return slot->committed ? SAVE_LOADED : SAVE_MISSING;
    }
    size_t bytes = sizeof(STATESLOT) + def->record.size + strlen(path) + 1;
    if (bytes > BZ_STATE_LIMIT - state_bytes) {
        fprintf(stderr, "State: live storage limit exceeded for %s\n", path); return SAVE_INVALID;
    }
    STATESLOT *slot = calloc(1, sizeof(*slot));
    if (!slot) return SAVE_INVALID;
    slot->data = calloc(1, def->record.size); slot->path = strdup(path);
    SAVERESULT result = slot->data && slot->path ? SAVE_MISSING : SAVE_INVALID;
    if (result != SAVE_INVALID && def->life != STATE_MEMORY) {
        STATEBUFFER buf = {0};
        result = state_read_envelope(path, &buf, def);
        if (result == SAVE_LOADED && state_is_import(&buf, def->import)) {
            BOOL ok = def->import->decode(&buf, slot->data) && buf.pos == buf.size;
            if (ok && def->record.valid) ok = def->record.valid(slot->data);
            if (!ok) { fprintf(stderr, "State: invalid legacy record %s\n", path); result = SAVE_INVALID; }
            else fprintf(stderr, "State: imported legacy record %s; next commit upgrades it\n", path);
        } else if (result == SAVE_LOADED) {
            RECORDHEADER head;
            LPCSAVERECORD record = &def->record;
            if (load_bytes(&buf, &head, sizeof(head)) && def->legacy && head.magic == def->legacy->magic &&
                head.version == def->legacy->version && head.size == def->legacy->size) record = def->legacy;
            if (!state_decode(&buf, record, slot->data)) {
                fprintf(stderr, "State: incompatible or invalid record %s\n", path); result = SAVE_INVALID;
            } else if (record == def->legacy)
                fprintf(stderr, "State: imported legacy record %s; next commit upgrades it\n", path);
        }
        state_free(&buf);
    }
    if (result == SAVE_INVALID) { free(slot->data); free(slot->path); free(slot); return result; }
    slot->size = def->record.size; slot->magic = def->record.magic; slot->version = def->record.version;
    slot->life = def->life; slot->committed = result == SAVE_LOADED; slot->id = ++state_serial;
    slot->next = state_slots; state_slots = slot; state_bytes += bytes;
    *state = (STATE){ .id = slot->id, .data = slot->data };
    return result;
}

/* Callers validate game semantics; publish a private working copy only after disk replacement succeeds. */
BOOL state_commit(DWORD id, LPCVOID data) {
    if (state_in_restore) { fprintf(stderr, "State: commit blocked during world restore\n"); return false; }
    for (STATESLOT *slot = state_slots; slot; slot = slot->next) {
        if (slot->id != id) continue;
        SAVERECORD record = { .magic = slot->magic, .version = slot->version, .size = slot->size };
        if (slot->life != STATE_MEMORY && !save_record(slot->path, &record, (void *)data)) return false;
        if (slot->data != data) memcpy(slot->data, data, slot->size);
        slot->committed = true;
        return true;
    }
    fprintf(stderr, "State: invalid commit handle %u\n", id);
    return false;
}

void state_reset(void) {
    state_bytes = 0;
    while (state_slots) {
        STATESLOT *slot = state_slots;
        state_slots = slot->next;
        free(slot->path); free(slot->data); free(slot);
    }
}

void state_restore(BOOL active) { state_in_restore = active; }
BOOL state_restoring(void) { return state_in_restore; }
