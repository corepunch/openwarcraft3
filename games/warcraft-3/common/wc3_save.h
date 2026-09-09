#ifndef BZ_WC3_SAVE_H
#define BZ_WC3_SAVE_H
#include "common/shared.h"
#include <stdio.h>

typedef enum {
    F_INT,
    F_FLOAT,
    F_LSTRING,            // string on disk, pointer in memory, TAG_LEVEL
    F_GSTRING,            // string on disk, pointer in memory, TAG_GAME
    F_VECTOR,
    F_REGION,
    F_ANGLEHACK,
    F_EDICT,            // index on disk, pointer in memory
    F_ITEM,                // index on disk, pointer in memory
    F_TRIGGER,          // index on disk, pointer in memory
    F_TIMER,            // index on disk, pointer in memory
    F_EVENT,            // index on disk, pointer in memory
    F_FUNCTION,            // JASS function name; timers/triggers
    F_FUNCTION_LIST,
    F_CFUNCTION,           // C callback roster index; edict think/stand/die
    F_MMOVE,
    F_STRUCT,
    F_STRUCT_RING,
    F_IGNORE,
    F_STRING,           // bounded inline string
    F_UNION             // tagged union selected by a schema table
} fieldtype_t;

typedef struct {
    LPCSTR name;
    DWORD ofs;
    fieldtype_t type;
    size_t size;
    DWORD array_size;
    uintptr_t flags; /* field flags, or child schema pointer for F_STRUCT */
    DWORD count_ofs;
} SAVEFIELD;
typedef SAVEFIELD *LPSAVEFIELD;
typedef const SAVEFIELD *LPCSAVEFIELD;

typedef struct {
    SAVEFIELD const *fields;
    DWORD read_ofs, write_ofs;
} SAVERING;


typedef struct { DWORD tag_ofs, count; LPCSAVEFIELD const *fields; } SAVEUNION;
typedef struct saveio_s {
    FILE *file;
    BOOL reading;
    BOOL (*special)(struct saveio_s *, LPCSAVEFIELD, BYTE *);
} SAVEIO;
typedef SAVEIO *LPSAVEIO;
typedef const SAVEIO *LPCSAVEIO;
typedef struct {
    DWORD magic, version;
    size_t size;
    LPCSAVEFIELD fields;
} SAVERECORD;
typedef SAVERECORD *LPSAVERECORD;
typedef const SAVERECORD *LPCSAVERECORD;
typedef enum { SAVE_INVALID = -1, SAVE_MISSING, SAVE_LOADED } SAVERESULT;

#define BZ_SAVE_FIELD(t, f, k) { .name = #f, .ofs = offsetof(t, f), .type = k, .size = sizeof(((t *)0)->f), .count_ofs = UINT32_MAX }
#define BZ_SAVE_ARRAY(t, f, n, schema) { .name = #f, .ofs = offsetof(t, f), .type = F_STRUCT, .size = sizeof(((t *)0)->f), .array_size = n, .flags = (uintptr_t)schema, .count_ofs = UINT32_MAX }
#define BZ_SAVE_COUNTED(t, f, n, schema, cnt) { .name = #f, .ofs = offsetof(t, f), .type = F_STRUCT, .size = sizeof(((t *)0)->f), .array_size = n, .flags = (uintptr_t)schema, .count_ofs = offsetof(t, cnt) }

BOOL save_bytes(FILE *f, LPCVOID data, size_t size);
BOOL load_bytes(FILE *f, void *data, size_t size);
DWORD save_hash(DWORD hash, LPCVOID data, size_t size);
BOOL save_footer(FILE *f);
BOOL load_footer(FILE *f);
BOOL save_fields(LPSAVEIO io, LPCSAVEFIELD fields, void *base);
BOOL save_record(LPCSTR path, LPCSAVERECORD record, void *data);
SAVERESULT load_record(LPCSTR path, LPCSAVERECORD record, void *data);
#endif
