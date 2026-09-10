#ifdef BZ_TESTS
#include "test.h"
#include "../g_local.h"

static void persistence_userpath(LPCSTR rel, LPSTR out, DWORD size) {
    snprintf(out, size, "/tmp/wc3-persistence-test-%s", rel);
}

static LPCSTR persistence_roc(LPCSTR name, LPCSTR fallback) {
    return !strcmp(name, "fs_expansion") ? "0" : !strcmp(name, "wc3_gamecache_mode") ? "disk" : fallback;
}
static LPCSTR persistence_tft(LPCSTR name, LPCSTR fallback) {
    return !strcmp(name, "fs_expansion") ? "1" : persistence_roc(name, fallback);
}

/* Author the historical byte grammar independently of the importer, including unaligned scalar words. */
static void persistence_old_word(LPSTATEBUFFER buf, DWORD value) {
    BYTE bytes[4];
    FOR_LOOP(i, 4) bytes[i] = value >> (i * 8);
    T_ASSERT(save_bytes(buf, bytes, sizeof(bytes)));
}
static void persistence_old_string(LPSTATEBUFFER buf, LPCSTR value) {
    size_t size = strlen(value);
    BYTE len[2] = { size, size >> 8 };
    T_ASSERT(save_bytes(buf, len, sizeof(len))); T_ASSERT(save_bytes(buf, value, size));
}

TEST(wc3_persistence, imports_main_cache_and_preserves_original_until_commit) {
    struct game_import saved = gi;
    gi.UserPath = persistence_userpath; gi.StateAcquire = test_state_acquire;
    gi.StateCommit = state_commit; gi.CvarString = persistence_roc;
    PATHSTR path;
    G_GameCachePath("legacy.w3v", path, sizeof(path));
    STATEBUFFER buf = {0};
    persistence_old_word(&buf, MAKEFOURCC('O','R','G','C'));
    persistence_old_word(&buf, MAKEFOURCC('A','C','H','E'));
    persistence_old_word(&buf, 1); persistence_old_word(&buf, 5);
    for (BYTE tag = GAMECACHE_INTEGER; tag <= GAMECACHE_STRING; tag++) {
        T_ASSERT(save_bytes(&buf, &tag, sizeof(tag)));
        persistence_old_string(&buf, "Human01"); persistence_old_string(&buf, "same");
        if (tag == GAMECACHE_STRING) persistence_old_string(&buf, "carried");
        else if (tag == GAMECACHE_UNIT) {
            DWORD hero[] = { MAKEFOURCC('H','p','a','l'), 2, 21, 22, 23, 1000, 1, 3 };
            FOR_LOOP(i, sizeof(hero) / sizeof(*hero)) persistence_old_word(&buf, hero[i]);
            FOR_LOOP(i, MAX_HERO_ABILITIES) {
                persistence_old_word(&buf, i ? 0 : MAKEFOURCC('A','H','h','b'));
                persistence_old_word(&buf, i ? 0 : 1);
            }
            FLOAT stats[] = {123, 250, 45, 100};
            FOR_LOOP(i, 4) { DWORD bits; memcpy(&bits, stats + i, sizeof(bits)); persistence_old_word(&buf, bits); }
            persistence_old_word(&buf, 5);
            FOR_LOOP(i, MAX_INVENTORY) {
                persistence_old_word(&buf, i == 2 ? MAKEFOURCC('s','p','r','o') : 0);
                persistence_old_word(&buf, i == 2 ? 3 : 0);
            }
        } else persistence_old_word(&buf, tag == GAMECACHE_INTEGER ? (DWORD)-42 : tag == GAMECACHE_REAL ? 0x40500000 : 1);
    }
    FILE *f = fopen(path, "wb"); T_NOT_NULL(f);
    T_EQ(fwrite(buf.data, 1, buf.size, f), buf.size); T_EQ(fclose(f), 0);
    gameCache_t *cache = calloc(1, sizeof(*cache)); T_NOT_NULL(cache);
    state_reset(); G_GameCacheInit(cache, "legacy.w3v");
    T_EQ(cache->num_entries, 5); T_STREQ(cache->campaign, "legacy.w3v");
    T_EQ(G_GameCacheGetInteger(cache, "Human01", "same"), -42);
    T_FEQ(G_GameCacheGetReal(cache, "Human01", "same"), 3.25f, 0.001f);
    T_ASSERT(G_GameCacheGetBoolean(cache, "Human01", "same"));
    T_STREQ(G_GameCacheGetString(cache, "Human01", "same"), "carried");
    gameCacheUnit_t *unit = &cache->entries[3].value.unit;
    T_EQ(unit->class_id, MAKEFOURCC('H','p','a','l')); T_EQ(unit->hero.level, 2);
    T_EQ(unit->hero.str, 21); T_EQ(unit->hero.agi, 22); T_EQ(unit->hero.intel, 23);
    T_EQ(unit->hero.xp, 1000); T_ASSERT(unit->hero.suspend_xp); T_EQ(unit->hero.skillpoints, 3);
    T_EQ(unit->abilities[0].code, MAKEFOURCC('A','H','h','b')); T_EQ(unit->abilities[0].level, 1);
    T_FEQ(unit->health.value, 123, 0.001f); T_FEQ(unit->health.max_value, 250, 0.001f);
    T_FEQ(unit->mana.value, 45, 0.001f); T_FEQ(unit->mana.max_value, 100, 0.001f);
    T_EQ(unit->unit_color, 5); T_EQ(unit->inventory[2].item_id, MAKEFOURCC('s','p','r','o'));
    T_EQ(unit->inventory[2].charges, 3);
    BYTE *original = malloc(buf.size); T_NOT_NULL(original);
    f = fopen(path, "rb"); T_NOT_NULL(f);
    T_EQ(fread(original, 1, buf.size, f), buf.size); T_EQ(fclose(f), 0);
    T_EQ(memcmp(original, buf.data, buf.size), 0); free(original);
    T_ASSERT(G_GameCacheSave(cache)); state_reset(); G_GameCacheInit(cache, "legacy.w3v");
    T_EQ(G_GameCacheGetInteger(cache, "Human01", "same"), -42);
    /* Exact magic opts into import, but wrong versions, invalid tags and truncation must still reject. */
    FOR_LOOP(i, 3) {
        buf.data[8] = i == 0 ? 2 : 1;
        buf.data[16] = i == 1 ? 99 : GAMECACHE_INTEGER;
        f = fopen(path, "wb"); T_NOT_NULL(f);
        size_t size = buf.size - (i == 2);
        T_EQ(fwrite(buf.data, 1, size, f), size); T_EQ(fclose(f), 0);
        state_reset(); STATE state;
        T_EQ(G_GameCacheAcquire(cache, &state), SAVE_INVALID); T_NULL(state.data);
        T_ASSERT(!G_GameCacheSave(cache));
    }
    state_reset(); state_free(&buf); free(cache); remove(path); gi = saved;
}

