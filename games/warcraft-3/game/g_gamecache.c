#include "g_local.h"

#include "../common/wc3_save.h"

#define GAMECACHE_FILE_SUFFIX ".orcgc"

typedef enum {
    GAMECACHE_STORAGE_DISABLED,
    GAMECACHE_STORAGE_MEMORY,
    GAMECACHE_STORAGE_DISK,
} gameCacheStorageMode_t;

static BOOL G_GameCacheValid(LPCVOID data);
static BOOL G_GameCacheImport(LPSTATEBUFFER buf, void *data);

enum { F_CACHE_WORDS = F_CUSTOM + 32, F_CACHE_BOOL, F_CACHE_STRING, F_CACHE_TAG, F_CACHE_VALUE };
static STATEIMPORT const cache_import = {
    .magic = { MAKEFOURCC('O','R','G','C'), MAKEFOURCC('A','C','H','E') }, .decode = G_GameCacheImport,
};
/* Main's ORGCACHE v1 wrote little-endian values, length-prefixed strings and tagged payloads. */
static field_t const cache_old_unit[] = {
    BZ_SAVE_FIELD(gameCacheUnit_t, class_id, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.level, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.str, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.agi, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.intel, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.xp, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.suspend_xp, F_CACHE_BOOL),
    BZ_SAVE_FIELD(gameCacheUnit_t, hero.skillpoints, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, abilities, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, health, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, mana, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, unit_color, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheUnit_t, inventory, F_CACHE_WORDS),
    {0}
};
static field_t const cache_old_value[] = {
    BZ_SAVE_FIELD(gameCacheEntry_t, value.integer, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheEntry_t, value.real, F_CACHE_WORDS),
    BZ_SAVE_FIELD(gameCacheEntry_t, value.boolean, F_CACHE_BOOL),
    BZ_SAVE_ARRAY(gameCacheEntry_t, value.unit, 1, cache_old_unit),
    BZ_SAVE_FIELD(gameCacheEntry_t, value.string, F_CACHE_STRING),
    {0}
};
static field_t const cache_old_entry[] = {
    BZ_SAVE_FIELD(gameCacheEntry_t, type, F_CACHE_TAG),
    BZ_SAVE_FIELD(gameCacheEntry_t, mission, F_CACHE_STRING),
    BZ_SAVE_FIELD(gameCacheEntry_t, key, F_CACHE_STRING),
    BZ_SAVE_FIELD(gameCacheEntry_t, value, F_CACHE_VALUE),
    {0}
};

/* The union contains only values. Like Quake II's raw edict scalars, it needs no per-member schema. */
static field_t const cache_entry_fields[] = {
    BZ_SAVE_FIELD(gameCacheEntry_t, mission, F_STRING),
    BZ_SAVE_FIELD(gameCacheEntry_t, key, F_STRING),
    BZ_SAVE_FIELD(gameCacheEntry_t, type, F_INT),
    BZ_SAVE_FIELD(gameCacheEntry_t, value, F_BYTES),
    {0}
};
static field_t const cache_fields[] = {
    BZ_SAVE_FIELD(gameCache_t, campaign, F_STRING),
    BZ_SAVE_COUNTED(gameCache_t, entries, MAX_GAMECACHE_ENTRIES, cache_entry_fields, num_entries),
    {0}
};
static SAVERECORD const cache_record = {
    .magic = MAKEFOURCC('W','3','G','C'), .version = 3, .size = sizeof(gameCache_t), .footer = MAKEFOURCC('W','3','O','K'), .fields = cache_fields, .valid = G_GameCacheValid,
};

static BOOL cache_old_word(LPSTATEBUFFER buf, DWORD *value) {
    BYTE bytes[4];
    if (!load_bytes(buf, bytes, sizeof(bytes))) return false;
    *value = (DWORD)bytes[0] | ((DWORD)bytes[1] << 8) | ((DWORD)bytes[2] << 16) | ((DWORD)bytes[3] << 24);
    return true;
}

/* Only the legacy tagged union and endian/string conversions need custom productions. */
static BOOL cache_old_field(LPSAVEIO io, field_t const *field, BYTE *base) {
    BYTE *ptr = base + field->ofs;
    DWORD value;
    switch (field->type) {
    case F_CACHE_WORDS:
        for (size_t ofs = 0; ofs < field->size; ofs += sizeof(DWORD)) {
            if (!cache_old_word(io->buf, &value)) return false;
            memcpy(ptr + ofs, &value, sizeof(value));
        }
        return true;
    case F_CACHE_BOOL:
        if (!cache_old_word(io->buf, &value)) return false;
        *(BOOL *)ptr = value != 0;
        return true;
    case F_CACHE_TAG: {
        BYTE tag;
        if (!load_bytes(io->buf, &tag, sizeof(tag)) || tag < GAMECACHE_INTEGER || tag > GAMECACHE_STRING) return false;
        *(gameCacheValueType_t *)ptr = tag;
        return true;
    }
    case F_CACHE_STRING: {
        BYTE len[2];
        if (!load_bytes(io->buf, len, sizeof(len))) return false;
        value = len[0] | ((DWORD)len[1] << 8);
        if (value >= field->size || !load_bytes(io->buf, ptr, value)) return false;
        ptr[value] = 0;
        return true;
    }
    case F_CACHE_VALUE: {
        gameCacheEntry_t *entry = (gameCacheEntry_t *)base;
        field_t fields[] = { cache_old_value[entry->type - GAMECACHE_INTEGER], {0} };
        return save_fields(io, fields, base);
    }
    default: return false;
    }
}

