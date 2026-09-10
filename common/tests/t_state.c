#ifdef BZ_TESTS
#include "common/state.h"
#include "shared/test.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <direct.h>
#endif

typedef struct { DWORD value, reserved; } TESTSTATE;
static STATEDEF const test_state_def = {
    .key = "state-test", .life = STATE_PROFILE,
    .record = { .magic = MAKEFOURCC('T','E','S','T'), .version = 1, .size = sizeof(TESTSTATE) },
};

/* Engine metadata is readable without a game DLL, and rejects wrong games and malformed map names. */
TEST(state, slot_metadata_and_payload_boundary) {
    STATEBUFFER buf = {0};
    STATESAVE info = { .game = "test-game", .map = "Maps/Test.map" };
    PATHSTR map;
    DWORD value = 37, restored = 0;
    T_ASSERT(state_save_head(&buf, &info)); T_ASSERT(save_bytes(&buf, &value, sizeof(value)));
    T_ASSERT(state_load_head(&buf, info.game, map)); T_STREQ(map, info.map);
    T_EQ(buf.pos, sizeof(STATESAVE));
    T_ASSERT(load_bytes(&buf, &restored, sizeof(restored))); T_EQ(restored, value);
    T_ASSERT(!state_load_head(&buf, "different-game", map));
    ((STATESAVE *)buf.data)->version++;
    T_ASSERT(!state_load_head(&buf, info.game, map));
    ((STATESAVE *)buf.data)->version--;
    memset(((STATESAVE *)buf.data)->map, 'x', sizeof(PATHSTR));
    T_ASSERT(!state_load_head(&buf, info.game, map));
    buf.size = sizeof(STATESAVE) - 1;
    T_ASSERT(!state_load_head(&buf, info.game, map));
    state_free(&buf);
    memset(info.map, 'x', sizeof(info.map));
    T_ASSERT(!state_save_head(&buf, &info)); T_EQ(buf.size, 0);
}

/* Reacquisition must share live storage even when no file exists; reset models a new process. */
TEST(state, shared_live_root_and_restart) {
    char path[128];
    snprintf(path, sizeof(path), "/tmp/engine-state-%u.bin", (unsigned)getpid());
    remove(path); state_reset();
    STATE first, second;
    T_EQ(state_acquire(path, &test_state_def, &first), SAVE_MISSING);
    ((TESTSTATE *)first.data)->value = 37;
    T_EQ(state_acquire(path, &test_state_def, &second), SAVE_MISSING);
    T_ASSERT(first.data == second.data); T_EQ(first.id, second.id);
    T_EQ(((TESTSTATE *)second.data)->value, 37);
    T_ASSERT(state_commit(first.id, first.data));
    DWORD old = first.id;
    state_reset();
    T_ASSERT(!state_commit(old, &(TESTSTATE){0}));
    T_EQ(state_acquire(path, &test_state_def, &second), SAVE_LOADED);
    T_EQ(((TESTSTATE *)second.data)->value, 37);
    STATEDEF mismatch = test_state_def;
    mismatch.record.version++;
    T_EQ(state_acquire(path, &mismatch, &first), SAVE_INVALID);
    T_NULL(first.data);
    state_reset(); remove(path);
}

/* Failure before replacement must preserve both the old disk bytes and the published committed cache. */
TEST(state, failed_commit_preserves_disk_and_private_copy) {
    char path[128], tmp[132];
    snprintf(path, sizeof(path), "/tmp/engine-state-commit-%u.bin", (unsigned)getpid());
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    remove(path); state_reset();
    STATE slot;
    T_EQ(state_acquire(path, &test_state_def, &slot), SAVE_MISSING);
    TESTSTATE initial = { .value = 7 }, next = { .value = 19 }, disk = {0};
    T_ASSERT(state_commit(slot.id, &initial));
#ifdef _WIN32
    T_EQ(_mkdir(tmp), 0);
#else
    T_EQ(mkdir(tmp, 0700), 0);
#endif
    T_ASSERT(!state_commit(slot.id, &next));
    T_EQ(((TESTSTATE *)slot.data)->value, 7);
    T_EQ(load_record(path, &test_state_def.record, &disk), SAVE_LOADED); T_EQ(disk.value, 7);
    /* Direct profile edits remain pending and can be retried without reacquiring an empty copy. */
    *(TESTSTATE *)slot.data = next;
    T_ASSERT(!state_commit(slot.id, slot.data)); T_EQ(((TESTSTATE *)slot.data)->value, 19);
    T_EQ(rmdir(tmp), 0);
    T_ASSERT(state_commit(slot.id, slot.data));
    T_EQ(load_record(path, &test_state_def.record, &disk), SAVE_LOADED); T_EQ(disk.value, 19);
    state_reset(); remove(path);
}