/* Force disk reloads between independent handles, including every tagged value and a Hero snapshot. */
TEST(wc3_persistence, cache_disk_commit_and_hero_restore) {
    struct game_import saved = gi;
    PATHSTR path;
    gi.UserPath = persistence_userpath; gi.StateAcquire = test_state_acquire; gi.StateCommit = state_commit; state_reset(); gi.CvarString = persistence_roc;
    G_GameCachePath("persist.w3v", path, sizeof(path));
    remove(path);
    state_reset();
    gameCache_t *cache = calloc(1, sizeof(*cache)), *other = calloc(1, sizeof(*other));
    T_NOT_NULL(cache); T_NOT_NULL(other);
    setup_test_world();
    LPEDICT hero = alloc_test_unit(MAKEFOURCC('H','p','a','l'), 0, 0);
    G_HeroApplyLevel(hero, 2);
    hero->heroabilities[0] = (heroability_t){ .code = MAKEFOURCC('A','H','h','b'), .level = 1 };
    hero->hero.skillpoints = 1;
    hero->health.value = 123; hero->mana.value = 45;
    LPEDICT item = make_item_test_world_item(MAKEFOURCC('s','p','r','o'), 0, 0);
    G_SetItemCharges(item, 3);
    T_ASSERT(G_AddItemToSlot(hero, item, 2));
    G_GameCacheInit(cache, "persist.w3v");
    T_ASSERT(G_GameCacheStoreInteger(cache, "Human01", "same", -42));
    T_ASSERT(G_GameCacheStoreReal(cache, "Human01", "same", 3.25f));
    T_ASSERT(G_GameCacheStoreBoolean(cache, "Human01", "same", true));
    T_ASSERT(G_GameCacheStoreString(cache, "Human01", "same", "carried"));
    T_ASSERT(G_GameCacheStoreUnit(cache, "Human01", "Arthas", hero));
    G_GameCacheInit(other, "persist.w3v");
    T_EQ(other->num_entries, 0); /* Stores do not cross the commit boundary. */
    T_ASSERT(G_GameCacheSave(cache));
    T_ASSERT(G_GameCacheStoreInteger(cache, "Human01", "same", 100));
    state_reset();
    G_GameCacheInit(other, "persist.w3v");
    T_EQ(G_GameCacheGetInteger(other, "Human01", "same"), -42);
    T_FEQ(G_GameCacheGetReal(other, "Human01", "same"), 3.25f, 0.001f);
    T_ASSERT(G_GameCacheGetBoolean(other, "Human01", "same"));
    T_STREQ(G_GameCacheGetString(other, "Human01", "same"), "carried");
    VECTOR2 pos = {128, 64};
    LPEDICT restored = G_GameCacheRestoreUnit(other, "Human01", "Arthas", 0, &pos, 90);
    T_NOT_NULL(restored);
    T_EQ(restored->hero.level, 2); T_EQ(restored->hero.skillpoints, 1);
    T_EQ(restored->heroabilities[0].level, 1);
    T_FEQ(restored->health.value, 123, 0.001f); T_FEQ(restored->mana.value, 45, 0.001f);
    T_NOT_NULL(restored->inventory[2]);
    T_EQ(restored->inventory[2]->class_id, item->class_id);
    T_EQ(restored->inventory[2]->item.charges, 3);
    /* Invalid tagged payloads must leave the last committed cache loadable. */
    cache->entries[0].type = (gameCacheValueType_t)99;
    T_ASSERT(!G_GameCacheSave(cache));
    state_reset();
    G_GameCacheInit(other, "persist.w3v");
    T_EQ(G_GameCacheGetInteger(other, "Human01", "same"), -42);
    free(cache); free(other); remove(path);
    state_reset();
    gi = saved;
}

