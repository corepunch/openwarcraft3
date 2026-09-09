#include "g_local.h"

#include "../common/wc3_save.h"

#define GAMECACHE_FILE_SUFFIX ".orcgc"
#define MAX_GAMECACHE_MEMORY_CACHES 8 // caches; bounded committed campaign handles retained across maps

typedef enum {
    GAMECACHE_STORAGE_DISABLED,
    GAMECACHE_STORAGE_MEMORY,
    GAMECACHE_STORAGE_DISK,
} gameCacheStorageMode_t;

typedef struct {
    BOOL inuse;
    gameCache_t cache;
} gameCacheMemorySlot_t;

static gameCacheMemorySlot_t gamecache_memory[MAX_GAMECACHE_MEMORY_CACHES];

static SAVEFIELD const cache_hero_fields[] = {
    BZ_SAVE_FIELD(doodadHero_t, level, F_INT),
    BZ_SAVE_FIELD(doodadHero_t, str, F_INT),
    BZ_SAVE_FIELD(doodadHero_t, agi, F_INT),
    BZ_SAVE_FIELD(doodadHero_t, intel, F_INT),
    BZ_SAVE_FIELD(doodadHero_t, xp, F_INT),
    BZ_SAVE_FIELD(doodadHero_t, suspend_xp, F_INT),
    BZ_SAVE_FIELD(doodadHero_t, skillpoints, F_INT),
    {0}
};
static SAVEFIELD const cache_ability_fields[] = {
    BZ_SAVE_FIELD(heroability_t, code, F_INT),
    BZ_SAVE_FIELD(heroability_t, level, F_INT),
    {0}
};
static SAVEFIELD const cache_item_fields[] = {
    BZ_SAVE_FIELD(gameCacheItem_t, item_id, F_INT),
    BZ_SAVE_FIELD(gameCacheItem_t, charges, F_INT),
    {0}
};
static SAVEFIELD const cache_stat_fields[] = {
    BZ_SAVE_FIELD(EDICTSTAT, value, F_FLOAT),
    BZ_SAVE_FIELD(EDICTSTAT, max_value, F_FLOAT),
    {0}
};
static SAVEFIELD const cache_unit_fields[] = {
    BZ_SAVE_FIELD(gameCacheUnit_t, class_id, F_INT),
    BZ_SAVE_ARRAY(gameCacheUnit_t, hero, 1, cache_hero_fields),
    BZ_SAVE_ARRAY(gameCacheUnit_t, abilities, MAX_HERO_ABILITIES, cache_ability_fields),
    BZ_SAVE_ARRAY(gameCacheUnit_t, health, 1, cache_stat_fields),
    BZ_SAVE_ARRAY(gameCacheUnit_t, mana, 1, cache_stat_fields),
    BZ_SAVE_FIELD(gameCacheUnit_t, unit_color, F_INT),
    BZ_SAVE_ARRAY(gameCacheUnit_t, inventory, MAX_INVENTORY, cache_item_fields),
    {0}
};
static SAVEFIELD const cache_integer_fields[] = {
    { .name = "integer", .type = F_INT, .size = sizeof(LONG), .count_ofs = UINT32_MAX },
    {0}
};
static SAVEFIELD const cache_real_fields[] = {
    { .name = "real", .type = F_FLOAT, .size = sizeof(FLOAT), .count_ofs = UINT32_MAX },
    {0}
};
static SAVEFIELD const cache_boolean_fields[] = {
    { .name = "boolean", .type = F_INT, .size = sizeof(BOOL), .count_ofs = UINT32_MAX },
    {0}
};
static SAVEFIELD const cache_string_fields[] = {
    { .name = "string", .type = F_STRING, .size = sizeof(char[MAX_GAMECACHE_STRING]), .count_ofs = UINT32_MAX },
    {0}
};
static LPCSAVEFIELD const cache_variants[] = {
    [GAMECACHE_INTEGER] = cache_integer_fields,
    [GAMECACHE_REAL] = cache_real_fields,
    [GAMECACHE_BOOLEAN] = cache_boolean_fields,
    [GAMECACHE_STRING] = cache_string_fields,
    [GAMECACHE_UNIT] = cache_unit_fields,
};
static SAVEUNION const cache_value = {
    .tag_ofs = offsetof(gameCacheEntry_t, type), .count = sizeof(cache_variants) / sizeof(*cache_variants),
    .fields = cache_variants,
};
static SAVEFIELD const cache_entry_fields[] = {
    BZ_SAVE_FIELD(gameCacheEntry_t, mission, F_STRING),
    BZ_SAVE_FIELD(gameCacheEntry_t, key, F_STRING),
    BZ_SAVE_FIELD(gameCacheEntry_t, type, F_INT),
    { .name = "value", .ofs = offsetof(gameCacheEntry_t, value), .type = F_UNION, .flags = (uintptr_t)&cache_value, .count_ofs = UINT32_MAX },
    {0}
};
static SAVEFIELD const cache_fields[] = {
    BZ_SAVE_FIELD(gameCache_t, campaign, F_STRING),
    BZ_SAVE_COUNTED(gameCache_t, entries, MAX_GAMECACHE_ENTRIES, cache_entry_fields, num_entries),
    {0}
};
static SAVERECORD const cache_record = {
    .magic = MAKEFOURCC('W','3','G','C'), .version = 2, .size = sizeof(gameCache_t), .fields = cache_fields,
};

