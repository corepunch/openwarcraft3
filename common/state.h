#ifndef BZ_STATE_H
#define BZ_STATE_H
#include "common/shared.h"

/* Scalar types share the parser contract; custom identities belong to the calling module. */
enum {
    F_INT = BZ_FIELD_U32, F_FLOAT = BZ_FIELD_FLOAT, F_STRING = BZ_FIELD_CHAR_ARRAY,
    F_VECTOR = BZ_FIELD_VEC3, F_REGION = 32, F_ANGLEHACK, F_BYTES, F_STRUCT, F_STRUCT_RING, F_IGNORE,
    F_CUSTOM = 256
};
typedef struct statebuffer_s { BYTE *data; size_t size, pos, capacity; } STATEBUFFER;
typedef STATEBUFFER *LPSTATEBUFFER;
/* Slot metadata belongs to the engine, so listing/loading a slot needs no game decoder. */
typedef struct { DWORD magic, version; PATHSTR game, map; } STATESAVE;
typedef const STATESAVE *LPCSTATESAVE;
BOOL state_save_head(LPSTATEBUFFER buf, LPCSTATESAVE info);
BOOL state_load_head(LPSTATEBUFFER buf, LPCSTR game, LPSTR map);
typedef struct statefield_s {
    LPCSTR name;
    DWORD ofs;
    DWORD type;
    size_t size;
    DWORD array_size;
    DWORD flags;
    struct statefield_s const *child;
    struct statering_s const *ring;
    DWORD count_ofs;
} field_t;

typedef struct { void *data; size_t size; field_t const *fields; } STATEBLOCK;
typedef const STATEBLOCK *LPCSTATEBLOCK;

typedef struct statering_s {
    field_t const *fields;
    DWORD read_ofs, write_ofs;
} SAVERING;

typedef struct saveio_s {
    LPSTATEBUFFER buf;
    BOOL image;
    BOOL reading;
    BOOL (*special)(struct saveio_s *, field_t const *, BYTE *);
} SAVEIO;
typedef SAVEIO *LPSAVEIO;
typedef const SAVEIO *LPCSAVEIO;
typedef struct {
    DWORD magic, version, footer; /* footer is an optional legacy envelope marker, used only for import. */
    size_t size;
    field_t const *fields;
    BOOL (*valid)(LPCVOID data); /* Optional semantic validation before commit or publishing a loaded record. */
} SAVERECORD;
typedef SAVERECORD *LPSAVERECORD;
typedef const SAVERECORD *LPCSAVERECORD;
typedef enum { SAVE_INVALID = -1, SAVE_MISSING, SAVE_LOADED } SAVERESULT;

#define BZ_SAVE_FIELD(t, f, k) { .name = #f, .ofs = offsetof(t, f), .type = k, .size = sizeof(((t *)0)->f), .count_ofs = UINT32_MAX }
#define BZ_SAVE_ARRAY(t, f, n, schema) { .name = #f, .ofs = offsetof(t, f), .type = F_STRUCT, .size = sizeof(((t *)0)->f), .array_size = n, .child = schema, .count_ofs = UINT32_MAX }
#define BZ_SAVE_COUNTED(t, f, n, schema, cnt) { .name = #f, .ofs = offsetof(t, f), .type = F_STRUCT, .size = sizeof(((t *)0)->f), .array_size = n, .child = schema, .count_ofs = offsetof(t, cnt) }

BOOL save_bytes(LPSTATEBUFFER buf, LPCVOID data, size_t size);
BOOL load_bytes(LPSTATEBUFFER buf, void *data, size_t size);
DWORD save_hash(DWORD hash, LPCVOID data, size_t size);
BOOL save_fields(LPSAVEIO io, field_t const *fields, void *base);
BOOL state_image(LPSAVEIO io, LPCSTATEBLOCK block);
BOOL save_record(LPCSTR path, LPCSAVERECORD record, void *data);
SAVERESULT load_record(LPCSTR path, LPCSAVERECORD record, void *data);
SAVERESULT state_read(LPCSTR path, LPSTATEBUFFER buf);
BOOL state_write(LPCSTR path, LPSTATEBUFFER buf);
void state_free(LPSTATEBUFFER buf);

typedef enum { STATE_PROFILE, STATE_CACHE, STATE_MEMORY } STATELIFE;
typedef struct {
    DWORD magic[2];
    BOOL (*decode)(LPSTATEBUFFER buf, void *data);
} STATEIMPORT;
typedef struct {
    LPCSTR key;
    STATELIFE life;
    SAVERECORD record;
    LPCSAVERECORD legacy; /* Explicit read-only migration contract. */
    STATEIMPORT const *import; /* Identified legacy bytes without the engine envelope. */
} STATEDEF;
typedef const STATEDEF *LPCSTATEDEF;
typedef struct { DWORD id; void *data; } STATE;
typedef STATE *LPSTATE;
SAVERESULT state_acquire(LPCSTR path, LPCSTATEDEF def, LPSTATE state);
BOOL state_commit(DWORD id, LPCVOID data);
void state_reset(void);
void state_restore(BOOL active);
BOOL state_restoring(void);
#endif