/* A checksum-valid raw union still needs semantic checks before replacing live state. */
TEST(wc3_persistence, cache_raw_values_reject_invalid_tags_and_strings) {
    PATHSTR path;
    persistence_userpath("raw-cache.orcgc", path, sizeof(path)); remove(path);
    gameCache_t *cache = calloc(1, sizeof(*cache)), *other = calloc(1, sizeof(*other));
    T_NOT_NULL(cache); T_NOT_NULL(other);
    strlcpy(cache->campaign, "raw.w3v", sizeof(cache->campaign));
    cache->num_entries = 1;
    other->dirty = true;
    strlcpy(other->campaign, "unchanged.w3v", sizeof(other->campaign));
    SAVERECORD unchecked = cache_record;
    unchecked.valid = NULL; /* Author malformed fixtures with a valid envelope and checksum. */
    FOR_LOOP(i, 2) {
        cache->entries[0].type = i ? GAMECACHE_STRING : (gameCacheValueType_t)99;
        memset(cache->entries[0].value.string, 'x', sizeof(cache->entries[0].value.string));
        T_ASSERT(!save_record(path, &cache_record, cache));
        T_ASSERT(save_record(path, &unchecked, cache));
        T_EQ(load_record(path, &cache_record, other), SAVE_INVALID);
        T_STREQ(other->campaign, "unchanged.w3v");
        T_EQ(other->num_entries, 0); T_ASSERT(other->dirty);
    }
    free(cache); free(other); remove(path);
}