/* Import into unpublished scratch storage; the engine preserves the original until the next commit. */
static BOOL G_GameCacheImport(LPSTATEBUFFER buf, void *data) {
    gameCache_t *cache = data;
    DWORD head[4];
    FOR_LOOP(i, 4) if (!cache_old_word(buf, head + i)) return false;
    if (head[0] != cache_import.magic[0] || head[1] != cache_import.magic[1] || head[2] != 1 ||
        head[3] > MAX_GAMECACHE_ENTRIES) return false;
    cache->num_entries = head[3];
    SAVEIO io = { .buf = buf, .reading = true, .special = cache_old_field };
    FOR_LOOP(i, cache->num_entries)
        if (!save_fields(&io, cache_old_entry, cache->entries + i)) return false;
    return buf->pos == buf->size && G_GameCacheValid(cache);
}

/* Raw values still require tag and string validation before writing or exposing a loaded cache. */
static BOOL G_GameCacheValid(LPCVOID data) {
    gameCache_t const *cache = data;
    if (!memchr(cache->campaign, 0, sizeof(cache->campaign)) || cache->num_entries > MAX_GAMECACHE_ENTRIES) {
        fprintf(stderr, "Game cache: invalid entry count %u\n", cache->num_entries);
        return false;
    }
    FOR_LOOP(i, cache->num_entries) {
        gameCacheEntry_t const *entry = cache->entries + i;
        if (!memchr(entry->mission, 0, sizeof(entry->mission)) || !memchr(entry->key, 0, sizeof(entry->key)) ||
            entry->type < GAMECACHE_INTEGER || entry->type > GAMECACHE_STRING ||
            (entry->type == GAMECACHE_STRING && !memchr(entry->value.string, 0, sizeof(entry->value.string)))) {
            fprintf(stderr, "Game cache: invalid entry %u (type %u or unterminated string)\n", i, entry->type);
            return false;
        }
    }
    return true;
}

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

static BOOL G_GameCacheKey(LPCSTR campaign, LPSTR out, DWORD out_size) {
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
    strlcpy(out, rel, out_size);
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

/* Script handles remain private working copies until SaveGameCache publishes the whole value. */
static SAVERESULT G_GameCacheAcquire(gameCache_t const *cache, LPSTATE state) {
    PATHSTR path;
    if (!cache || !cache->campaign[0] || !G_GameCacheKey(cache->campaign, path, sizeof(path))) return SAVE_INVALID;
    /* The basename mapping preserves existing cache aliases and on-disk record names. */
    STATEDEF def = {
        .key = path,
        .life = G_GameCacheStorageMode() == GAMECACHE_STORAGE_MEMORY ? STATE_MEMORY : STATE_CACHE,
        .record = { .magic = MAKEFOURCC('W','3','G','C'), .version = 4, .size = sizeof(gameCache_t), .valid = G_GameCacheValid },
        .legacy = &cache_record,
        .import = &cache_import,
    };
    return gi.StateAcquire(&def, state);
}

void G_GameCacheInit(gameCache_t *cache, LPCSTR campaign) {
    if (!cache) return;
    memset(cache, 0, sizeof(*cache));
    if (!campaign || !*campaign || !G_GameCacheKeyFits(campaign, sizeof(cache->campaign), "campaign")) {
        fprintf(stderr, "InitGameCache: invalid campaign file\n"); return;
    }
    strlcpy(cache->campaign, campaign, sizeof(cache->campaign));
    if (G_GameCacheStorageMode() == GAMECACHE_STORAGE_DISABLED) return;
    STATE state;
    SAVERESULT result = G_GameCacheAcquire(cache, &state);
    if (result == SAVE_LOADED && G_GameCacheValid(state.data)) *cache = *(gameCache_t *)state.data;
    else if (result == SAVE_INVALID) fprintf(stderr, "InitGameCache: cannot acquire %s\n", campaign);
    /* ORGCACHE did not store its filename; the acquiring handle owns this logical identity. */
    strlcpy(cache->campaign, campaign, sizeof(cache->campaign));
    cache->dirty = false;
}

BOOL G_GameCacheSave(gameCache_t *cache) {
    if (!cache || !cache->campaign[0] || !G_GameCacheValid(cache)) return false;
    if (G_GameCacheStorageMode() != GAMECACHE_STORAGE_DISABLED) {
        STATE state;
        if (G_GameCacheAcquire(cache, &state) == SAVE_INVALID || !gi.StateCommit(state.id, cache)) return false;
    }
    cache->dirty = false;
    return true;
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

#ifdef BZ_TESTS
static BOOL G_GameCachePath(LPCSTR campaign, LPSTR out, DWORD size) {
    PATHSTR key;
    if (!G_GameCacheKey(campaign, key, sizeof(key))) return false;
    gi.UserPath(key, out, size);
    return true;
}
#endif