/* Versions reject same-size changes, and malformed envelopes never publish writable replacement state. */
TEST(state, invalid_records_are_not_empty_profiles) {
    char path[128];
    snprintf(path, sizeof(path), "/tmp/engine-state-invalid-%u.bin", (unsigned)getpid());
    remove(path); state_reset();
    TESTSTATE value = { .value = 3 };
    T_ASSERT(save_record(path, &test_state_def.record, &value));
    STATEDEF mismatch = test_state_def;
    mismatch.record.version++;
    STATE slot;
    T_EQ(state_acquire(path, &mismatch, &slot), SAVE_INVALID); T_NULL(slot.data);
    FILE *f = fopen(path, "r+b"); T_NOT_NULL(f);
    T_EQ(fputc(0, f), 0); T_EQ(fclose(f), 0);
    T_EQ(state_acquire(path, &test_state_def, &slot), SAVE_INVALID); T_NULL(slot.data);
    state_reset(); remove(path);
}

/* A memory cache commits privately supplied values without ever touching the filesystem. */
TEST(state, memory_and_restore_boundaries) {
    STATEDEF def = test_state_def;
    def.life = STATE_MEMORY;
    STATE slot;
    state_reset();
    T_EQ(state_acquire("/nonexistent/engine-state-memory", &def, &slot), SAVE_MISSING);
    TESTSTATE value = { .value = 11 };
    T_ASSERT(state_commit(slot.id, &value));
    state_restore(true);
    value.value = 22;
    T_ASSERT(!state_commit(slot.id, &value));
    T_EQ(((TESTSTATE *)slot.data)->value, 11);
    state_restore(false); state_reset();
}
/* An explicit legacy schema imports old bytes without retaining pointers into its supplying module. */
TEST(state, legacy_import_survives_descriptor_lifetime) {
    char path[128];
    snprintf(path, sizeof(path), "/tmp/engine-state-legacy-%u.bin", (unsigned)getpid());
    field_t const fields[] = {
        BZ_SAVE_FIELD(TESTSTATE, reserved, F_INT),
        BZ_SAVE_FIELD(TESTSTATE, value, F_INT),
        {0}
    };
    SAVERECORD legacy = test_state_def.record;
    legacy.version = 0; legacy.fields = fields; legacy.footer = MAKEFOURCC('O','L','D','!');
    TESTSTATE value = { .value = 37, .reserved = 5 };
    T_ASSERT(save_record(path, &legacy, &value));
    FILE *f = fopen(path, "r+b"); T_NOT_NULL(f);
    T_EQ(fseek(f, -(long)sizeof(DWORD), SEEK_END), 0);
    T_EQ(fwrite(&legacy.footer, sizeof(DWORD), 1, f), 1); T_EQ(fclose(f), 0);
    STATEDEF def = test_state_def;
    def.legacy = &legacy;
    STATE slot;
    state_reset();
    T_EQ(state_acquire(path, &def, &slot), SAVE_LOADED);
    T_EQ(((TESTSTATE *)slot.data)->value, 37); T_EQ(((TESTSTATE *)slot.data)->reserved, 5);
    memset(&def, 0, sizeof(def)); memset(&legacy, 0, sizeof(legacy));
    T_ASSERT(state_commit(slot.id, slot.data));
    state_reset();
    T_EQ(state_acquire(path, &test_state_def, &slot), SAVE_LOADED);
    T_EQ(((TESTSTATE *)slot.data)->value, 37); T_EQ(((TESTSTATE *)slot.data)->reserved, 5);
    state_reset(); remove(path);
}
#endif