/* Campaign offsets overlap across expansions; cinematic natives use global indexes instead. */
TEST(wc3_persistence, native_progress_survives_reload_and_separates_expansions) {
    struct game_import saved = gi;
    PATHSTR path;
    gi.UserPath = persistence_userpath; gi.StateAcquire = test_state_acquire; gi.StateCommit = state_commit; state_reset(); gi.CvarString = persistence_tft;
    strlcpy(level.map_path, "Maps/Campaign/Human01.w3m", sizeof(level.map_path));
    persistence_userpath(BZ_PROGRESS_FILE, path, sizeof(path)); remove(path);
    T_ASSERT(run_test_jass(
        "function main takes nothing returns nothing\n"
        "call SetTutorialCleared(true)\n"
        "call SetCampaignAvailable(1, true)\n"
        "call SetMissionAvailable(1, 1, true)\n"
        "call SetOpCinematicAvailable(1, true)\n"
        "call SetEdCinematicAvailable(1, false)\n"
        "endfunction\n"));
    gi.CvarString = persistence_roc;
    strlcpy(level.map_path, "Maps/FrozenThrone/Campaign/HumanX01.w3x", sizeof(level.map_path));
    T_ASSERT(run_test_jass(
        "function main takes nothing returns nothing\n"
        "call SetCampaignAvailable(1, false)\n"
        "call SetMissionAvailable(1, 2, true)\n"
        "call SetEdCinematicAvailable(6, true)\n"
        "endfunction\n"));
    CAMPAIGNPROGRESS state = {0};
    T_EQ(progress_load(path, &state), SAVE_LOADED);
    T_EQ(state.tutorial, PROGRESS_OPEN);
    T_EQ(state.campaigns[1].avail, PROGRESS_OPEN);
    T_EQ(state.campaigns[1].missions[1], PROGRESS_OPEN);
    T_EQ(state.campaigns[1].missions[2], PROGRESS_UNSET);
    T_EQ(state.campaigns[1].opening, PROGRESS_OPEN);
    T_EQ(state.campaigns[1].ending, PROGRESS_LOCKED);
    T_EQ(state.campaigns[6].avail, PROGRESS_LOCKED);
    T_EQ(state.campaigns[6].missions[2], PROGRESS_OPEN);
    T_EQ(state.campaigns[6].ending, PROGRESS_OPEN);
    PROGRESSCHANGE bad = { .kind = PROGRESS_MISSION, .campaign = 9, .available = true };
    T_ASSERT(!progress_change(path, &bad));
    bad.campaign = 1; bad.mission = BZ_PROGRESS_MISSIONS;
    T_ASSERT(!progress_change(path, &bad));
    T_EQ(progress_load(path, &state), SAVE_LOADED);
    T_EQ(state.campaigns[6].missions[2], PROGRESS_OPEN);
    remove(path); gi = saved;
}

/* Repeated map visits merge with native unlocks without depending on a menu launch or normal save. */
TEST(wc3_persistence, map_history_and_unlocks_share_one_committed_profile) {
    PATHSTR path;
    persistence_userpath(BZ_PROGRESS_FILE, path, sizeof(path)); remove(path);
    PROGRESSCHANGE visit = { .kind = PROGRESS_PLAYED, .map = "Maps\\Campaign\\Human01.w3m" };
    T_ASSERT(progress_change(path, &visit));
    PROGRESSCHANGE unlock = { .kind = PROGRESS_MISSION, .campaign = 1, .mission = 1, .available = true };
    T_ASSERT(progress_change(path, &unlock));
    visit.map = "maps/campaign/human01.w3m"; visit.kind = PROGRESS_COMPLETED;
    T_ASSERT(progress_change(path, &visit));
    CAMPAIGNPROGRESS state = {0};
    T_EQ(progress_load(path, &state), SAVE_LOADED);
    T_EQ(state.count, 1);
    T_EQ(progress_map_flags(&state, "MAPS/CAMPAIGN/HUMAN01.W3M"), PROGRESS_MAP_PLAYED | PROGRESS_MAP_COMPLETED);
    T_EQ(state.campaigns[1].missions[1], PROGRESS_OPEN);
    remove(path);
}

/* A bad record cannot partially load, and an attempted update cannot overwrite it with an empty profile. */
TEST(wc3_persistence, corruption_and_truncation_leave_state_and_file_untouched) {
    PATHSTR path;
    persistence_userpath(BZ_PROGRESS_FILE, path, sizeof(path)); remove(path);
    PROGRESSCHANGE change = { .kind = PROGRESS_CAMPAIGN, .campaign = 1, .available = true };
    T_ASSERT(progress_change(path, &change));
    FILE *f = fopen(path, "r+b");
    T_NOT_NULL(f);
    T_EQ(fseek(f, 0, SEEK_END), 0);
    long size = ftell(f);
    T_EQ(fseek(f, 0, SEEK_SET), 0);
    BYTE bad = 0;
    T_EQ(fwrite(&bad, 1, 1, f), 1); fclose(f);
    CAMPAIGNPROGRESS state = { .tutorial = PROGRESS_OPEN };
    T_EQ(progress_load(path, &state), SAVE_INVALID); T_EQ(state.tutorial, PROGRESS_OPEN);
    T_ASSERT(!progress_change(path, &change));
    f = fopen(path, "rb"); T_NOT_NULL(f);
    T_EQ(fseek(f, 0, SEEK_END), 0); T_EQ(ftell(f), size); fclose(f);
    f = fopen(path, "wb"); T_NOT_NULL(f); T_EQ(fwrite(&bad, 1, 1, f), 1); fclose(f);
    T_EQ(progress_load(path, &state), SAVE_INVALID); T_EQ(state.tutorial, PROGRESS_OPEN);
    remove(path);
}
#endif