static gameCacheStorageMode_t G_GameCacheStorageMode(void) {
    LPCSTR mode = gi.CvarString("wc3_gamecache_mode", "disk");

    if (!strcmp(mode, "disabled")) return GAMECACHE_STORAGE_DISABLED;
    if (!strcmp(mode, "memory")) return GAMECACHE_STORAGE_MEMORY;
    if (!strcmp(mode, "disk")) return GAMECACHE_STORAGE_DISK;
    fprintf(stderr,
            "Game cache: invalid wc3_gamecache_mode '%s' (expected disabled, memory, or disk); using disk\n",
            mode);
    return GAMECACHE_STORAGE_DISK;
}

static gameCacheMemorySlot_t *G_GameCacheMemoryFind(LPCSTR campaign) {
    if (!campaign || !*campaign) return NULL;
    FOR_LOOP(i, MAX_GAMECACHE_MEMORY_CACHES) {
        gameCacheMemorySlot_t *slot = gamecache_memory + i;
        if (slot->inuse && !strcmp(slot->cache.campaign, campaign)) return slot;
    }
    return NULL;
}

static BOOL G_GameCacheMemoryLoad(gameCache_t *cache) {
    gameCacheMemorySlot_t *slot;

    if (!cache || !cache->campaign[0]) return false;
    slot = G_GameCacheMemoryFind(cache->campaign);
    if (!slot) return false;
    *cache = slot->cache;
    cache->dirty = false;
    return true;
}

static BOOL G_GameCacheMemorySave(gameCache_t const *cache) {
    gameCacheMemorySlot_t *slot;

    if (!cache || !cache->campaign[0]) return false;
    slot = G_GameCacheMemoryFind(cache->campaign);
    if (!slot) {
        FOR_LOOP(i, MAX_GAMECACHE_MEMORY_CACHES) {
            if (!gamecache_memory[i].inuse) {
                slot = gamecache_memory + i;
                slot->inuse = true;
                break;
            }
        }
    }
    if (!slot) {
        fprintf(stderr, "Game cache: in-memory campaign limit %u reached while saving '%s'\n",
                (unsigned)MAX_GAMECACHE_MEMORY_CACHES, cache->campaign);
        return false;
    }
    slot->cache = *cache;
    slot->cache.dirty = false;
    return true;
}

static BOOL G_GameCacheKeyFits(LPCSTR value, size_t size, LPCSTR label) {
    if (!value) {
        fprintf(stderr, "Game cache: missing %s\n", label);
        return false;
    }
    if (strlen(value) >= size) {
        fprintf(stderr, "Game cache: %s is too long (%zu >= %zu)\n",
                label, strlen(value), size);
        return false;
    }
    return true;
}

