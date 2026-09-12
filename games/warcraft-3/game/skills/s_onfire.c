/* WC3 building damage presentation.  The game selects the authored model and
 * attachment slots so the renderer only consumes server-authored effect data. */
#include "../g_local.h"

typedef struct {
    LPCSTR dir[2];    /* 0 = small-building directory, 1 = large-building directory. */
    LPCSTR prefix[2]; /* 0 = small-building prefix, 1 = large-building prefix. */
} onFireNames_t;

typedef struct {
    BYTE size;
    BYTE variant;
    USHORT slots;
} onFireStage_t;

static onFireNames_t const onfire_standard = {
    { "SmallBuildingFire", "LargeBuildingFire" },
    { "SmallBuildingFire", "LargeBuildingFire" },
};

static onFireNames_t const onfire_names[] = {
    [kPlayerRaceUndead] = { { "UndeadBuildingFire", "UndeadBuildingFire" },
                            { "UndeadSmallBuildingFire", "UndeadLargeBuildingFire" } },
    [kPlayerRaceNightElf] = { { "NightElfBuildingFire", "NightElfBuildingFire" },
                              { "ElfSmallBuildingFire", "ElfLargeBuildingFire" } },
};

static onFireStage_t const onfire_stage[] = {
    { 0, 2, EFX_SLOT_FIRST | EFX_SLOT_SECOND },
    { 1, 2, EFX_SLOT_FIRST | EFX_SLOT_SECOND | EFX_SLOT_FOURTH | EFX_SLOT_FIFTH },
    { 1, 1, EFX_SLOT_FIRST | EFX_SLOT_SECOND | EFX_SLOT_THIRD | EFX_SLOT_FOURTH | EFX_SLOT_FIFTH },
};

static DWORD onfire_race(LPCSTR name) {
    static struct { LPCSTR name; DWORD race; } const races[] = {
        { STR_HUMAN, kPlayerRaceHuman }, { STR_ORC, kPlayerRaceOrc },
        { STR_UNDEAD, kPlayerRaceUndead }, { STR_NIGHTELF, kPlayerRaceNightElf },
    };

    if (name) FOR_LOOP(i, sizeof(races) / sizeof(*races))
        if (!strcmp(name, races[i].name)) return races[i].race;
    return kPlayerRaceNone;
}

/* Select the only authored race-specific families; Human, Orc, and unknown data use standard assets. */
static onFireNames_t const *onfire_family(DWORD race) {
    if (race == kPlayerRaceUndead || race == kPlayerRaceNightElf) return onfire_names + race;
    return &onfire_standard;
}

static DWORD onfire_level(LPCEDICT ent) {
    BYTE health;

    if (!ent->inuse || !(ent->s.flags & EF_BUILDING) || ent->health.value <= 0.0f ||
        !ent->health.max_value || ent->construction.active) return 0;
    health = compress_stat(&ent->health);
    if (health > 255 * 3 / 4) return 0;
    if (health > 255 / 2) return 1;
    if (health > 255 / 4) return 2;
    return 3;
}

static void onfire_disabled(LPEDICT ent) {
    ent->s.effect = 0;
    ent->s.effect_flags &= EFX_TEAM_COLOR_MASK;
}

/* Apply one of four fire levels: off, small, medium, or severe. */
static void onfire_level_changed(LPEDICT ent, DWORD level) {
    UnitData_t const *data;
    onFireNames_t const *names;
    onFireStage_t const *stage;
    PATHSTR path;
    DWORD race;

    if (!level) {
        onfire_disabled(ent);
        return;
    }
    data = G_UnitData(ent->class_id);
    race = onfire_race(data ? data->race : NULL);
    names = onfire_family(race);
    stage = onfire_stage + level - 1;
    if ((ent->s.effect_flags & EFX_ATTACH_SLOTS) &&
        (ent->s.effect_flags & EFX_SLOT_MASK) == stage->slots) return;
    onfire_disabled(ent);
    snprintf(path, sizeof(path), "Environment\\%s\\%s%d.mdx", names->dir[stage->size],
             names->prefix[stage->size], stage->variant);
    ent->s.effect = G_RegisterModel(path);
    if (!ent->s.effect) {
        fprintf(stderr, "onfire_level_changed: failed to register %s\n", path);
        return;
    }
    ent->s.effect_flags = (ent->s.effect_flags & EFX_TEAM_COLOR_MASK) |
        EFX_MODEL | EFX_ATTACH_SLOTS | stage->slots;
}

static void onfire_enabled(LPEDICT ent) { onfire_level_changed(ent, onfire_level(ent)); }

    /* Retail synthesizes one race-specific CAbilityOnFire for buildings instead of listing it in UnitAbilities.slk. */
    ability_t CAbilityOnFireHuman = {
        .flags = AB_PASSIVE,
        .enabled = onfire_enabled,
        .disabled = onfire_disabled,
        .level = onfire_level,
        .level_changed = onfire_level_changed,
    };