static BOOL G_GameCachePath(LPCSTR campaign, LPSTR out, DWORD out_size) {
    LPCSTR base;
    char safe[MAX_PATHLEN];
    char rel[MAX_PATHLEN];
    size_t len;

    if (!campaign || !*campaign || !out || !out_size) {
        return false;
    }
    base = campaign;
    for (LPCSTR p = campaign; *p; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    if (!*base) {
        fprintf(stderr, "Game cache: invalid campaign file '%s'\n", campaign);
        return false;
    }
    len = strlen(base);
    if (len >= sizeof(safe) - 1) {
        fprintf(stderr, "Game cache: campaign file name is too long: '%s'\n", base);
        return false;
    }
    FOR_LOOP(i, len) {
        unsigned char c = (unsigned char)base[i];
        safe[i] = (isalnum(c) || c == '.' || c == '_' || c == '-') ? (char)c : '_';
    }
    safe[len] = '\0';
    if (snprintf(rel, sizeof(rel), "gamecache-%s%s", safe, GAMECACHE_FILE_SUFFIX) >= (int)sizeof(rel)) {
        fprintf(stderr, "Game cache: resolved cache name is too long for '%s'\n", campaign);
        return false;
    }
    gi.UserPath(rel, out, out_size);
    return true;
}

static gameCacheEntry_t *G_GameCacheFind(gameCache_t *cache, LPCSTR mission, LPCSTR key,
                                         gameCacheValueType_t type) {
    if (!cache || !mission || !key) return NULL;
    FOR_LOOP(i, cache->num_entries) {
        gameCacheEntry_t *entry = cache->entries + i;
        if (entry->type == type && !strcmp(entry->mission, mission) && !strcmp(entry->key, key)) {
            return entry;
        }
    }
    return NULL;
}

static gameCacheEntry_t const *G_GameCacheFindConst(gameCache_t const *cache, LPCSTR mission, LPCSTR key,
                                                    gameCacheValueType_t type) {
    return G_GameCacheFind((gameCache_t *)cache, mission, key, type);
}

static gameCacheEntry_t *G_GameCacheGetOrCreate(gameCache_t *cache, LPCSTR mission, LPCSTR key,
                                                gameCacheValueType_t type) {
    gameCacheEntry_t *entry;

    if (!cache ||
        !G_GameCacheKeyFits(mission, sizeof(cache->entries[0].mission), "mission key") ||
        !G_GameCacheKeyFits(key, sizeof(cache->entries[0].key), "entry key")) {
        return NULL;
    }
    entry = G_GameCacheFind(cache, mission, key, type);
    if (entry) return entry;
    if (cache->num_entries >= MAX_GAMECACHE_ENTRIES) {
        fprintf(stderr, "Game cache '%s': entry limit %u reached while storing '%s/%s'\n",
                cache->campaign, (unsigned)MAX_GAMECACHE_ENTRIES, mission, key);
        return NULL;
    }
    entry = cache->entries + cache->num_entries++;
    memset(entry, 0, sizeof(*entry));
    strlcpy(entry->mission, mission, sizeof(entry->mission));
    strlcpy(entry->key, key, sizeof(entry->key));
    entry->type = type;
    return entry;
}

/* Cache persistence uses the same field walker and committed envelope as other game records. */
static BOOL G_GameCacheLoadDisk(gameCache_t *cache) {
    PATHSTR path;
    return G_GameCachePath(cache->campaign, path, sizeof(path)) && load_record(path, &cache_record, cache) == SAVE_LOADED;
}

void G_GameCacheInit(gameCache_t *cache, LPCSTR campaign) {
    gameCacheStorageMode_t mode;

    if (!cache) return;
    memset(cache, 0, sizeof(*cache));
    if (!campaign || !*campaign) {
        fprintf(stderr, "InitGameCache: empty campaign file\n");
        return;
    }
    if (strlen(campaign) >= sizeof(cache->campaign)) {
        fprintf(stderr, "InitGameCache: campaign file is too long (%zu >= %zu)\n",
                strlen(campaign), sizeof(cache->campaign));
        return;
    }
    strlcpy(cache->campaign, campaign, sizeof(cache->campaign));
    mode = G_GameCacheStorageMode();
    if (mode == GAMECACHE_STORAGE_DISABLED) return;
    if (G_GameCacheMemoryLoad(cache)) return;
    if (mode == GAMECACHE_STORAGE_DISK && G_GameCacheLoadDisk(cache)) {
        G_GameCacheMemorySave(cache);
    }
}

static BOOL G_GameCacheSaveDisk(gameCache_t *cache) {
    PATHSTR path;
    if (!cache || !cache->campaign[0] || !G_GameCachePath(cache->campaign, path, sizeof(path))) return false;
    if (!save_record(path, &cache_record, cache)) return false;
    cache->dirty = false;
    return true;
}

BOOL G_GameCacheSave(gameCache_t *cache) {
    gameCacheStorageMode_t const mode = G_GameCacheStorageMode();

    if (mode == GAMECACHE_STORAGE_DISABLED) {
        if (!cache || !cache->campaign[0]) return false;
        cache->dirty = false;
        return true;
    }
    if (mode == GAMECACHE_STORAGE_MEMORY) {
        if (!G_GameCacheMemorySave(cache)) return false;
        cache->dirty = false;
        return true;
    }
    if (!G_GameCacheSaveDisk(cache)) return false;
    return G_GameCacheMemorySave(cache);
}

static void G_GameCacheRemoveAt(gameCache_t *cache, DWORD index) {
    if (!cache || index >= cache->num_entries) return;
    if (index + 1 < cache->num_entries) {
        memmove(cache->entries + index, cache->entries + index + 1,
                (cache->num_entries - index - 1) * sizeof(cache->entries[0]));
    }
    cache->num_entries--;
    memset(cache->entries + cache->num_entries, 0, sizeof(cache->entries[0]));
    cache->dirty = true;
}

void G_GameCacheFlush(gameCache_t *cache) {
    if (!cache) return;
    memset(cache->entries, 0, sizeof(cache->entries));
    cache->num_entries = 0;
    cache->dirty = true;
}

void G_GameCacheFlushMission(gameCache_t *cache, LPCSTR mission) {
    if (!cache || !mission) return;
    for (DWORD i = 0; i < cache->num_entries;) {
        if (!strcmp(cache->entries[i].mission, mission)) {
            G_GameCacheRemoveAt(cache, i);
        } else {
            i++;
        }
    }
}

void G_GameCacheFlushEntry(gameCache_t *cache, LPCSTR mission, LPCSTR key, gameCacheValueType_t type) {
    if (!cache || !mission || !key) return;
    FOR_LOOP(i, cache->num_entries) {
        gameCacheEntry_t const *entry = cache->entries + i;
        if (entry->type == type && !strcmp(entry->mission, mission) && !strcmp(entry->key, key)) {
            G_GameCacheRemoveAt(cache, i);
            return;
        }
    }
}

BOOL G_GameCacheStoreInteger(gameCache_t *cache, LPCSTR mission, LPCSTR key, LONG value) {
    gameCacheEntry_t *entry = G_GameCacheGetOrCreate(cache, mission, key, GAMECACHE_INTEGER);
    if (!entry) return false;
    entry->value.integer = value;
    cache->dirty = true;
    return true;
}

BOOL G_GameCacheStoreReal(gameCache_t *cache, LPCSTR mission, LPCSTR key, FLOAT value) {
    gameCacheEntry_t *entry = G_GameCacheGetOrCreate(cache, mission, key, GAMECACHE_REAL);
    if (!entry) return false;
    entry->value.real = value;
    cache->dirty = true;
    return true;
}

BOOL G_GameCacheStoreBoolean(gameCache_t *cache, LPCSTR mission, LPCSTR key, BOOL value) {
    gameCacheEntry_t *entry = G_GameCacheGetOrCreate(cache, mission, key, GAMECACHE_BOOLEAN);
    if (!entry) return false;
    entry->value.boolean = value;
    cache->dirty = true;
    return true;
}

BOOL G_GameCacheStoreString(gameCache_t *cache, LPCSTR mission, LPCSTR key, LPCSTR value) {
    gameCacheEntry_t *entry;
    if (!value || strlen(value) >= MAX_GAMECACHE_STRING) {
        fprintf(stderr, "Game cache '%s': stored string '%s/%s' is too long or null\n",
                cache ? cache->campaign : "", mission ? mission : "", key ? key : "");
        return false;
    }
    entry = G_GameCacheGetOrCreate(cache, mission, key, GAMECACHE_STRING);
    if (!entry) return false;
    strlcpy(entry->value.string, value, sizeof(entry->value.string));
    cache->dirty = true;
    return true;
}

BOOL G_GameCacheStoreUnit(gameCache_t *cache, LPCSTR mission, LPCSTR key, LPCEDICT unit) {
    gameCacheEntry_t *entry;
    gameCacheUnit_t *saved;

    if (!unit || !unit->inuse || !unit->class_id) return false;
    entry = G_GameCacheGetOrCreate(cache, mission, key, GAMECACHE_UNIT);
    if (!entry) return false;
    saved = &entry->value.unit;
    memset(saved, 0, sizeof(*saved));
    saved->class_id = unit->class_id;
    saved->hero = unit->hero;
    memcpy(saved->abilities, unit->heroabilities, sizeof(saved->abilities));
    saved->health = unit->health;
    saved->mana = unit->mana;
    saved->unit_color = unit->unit_color;
    FOR_LOOP(i, MAX_INVENTORY) {
        LPCEDICT item = unit->inventory[i];
        if (!item) continue;
        saved->inventory[i].item_id = item->class_id;
        saved->inventory[i].charges = item->item.charges;
    }
    cache->dirty = true;
    return true;
}

BOOL G_GameCacheHave(gameCache_t const *cache, LPCSTR mission, LPCSTR key, gameCacheValueType_t type) {
    return G_GameCacheFindConst(cache, mission, key, type) != NULL;
}

LONG G_GameCacheGetInteger(gameCache_t const *cache, LPCSTR mission, LPCSTR key) {
    gameCacheEntry_t const *entry = G_GameCacheFindConst(cache, mission, key, GAMECACHE_INTEGER);
    return entry ? entry->value.integer : 0;
}

FLOAT G_GameCacheGetReal(gameCache_t const *cache, LPCSTR mission, LPCSTR key) {
    gameCacheEntry_t const *entry = G_GameCacheFindConst(cache, mission, key, GAMECACHE_REAL);
    return entry ? entry->value.real : 0.0f;
}

BOOL G_GameCacheGetBoolean(gameCache_t const *cache, LPCSTR mission, LPCSTR key) {
    gameCacheEntry_t const *entry = G_GameCacheFindConst(cache, mission, key, GAMECACHE_BOOLEAN);
    return entry ? entry->value.boolean : false;
}

LPCSTR G_GameCacheGetString(gameCache_t const *cache, LPCSTR mission, LPCSTR key) {
    gameCacheEntry_t const *entry = G_GameCacheFindConst(cache, mission, key, GAMECACHE_STRING);
    return entry ? entry->value.string : "";
}

LPEDICT G_GameCacheRestoreUnit(gameCache_t const *cache, LPCSTR mission, LPCSTR key,
                              DWORD player, LPCVECTOR2 location, FLOAT facing) {
    gameCacheEntry_t const *entry = G_GameCacheFindConst(cache, mission, key, GAMECACHE_UNIT);
    gameCacheUnit_t const *saved;
    LPEDICT unit;

    if (!entry || !location) return NULL;
    saved = &entry->value.unit;
    unit = SP_SpawnAtLocation(saved->class_id, player, location);
    if (!unit) return NULL;
    unit->s.angle = DEG2RAD(facing);

    if (saved->hero.level) {
        G_HeroApplyLevel(unit, saved->hero.level);
    }
    unit->hero = saved->hero;
    memcpy(unit->heroabilities, saved->abilities, sizeof(unit->heroabilities));
    G_RecomputeHeroStats(unit);
    unit->health = saved->health;
    unit->mana = saved->mana;
    unit->unit_color = saved->unit_color;

    FOR_LOOP(i, MAX_INVENTORY) {
        gameCacheItem_t const *saved_item = saved->inventory + i;
        LPEDICT item;
        if (!saved_item->item_id) continue;
        item = SP_SpawnAtLocation(saved_item->item_id, player, &unit->s.origin2);
        if (!item) {
            fprintf(stderr, "Game cache '%s': failed to restore item %.4s in slot %u\n",
                    cache->campaign, (LPCSTR)&saved_item->item_id, (unsigned)i);
            continue;
        }
        G_SetItemCharges(item, saved_item->charges);
        if (!G_AddItemToSlot(unit, item, i)) {
            fprintf(stderr, "Game cache '%s': cannot restore item %.4s into slot %u of %.4s\n",
                    cache->campaign, (LPCSTR)&saved_item->item_id, (unsigned)i,
                    (LPCSTR)&saved->class_id);
            G_RemoveItem(item);
        }
    }
    return unit;
}
