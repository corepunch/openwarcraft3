#include "g_local.h"

#include "../common/wc3_save.h"

#define F_METADATA(kind, ...) F_METADATA_INNER(kind, __VA_ARGS__)
#define F_METADATA_INNER(kind, ...) F_METADATA_##kind(__VA_ARGS__)
#define F_METADATA_F_STRUCT(n, schema) .array_size = n, .child = schema
#define F_METADATA_F_IGNORE(n, bits) .array_size = n, .flags = bits
#define F_METADATA_F_INT(...) .flags = 0
#define F_METADATA_F_FLOAT(...) .flags = 0
#define F_METADATA_F_LSTRING(...) .flags = 0
#define F_METADATA_F_GSTRING(...) .flags = 0
#define F_METADATA_F_VECTOR(...) .flags = 0
#define F_METADATA_F_REGION(...) .flags = 0
#define F_METADATA_F_ANGLEHACK(...) .flags = 0
#define F_METADATA_F_FUNCTION(...) .flags = 0
#define F_METADATA_F_FUNCTION_LIST(...) .flags = 0
#define F_METADATA_F_CFUNCTION(...) .flags = 0
#define F_METADATA_F_MMOVE(...) .flags = 0
#define F_METADATA_F_EDICT(n, bits) .array_size = n, .flags = bits
#define F_METADATA_F_ITEM(n, bits) .array_size = n, .flags = bits
#define F_METADATA_F_TRIGGER(n, bits) .array_size = n, .flags = bits
#define F_METADATA_F_TIMER(n, bits) .array_size = n, .flags = bits
#define F_METADATA_F_EVENT(n, bits) .array_size = n, .flags = bits
#define F(T, x, k, ...) { .name = #x, .ofs = offsetof(struct T, x), .type = k, .size = sizeof(((struct T *)0)->x), F_METADATA(k, ##__VA_ARGS__), .count_ofs = UINT32_MAX }
#define TF(T, x, k, ...) { .name = #x, .ofs = offsetof(T, x), .type = k, .size = sizeof(((T *)0)->x), F_METADATA(k, ##__VA_ARGS__), .count_ofs = UINT32_MAX }
#define FC(T, x, k, n, schema, cnt) BZ_SAVE_COUNTED(struct T, x, n, schema, cnt)
#define FR(T, x, n, r) { .name = #x, .ofs = offsetof(struct T, x), .type = F_STRUCT_RING, .size = sizeof(((struct T *)0)->x), .array_size = n, .ring = r, .count_ofs = UINT32_MAX }
#define TFC(T, x, k, n, cnt) { .name = #x, .ofs = offsetof(T, x), .type = k, .size = sizeof(((T *)0)->x), .array_size = n, .count_ofs = offsetof(T, cnt) }

enum {
    FIELD_NONE,
    FIELD_RUNTIME = 1 << 0,
};

static DWORD const save_magic = MAKEFOURCC('W', '3', 'S', 'V');
static DWORD const save_version = 16; // camera targets now use client image fixups; removes the separate target stream
#define MAX_SAVE_STRING (1u << 20) // bytes; bounds quest-string allocations from corrupt saves
#define MAX_SAVE_GROUP_HANDLES 65536u // corrupt-save bound only; runtime group registry itself grows dynamically
/* Move identities are append-only symbols, never addresses or executable-relative offsets. */
#define BZ_SAVE_MOVES(M) \
    M(static, wc3_effect_temp_birth) \
    M(static, wc3_effect_temp_stand) \
    M(static, wc3_effect_birth) \
    M(static, wc3_effect_stand) \
    M(static, wc3_effect_death) \
    M(static, item_move_pickup) \
    M(static, tree_move_birth) \
    M(static, tree_move_stand) \
    M(static, tree_move_pain) \
    M(static, tree_move_death) \
    M(static, unit_move_birth) \
    M(static, unit_move_stand) \
    M(static, unit_move_stand_ready) \
    M(static, unit_move_death) \
    M(static, unit_move_decay) \
    M(static, attack_move_walk) \
    M(static, attack_move_melee_cooldown) \
    M(static, attack_move_melee) \
    M(static, attack_move_ranged_cooldown) \
    M(static, attack_move_ranged) \
    M(static, attackmove_move_walk) \
    M(static, build_move_walk) \
    M(static, battlestations_move_walk) \
    M(static, harvestgold_move_walk) \
    M(static, harvestgold_move_walkback) \
    M(static, harvestgold_move_minegold) \
    M(static, harvestgold_move_wait) \
    M(static, harvest_move_walk) \
    M(static, harvest_move_walkback) \
    M(static, harvest_move_swing) \
    M(static, harvest_move_cooldown) \
    M(static, wisp_harvest_mine) \
    M(static, wisp_harvest_walk) \
    M(extern, holdpos_move_stand) \
    M(extern, holdpos_move_stand_ready) \
    M(static, move_heal) \
    M(static, militia_move_walk) \
    M(static, follow_move_walk) \
    M(static, move_move_hold) \
    M(static, move_move_walk) \
    M(static, patrol_move_walk) \
    M(static, repair_move_walk) \
    M(static, repair_move_work) \
    M(static, repair_generic_move_walk) \
    M(static, repair_generic_move_work) \
    M(static, repair_legacy_move_work) \
    M(static, thunderbolt_projectile_move) \
    M(static, firebolt_projectile_move) \
    M(static, spell_cast_move) \
    M(static, train_move_train)
#define BZ_DECLARE_MOVE(storage, name) storage umove_t name;
BZ_SAVE_MOVES(BZ_DECLARE_MOVE)
#undef BZ_DECLARE_MOVE

_Static_assert(sizeof(void *) == 8, "Native world images require 64-bit reference slots");

typedef struct {
    LPCSTR name;
    void *ptr;
} SAVESYMBOL;

#define SAVE_CFUNCTION(fn) { .name = #fn, .ptr = (void *)(fn) }

/* Append-only: the 1-based index is part of the save format. Reordering rejects older saves.
 * idle/move/run/attack have no production assignments; they still use F_CFUNCTION so a later
 * assignment must be rostered here or WriteGame fails instead of writing an ASLR address. */
static SAVESYMBOL const save_cfunctions[] = {
    SAVE_CFUNCTION(monster_think),
    SAVE_CFUNCTION(blight_mine_think),
    SAVE_CFUNCTION(G_FreeEdict),
    SAVE_CFUNCTION(G_EffectThink),
    SAVE_CFUNCTION(G_EffectValidateTarget),
    SAVE_CFUNCTION(blizzard_think),
    SAVE_CFUNCTION(flame_strike_tick),
    SAVE_CFUNCTION(siphon_mana_think),
    SAVE_CFUNCTION(unit_stand),
    SAVE_CFUNCTION(unit_birth),
    SAVE_CFUNCTION(unit_die),
    SAVE_CFUNCTION(tree_stand),
    SAVE_CFUNCTION(tree_birth),
    SAVE_CFUNCTION(tree_pain),
    SAVE_CFUNCTION(tree_die),
    SAVE_CFUNCTION(human_ability_think),
    {0}
};

#define BZ_MOVE_SYMBOL(storage, name) { #name, &name },
static SAVESYMBOL const save_moves[] = {
    BZ_SAVE_MOVES(BZ_MOVE_SYMBOL)
    {0}
};
#undef BZ_MOVE_SYMBOL
#undef BZ_SAVE_MOVES

typedef struct {
    DWORD magic, version, abi, edict_size, num_edicts, max_clients;
    DWORD script_identity, quests, groups, triggers, timers, events;
    PATHSTR map_path;
} SAVEHEADER;


typedef enum {
    JASS_HANDLE_ENTITY,
    JASS_HANDLE_PLAYER,
    JASS_HANDLE_QUEST,
    JASS_HANDLE_QUESTITEM,
    JASS_HANDLE_EVENT,
    JASS_HANDLE_TRIGGER,
    JASS_HANDLE_GROUP,
    JASS_HANDLE_TIMER,
    JASS_HANDLE_WEATHER,
} jassHandleDomain_t;

static struct { LPCSTR type; jassHandleDomain_t domain; } const jass_handle_domains[] = {
    { "unit", JASS_HANDLE_ENTITY },
    { "widget", JASS_HANDLE_ENTITY },
    { "destructable", JASS_HANDLE_ENTITY },
    { "item", JASS_HANDLE_ENTITY },
    { "effect", JASS_HANDLE_ENTITY },
    { "player", JASS_HANDLE_PLAYER },
    { "quest", JASS_HANDLE_QUEST },
    { "questitem", JASS_HANDLE_QUESTITEM },
    { "event", JASS_HANDLE_EVENT },
    { "trigger", JASS_HANDLE_TRIGGER },
    { "group", JASS_HANDLE_GROUP },
    { "timer", JASS_HANDLE_TIMER },
    { "weathereffect", JASS_HANDLE_WEATHER },
};


static field_t const weather_fields[] = {
    TF(gweather_t, inuse, F_INT),
    TF(gweather_t, enabled, F_INT),
    TF(gweather_t, handle_id, F_INT),
    TF(gweather_t, effect_id, F_INT),
    TF(gweather_t, bounds, F_VECTOR),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const save_event_fields[] = {
    F(gevent_s, type, F_INT),
    F(gevent_s, subject, F_EDICT, 0, FIELD_NONE),
    F(gevent_s, trigger, F_TRIGGER, 0, FIELD_NONE),
    F(gevent_s, timer, F_TIMER, 0, FIELD_NONE),
    F(gevent_s, region, F_REGION),
    F(gevent_s, range, F_FLOAT),
    F(gevent_s, state, F_INT),
    F(gevent_s, limitop, F_INT),
    F(gevent_s, limitval, F_FLOAT),
    F(gevent_s, inuse, F_INT),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const save_game_event_fields[] = {
    F(gameevent_s, type, F_INT),
    F(gameevent_s, edict, F_EDICT, 0, FIELD_NONE),
    F(gameevent_s, source, F_EDICT, 0, FIELD_NONE),
    F(gameevent_s, value, F_INT),
    F(gameevent_s, responseTo, F_EVENT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const group_fields[] = {
    /* handle_id is runtime identity derived from the table ordinal and is not serialized. */
    TF(ggroup_t, inuse, F_INT),
    TFC(ggroup_t, units, F_EDICT, MAX_GROUP_SIZE, num_units),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const trigger_fields[] = {
    F(gtrigger_s, disabled, F_INT),
    F(gtrigger_s, actions, F_FUNCTION_LIST),
    F(gtrigger_s, conditions, F_FUNCTION_LIST),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const timer_fields[] = {
    F(gtimer_s, duration, F_INT),
    F(gtimer_s, remaining, F_INT),
    F(gtimer_s, periodic, F_INT),
    F(gtimer_s, paused, F_INT),
    F(gtimer_s, running, F_INT),
    F(gtimer_s, handler, F_FUNCTION),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const questitem_fields[] = {
    F(gquestitem_s, description, F_LSTRING),
    F(gquestitem_s, completed, F_INT),
    F(gquestitem_s, inuse, F_INT),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const quest_fields[] = {
    F(gquest_s, title, F_LSTRING),
    F(gquest_s, description, F_LSTRING),
    F(gquest_s, iconPath, F_LSTRING),
    F(gquest_s, discovered, F_INT),
    F(gquest_s, required, F_INT),
    F(gquest_s, completed, F_INT),
    F(gquest_s, failed, F_INT),
    F(gquest_s, enabled, F_INT),
    F(gquest_s, inuse, F_INT),
    FC(gquest_s, items, F_STRUCT, MAX_QUESTITEMS, questitem_fields, num_items),
    { NULL, 0, 0, 0, 0, 0 }
};

static SAVERING const game_event_ring = {
    save_game_event_fields,
    FOFS(level_locals, events.read) - (HANDLE)NULL,
    FOFS(level_locals, events.write) - (HANDLE)NULL
};

static field_t const level_fields[] = {
    F(level_locals, framenum, F_INT),
    F(level_locals, time, F_INT),
    F(level_locals, timeofday.elapsed, F_FLOAT),
    F(level_locals, timeofday.pending, F_FLOAT),
    F(level_locals, timeofday.pending_valid, F_INT),
    F(level_locals, timeofday.suspended, F_INT),
    F(level_locals, timeofday.false_time.hour, F_INT),
    F(level_locals, timeofday.false_time.minute, F_INT),
    F(level_locals, timeofday.false_time.ticks_remaining, F_INT),
    F(level_locals, timeofday.false_time.active, F_INT),
    F(level_locals, timeofday.false_time.initialized, F_INT),
    F(level_locals, camera_bounds, F_VECTOR),
    F(level_locals, started, F_INT),
    F(level_locals, scriptsStarted, F_INT),
    F(level_locals, waypoints.base, F_INT),
    F(level_locals, waypoints.cursor, F_INT),
    F(level_locals, waypoints.count, F_INT),
    F(level_locals, next_weather_id, F_INT),
    F(level_locals, weather_effects, F_STRUCT, MAX_WEATHER_EFFECTS, weather_fields),
    F(level_locals, quests, F_STRUCT, MAX_QUESTS, quest_fields),
    FC(level_locals, triggers, F_STRUCT, MAX_TRIGGERS, trigger_fields, num_triggers),
    FC(level_locals, timers, F_STRUCT, MAX_TIMERS, timer_fields, num_timers),
    F(level_locals, events.handlers, F_STRUCT, MAX_EVENTS, save_event_fields),
    FR(level_locals, events.queue, MAX_EVENT_QUEUE, &game_event_ring),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const entity_state_fields[] = {
    TF(entityState_t, origin, F_VECTOR),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const link_fields[] = {
    F(link_s, prev, F_IGNORE, 0, FIELD_RUNTIME),
    F(link_s, next, F_IGNORE, 0, FIELD_RUNTIME),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const construction_fields[] = {
    TF(edictConstruction_s, primary_builder, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const rally_fields[] = {
    TF(edictRally_s, entity, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const revival_fields[] = {
    TF(edictRevival_s, producer, F_EDICT, 0, FIELD_NONE),
    TF(edictRevival_s, queue_next, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const militia_fields[] = {
    TF(edictMilitia_s, partner, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictMilitia_s, partner_spawn_time, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictMilitia_s, returning, F_IGNORE, 0, FIELD_RUNTIME),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const goldmine_fields[] = {
    TF(edictGoldMine_s, mine, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const item_fields[] = {
    TF(edictItem_s, carrier, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const destructable_fields[] = {
    TF(edictDestructable_s, alive_pathtex, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictDestructable_s, death_pathtex, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictDestructable_s, drop_sets, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictDestructable_s, drop_sets_count, F_IGNORE, 0, FIELD_RUNTIME),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const cargo_fields[] = {
    TF(edictCargo_s, units, F_EDICT, MAX_CARGO, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const abilities_fields[] = {
    TFC(edictAbilities_s, added, F_INT, MAX_ABILITIES, added_count),
    TFC(edictAbilities_s, removed, F_INT, MAX_ABILITIES, removed_count),
    TFC(edictAbilities_s, permanent, F_INT, MAX_ABILITIES, permanent_count),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const avatar_fields[] = {
    TF(struct edictAvatar_s, level, F_INT),
    TF(struct edictAvatar_s, armor, F_FLOAT),
    TF(struct edictAvatar_s, health, F_FLOAT),
    TF(struct edictAvatar_s, damage, F_INT),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const movement_fields[] = {
    TF(edictMovement_s, attackmove_waypoint, F_EDICT, 0, FIELD_NONE),
    TF(edictMovement_s, patrol_a, F_EDICT, 0, FIELD_NONE),
    TF(edictMovement_s, patrol_b, F_EDICT, 0, FIELD_NONE),
    TF(edictMovement_s, patrol_target, F_EDICT, 0, FIELD_NONE),
    TF(edictMovement_s, follow_target, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const edict_data_fields[] = {
    TF(edictData_s, UnitProfile, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, UnitBalance, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, UnitData, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, UnitUI, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, UnitWeapons, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, UnitAbilities, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, Doodads, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, ItemData, F_IGNORE, 0, FIELD_RUNTIME),
    TF(edictData_s, DestructableData, F_IGNORE, 0, FIELD_RUNTIME),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const client_menu_fields[] = {
    TF(clientMenu_s, on_entity_selected, F_IGNORE, 0, FIELD_RUNTIME),
    TF(clientMenu_s, on_location_selected, F_IGNORE, 0, FIELD_RUNTIME),
    TF(clientMenu_s, cmdbutton, F_IGNORE, 0, FIELD_RUNTIME),
    TF(clientMenu_s, refresh, F_IGNORE, 0, FIELD_RUNTIME),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const client_camera_fields[] = {
    TF(clientCamera_s, target_controller, F_EDICT, 0, FIELD_NONE),
    { NULL, 0, 0, 0, 0, 0 }
};

/* Every persistent and process-owned edict field crossing the save boundary is represented here. */
field_t edict_fields[] = {
    F(edict_s, class_id, F_INT),
    F(edict_s, variation, F_INT),
    F(edict_s, build_project, F_INT),
    F(edict_s, spawn_time, F_INT),
    F(edict_s, harvested_lumber, F_INT),
    F(edict_s, harvested_gold, F_INT),
    F(edict_s, heatmap2, F_INT),
    F(edict_s, peonsinside, F_INT),
    F(edict_s, aiflags, F_INT),
    F(edict_s, autocast_code, F_INT),
    F(edict_s, damage, F_INT),
    F(edict_s, collision, F_FLOAT),
    F(edict_s, s, F_STRUCT, 1, entity_state_fields),
    F(edict_s, construction, F_STRUCT, 1, construction_fields),
    F(edict_s, rally, F_STRUCT, 1, rally_fields),
    F(edict_s, revival, F_STRUCT, 1, revival_fields),
    F(edict_s, goldmine, F_STRUCT, 1, goldmine_fields),
    F(edict_s, inventory, F_EDICT, MAX_INVENTORY, FIELD_NONE),
    F(edict_s, cargo, F_STRUCT, 1, cargo_fields),
    F(edict_s, item, F_STRUCT, 1, item_fields),
    F(edict_s, ground_next, F_EDICT, 0, FIELD_NONE),
    F(edict_s, movement, F_STRUCT, 1, movement_fields),
    F(edict_s, goalentity, F_EDICT, 0, FIELD_NONE),
    F(edict_s, combatentity, F_EDICT, 0, FIELD_NONE),
    F(edict_s, secondarygoal, F_EDICT, 0, FIELD_NONE),
    F(edict_s, owner, F_EDICT, 0, FIELD_NONE),
    F(edict_s, build, F_EDICT, 0, FIELD_NONE),
    F(edict_s, client, F_IGNORE, 0, FIELD_RUNTIME),
    F(edict_s, pathtex, F_IGNORE, 0, FIELD_RUNTIME),
    F(edict_s, area, F_STRUCT, 1, link_fields),
    F(edict_s, destructable, F_STRUCT, 1, destructable_fields),
    F(edict_s, abilities, F_STRUCT, 1, abilities_fields),
    F(edict_s, avatar, F_STRUCT, 1, avatar_fields),
    F(edict_s, temporary_health_bonus, F_FLOAT),
    F(edict_s, animation, F_IGNORE, 0, FIELD_RUNTIME),
    F(edict_s, currentmove, F_MMOVE),
    F(edict_s, militia, F_STRUCT, 1, militia_fields),
    F(edict_s, stand, F_CFUNCTION),
    F(edict_s, birth, F_CFUNCTION),
    F(edict_s, prethink, F_CFUNCTION),
    F(edict_s, think, F_CFUNCTION),
    F(edict_s, die, F_CFUNCTION),
    F(edict_s, idle, F_CFUNCTION),
    F(edict_s, move, F_CFUNCTION),
    F(edict_s, run, F_CFUNCTION),
    F(edict_s, attack, F_CFUNCTION),
    F(edict_s, pain, F_CFUNCTION),
    F(edict_s, data, F_STRUCT, 1, edict_data_fields),
    { NULL, 0, 0, 0, 0, 0 }
};

static field_t const client_fields[] = {
    F(client_s, ps.name, F_IGNORE, 0, FIELD_RUNTIME),
    F(client_s, ps.texts, F_IGNORE, 0, FIELD_RUNTIME),
    F(client_s, mapplayer, F_IGNORE, 0, FIELD_RUNTIME),
    /* Modal ownership is live client-window/session state. Persisting it from
     * an Esc-menu save can reload a client as paused without a live window. */
    F(client_s, modal_flags, F_IGNORE, 0, FIELD_RUNTIME),
    F(client_s, quest_dialog_open, F_IGNORE, 0, FIELD_RUNTIME),
    F(client_s, menu, F_STRUCT, 1, client_menu_fields),
    F(client_s, camera, F_STRUCT, 1, client_camera_fields),
    F(client_s, rally_indicator, F_IGNORE, 0, FIELD_RUNTIME),
    { NULL, 0, 0, 0, 0, 0 }
};

/* Native images have a declared revision plus producer layout/byte-order constraints. */
static DWORD SaveABI(void) {
    DWORD const layout[] = { sizeof(void *), sizeof(size_t), sizeof(GAMECLIENT), sizeof(level), sizeof(edict_t), 0x01020304 };
    return save_hash(0, layout, sizeof(layout));
}

static BOOL WriteJassBytes(void *context, void *data, DWORD size) { return save_bytes(context, data, size); }
static BOOL ReadJassBytes(void *context, void *data, DWORD size) { return load_bytes(context, data, size); }
static BOOL WriteMappedFields(LPSTATEBUFFER f, field_t const *fields, BYTE *base);
static BOOL ReadMappedFields(LPSTATEBUFFER f, field_t const *fields, BYTE *base);
static BOOL WriteString(LPSTATEBUFFER f, LPCSTR text);
static BOOL ReadString(LPSTATEBUFFER f, LPSTR *text);
static DWORD ActiveEventCount(void);

/* Save files carry the canonical map path so the server can rebuild the map before restoring state. */
BOOL G_SaveMap(LPSTATEBUFFER f, LPSTR map, DWORD map_size) {
    SAVEHEADER header;
    f->pos = 0;
    BOOL ok = map && map_size && load_bytes(f, &header, sizeof(header)) && header.magic == save_magic &&
        header.version == save_version && header.abi == SaveABI() && header.edict_size == sizeof(edict_t) && header.map_path[0] &&
        memchr(header.map_path, 0, sizeof(header.map_path));
    if (ok) strlcpy(map, header.map_path, map_size);
    f->pos = 0;
    return ok;
}

/* Engine slot metadata selects the map; game layout compatibility must also precede teardown. */
BOOL G_CheckState(LPSTATEBUFFER buf) {
    PATHSTR map;
    BOOL ok = G_SaveMap(buf, map, sizeof(map));
    if (!ok) fprintf(stderr, "WC3 LoadGame: incompatible world-state header\n");
    return ok;
}

void G_ClearSaveRegistries(void) {
    FOR_LOOP(i, level.num_triggers) {
        DELETE_LIST(TRIGGERACTION, level.triggers[i].actions, gi.MemFree);
        DELETE_LIST(TRIGGERCONDITION, level.triggers[i].conditions, gi.MemFree);
    }
}

static BOOL RestoreRegistrySlots(DWORD groups, DWORD timers, DWORD triggers, DWORD events) {
    if (groups < level.num_groups || timers < level.num_timers || triggers < level.num_triggers ||
        events < ActiveEventCount() || groups > MAX_SAVE_GROUP_HANDLES || timers > MAX_TIMERS ||
        triggers > MAX_TRIGGERS || events > MAX_EVENTS)
        return false;
    if (!G_EnsureJassGroupSlots(groups)) return false;
    while (level.num_timers < timers) if (!G_AllocJassTimer()) return false;
    while (level.num_triggers < triggers) if (!G_AllocJassTrigger()) return false;
    while (ActiveEventCount() < events) if (!G_MakeEvent(0)) return false;
    return true;
}

/* VM state follows native domains so load-side handle relocation sees restored objects. */
static BOOL WriteJass(LPSTATEBUFFER f) {
    BOOL present = level.vm != NULL;
    JASSSNAPSHOT snapshot = { f, WriteJassBytes };
    return save_bytes(f, &present, sizeof(present)) && (!present || jass_writesnapshot(level.vm, &snapshot));
}

static BOOL ReadJass(LPSTATEBUFFER f) {
    BOOL present;
    JASSSNAPSHOT snapshot = { f, ReadJassBytes };
    if (!load_bytes(f, &present, sizeof(present)) || present > 1 || present != (level.vm != NULL)) {
        fprintf(stderr, "WC3 LoadGame: JASS VM lifecycle does not match save\n");
        return false;
    }
    return !present || jass_readsnapshot(level.vm, &snapshot);
}

static DWORD ActiveQuestCount(void) {
    DWORD count = 0;
    FOR_EACH_QUEST(quest) count++;
    return count;
}

static DWORD ActiveEventCount(void) {
    DWORD count = 0;
    FOR_EACH_EVENT(event) count++;
    return count;
}

static BOOL EventId(LPEVENT value, DWORD *id) {
    if (!value) { *id = UINT32_MAX; return true; }
    if (value >= level.events.handlers && value < level.events.handlers + MAX_EVENTS) {
        *id = (DWORD)(value - level.events.handlers); return value->inuse;
    }
    return false;
}

static LPEVENT EventById(DWORD id) {
    return id < MAX_EVENTS && level.events.handlers[id].inuse ? &level.events.handlers[id] : NULL;
}

static BOOL TriggerIndex(LPTRIGGER value, DWORD *id) {
    if (!value) { *id = UINT32_MAX; return true; }
    if (value < level.triggers || value >= level.triggers + level.num_triggers) return false;
    *id = (DWORD)(value - level.triggers); return true;
}

static BOOL TimerIndex(LPGTIMER value, DWORD *id) {
    if (!value) { *id = UINT32_MAX; return true; }
    if (value < level.timers || value >= level.timers + level.num_timers) return false;
    *id = (DWORD)(value - level.timers); return true;
}

static DWORD TriggerCodeCount(TRIGGERACTION const *list) {
    DWORD n = 0;
    for (; list; list = list->next) n++;
    return n;
}

static BOOL WriteTriggerCodeList(LPSTATEBUFFER f, TRIGGERACTION const *list) {
    DWORD n = TriggerCodeCount(list);
    if (!save_bytes(f, &n, sizeof(n))) return false;
    for (; list; list = list->next) if (!WriteString(f, jass_functionname(list->func))) return false;
    return true;
}

static BOOL ReadTriggerCodeList(LPSTATEBUFFER f, TRIGGERACTION **list) {
    DWORD n;
    TRIGGERACTION **tail;
    if (!load_bytes(f, &n, sizeof(n))) return false;
    DELETE_LIST(TRIGGERACTION, *list, gi.MemFree);
    *list = NULL;
    tail = list;
    FOR_LOOP(i, n) {
        LPSTR name = NULL;
        TRIGGERACTION *item = gi.MemAlloc(sizeof(*item));
        if (!item || !ReadString(f, &name)) { free(name); if (item) gi.MemFree(item); return false; }
        item->func = name ? jass_functionbyname(level.vm, name) : NULL;
        if (name && !item->func) { free(name); gi.MemFree(item); return false; }
        free(name);
        *tail = item;
        tail = &item->next;
    }
    return true;
}

static BOOL JassHandleDomain(LPCSTR type, jassHandleDomain_t *domain) {
    FOR_LOOP(i, sizeof(jass_handle_domains) / sizeof(*jass_handle_domains)) {
        if (!strcmp(type, jass_handle_domains[i].type)) { *domain = jass_handle_domains[i].domain; return true; }
    }
    return false;
}

static HANDLE JassListHandle(jassHandleDomain_t domain, DWORD id) {
    DWORD index = 0;
    if (domain == JASS_HANDLE_QUEST) {
        if (id < MAX_QUESTS && level.quests[id].inuse) return &level.quests[id];
    } else if (domain == JASS_HANDLE_QUESTITEM) {
        FOR_EACH_QUEST(quest)
            FOR_EACH_QUESTITEM(quest, item) if (index++ == id) return item;
    } else if (domain == JASS_HANDLE_EVENT) {
        return EventById(id);
    } else if (domain == JASS_HANDLE_WEATHER) {
        if (id < MAX_WEATHER_EFFECTS && level.weather_effects[id].inuse) return &level.weather_effects[id];
    } else if (domain == JASS_HANDLE_TRIGGER && id < level.num_triggers) return &level.triggers[id];
    else if (domain == JASS_HANDLE_TIMER && id < level.num_timers) return &level.timers[id];
    return NULL;
}

/* Native pointers cross the save boundary only through stable domain-specific indexes. */
BOOL G_SaveJassHandle(LPCSTR type, HANDLE value, DWORD *id) {
    jassHandleDomain_t domain;
    DWORD index = 0;
    if (!JassHandleDomain(type, &domain) || !value) return false;
    if (domain == JASS_HANDLE_ENTITY) {
        LPEDICT ent = value;
        uintptr_t ptr = (uintptr_t)ent, base = (uintptr_t)g_edicts;
        if (ptr < base || ptr >= base + sizeof(*g_edicts) * globals.num_edicts || (ptr - base) % sizeof(*g_edicts)) {
            fprintf(stderr, "WC3 SaveGame: %s handle %p outside edict table [%p, %p)\n", type, value,
                (void *)g_edicts, (void *)(g_edicts + globals.num_edicts));
            return false;
        }
        if (!ent->inuse) {
            fprintf(stderr, "WC3 SaveGame: %s handle %p is unused edict %ld\n", type, value, (long)(ent - g_edicts));
            return false;
        }
        *id = (DWORD)(ent - g_edicts); return true;
    }
    if (domain == JASS_HANDLE_PLAYER) {
        FOR_LOOP(i, game.max_clients) if (value == &game.clients[i].ps) { *id = i; return true; }
        return false;
    }
    if (domain == JASS_HANDLE_GROUP) {
        if (!G_JassGroupValid(value)) return false;
        return G_JassGroupIndex(value, id);
    }
    if (domain == JASS_HANDLE_TIMER) {
        return TimerIndex(value, id);
    }
    if (domain == JASS_HANDLE_WEATHER) {
        LPGWEATHER effect = value;
        if (effect < level.weather_effects || effect >= level.weather_effects + MAX_WEATHER_EFFECTS || !effect->inuse)
            return false;
        *id = (DWORD)(effect - level.weather_effects);
        return true;
    }
    if (domain == JASS_HANDLE_QUEST) {
        if ((LPQUEST)value >= level.quests && (LPQUEST)value < level.quests + MAX_QUESTS && ((LPQUEST)value)->inuse) {
            *id = (DWORD)((LPQUEST)value - level.quests); return true;
        }
        return false;
    }
    if (domain == JASS_HANDLE_QUESTITEM) {
        FOR_EACH_QUEST(quest)
            FOR_EACH_QUESTITEM(quest, item) { if (item == value) { *id = index; return true; } index++; }
        return false;
    }
    if (domain == JASS_HANDLE_EVENT) {
        return EventId(value, id);
    }
    return TriggerIndex(value, id);
}

HANDLE G_LoadJassHandle(LPCSTR type, DWORD id) {
    jassHandleDomain_t domain;
    if (!JassHandleDomain(type, &domain)) return NULL;
    if (domain == JASS_HANDLE_ENTITY) return id < globals.num_edicts && g_edicts[id].inuse ? g_edicts + id : NULL;
    if (domain == JASS_HANDLE_PLAYER) return id < (DWORD)game.max_clients ? &game.clients[id].ps : NULL;
    if (domain == JASS_HANDLE_GROUP) {
        ggroup_t *group = G_JassGroupByIndex(id);
        return group && group->inuse ? group : NULL;
    }
    if (domain == JASS_HANDLE_TIMER) return id < level.num_timers ? &level.timers[id] : NULL;
    return JassListHandle(domain, id);
}

static BOOL WriteString(LPSTATEBUFFER f, LPCSTR text) {
    size_t size = text ? strlen(text) + 1 : 0;
    DWORD len;

    if (size > MAX_SAVE_STRING) return false;
    len = (DWORD)size;
    return save_bytes(f, &len, sizeof(len)) && (!len || save_bytes(f, text, len));
}

static BOOL ReadString(LPSTATEBUFFER f, LPSTR *text) {
    DWORD len;
    LPSTR value = NULL;

    if (!load_bytes(f, &len, sizeof(len)) || len > MAX_SAVE_STRING) return false;
    if (len) {
        value = malloc(len);
        if (!value || !load_bytes(f, value, len) || value[len - 1]) { free(value); return false; }
    }
    free(*text); *text = value;
    return true;
}

/* Both callback and move slots contain an index and name hash while on disk. */
static BOOL SaveSymbol(BOOL reading, void *ptr, SAVESYMBOL const *table) {
    DWORD id[2] = {0};
    if (reading) {
        memcpy(id, ptr, sizeof(id));
        if (!id[0] && !id[1]) { *(void **)ptr = NULL; return true; }
        for (DWORD i = 0; table[i].name; i++) {
            if (i + 1 != id[0]) continue;
            if (save_hash(0, table[i].name, strlen(table[i].name) + 1) != id[1]) return false;
            *(void **)ptr = table[i].ptr;
            return true;
        }
    } else {
        void *value = *(void **)ptr;
        if (!value) { memset(ptr, 0, sizeof(id)); return true; }
        for (DWORD i = 0; table[i].name; i++) {
            if (table[i].ptr != value) continue;
            id[0] = i + 1; id[1] = save_hash(0, table[i].name, strlen(table[i].name) + 1);
            memcpy(ptr, id, sizeof(id));
            return true;
        }
    }
    fprintf(stderr, "WC3 state: unresolved %s symbol\n", reading ? "saved" : "live");
    return false;
}

/* Convert one schema pointer to its stable save-domain index without mutating the live object. */
static BOOL WriteMappedIndex(field_t const *field, void *ptr, int *index) {
    switch (field->type) {
    case F_EDICT:
    case F_ITEM: {
        LPEDICT value = *(LPEDICT *)ptr;
        uintptr_t addr = (uintptr_t)value, base = (uintptr_t)g_edicts;
        if (value && (addr < base || addr >= base + sizeof(*g_edicts) * globals.num_edicts ||
            (addr - base) % sizeof(*g_edicts))) return false;
        *index = value ? (int)(value - g_edicts) : -1; return true;
    }
    case F_TRIGGER: {
        DWORD id;
        if (!TriggerIndex(*(LPTRIGGER *)ptr, &id)) return false;
        *index = id == UINT32_MAX ? -1 : (int)id; return true;
    }
    case F_TIMER: {
        DWORD id;
        if (!TimerIndex(*(LPGTIMER *)ptr, &id)) return false;
        *index = id == UINT32_MAX ? -1 : (int)id; return true;
    }
    case F_EVENT: {
        DWORD id;
        if (!EventId(*(LPEVENT *)ptr, &id)) return false;
        *index = id == UINT32_MAX ? -1 : (int)id; return true;
    }
    default: return false;
    }
}

/* Resolve one schema index directly into the pointer domain declared by its field type. */
static BOOL ReadMappedIndex(field_t const *field, void *ptr, int index) {
    if (index < -1) return false;
    switch (field->type) {
    case F_EDICT:
    case F_ITEM:
        if (index >= (int)globals.max_edicts) return false;
        *(LPEDICT *)ptr = index < 0 ? NULL : g_edicts + index; return true;
    case F_TRIGGER:
        if (index >= (int)level.num_triggers) return false;
        *(LPTRIGGER *)ptr = index < 0 ? NULL : &level.triggers[index]; return true;
    case F_TIMER:
        if (index >= (int)level.num_timers) return false;
        *(LPGTIMER *)ptr = index < 0 ? NULL : &level.timers[index]; return true;
    case F_EVENT:
        if (index >= (int)ActiveEventCount()) return false;
        *(LPEVENT *)ptr = index < 0 ? NULL : EventById(index); return true;
    default: return false;
    }
}

/* One identity codec serves mapped payloads and in-place fixups in copied native images. */
static BOOL GameSaveField(LPSAVEIO io, field_t const *field, BYTE *base) {
    LPSTATEBUFFER f = io->buf;
    DWORD count = field->count_ofs != UINT32_MAX ? *(DWORD *)(base + field->count_ofs) :
        field->array_size ? field->array_size : 1;
    size_t size = field->array_size ? field->size / field->array_size : field->size;
    FOR_LOOP(i, count) {
        void *ptr = base + field->ofs + i * size;
        switch (field->type) {
        case F_CFUNCTION: case F_MMOVE:
            if (!io->image || !SaveSymbol(io->reading, ptr, field->type == F_MMOVE ? save_moves : save_cfunctions)) return false;
            break;
        case F_FUNCTION_LIST:
            if (!(io->reading ? ReadTriggerCodeList(f, ptr) : WriteTriggerCodeList(f, *(TRIGGERACTION **)ptr))) return false;
            break;
        case F_FUNCTION: {
            LPSTR name = NULL;
            if (!io->reading) {
                if (!WriteString(f, jass_functionname(*(LPCJASSFUNC *)ptr))) return false;
            } else {
                if (!ReadString(f, &name)) return false;
                *(LPCJASSFUNC *)ptr = name ? jass_functionbyname(level.vm, name) : NULL;
                BOOL ok = !name || *(LPCJASSFUNC *)ptr;
                free(name);
                if (!ok) return false;
            }
            break;
        }
        case F_LSTRING: case F_GSTRING:
            if (!(io->reading ? ReadString(f, ptr) : WriteString(f, *(LPCSTR *)ptr))) return false;
            break;
        case F_EDICT: case F_ITEM: case F_TRIGGER: case F_TIMER: case F_EVENT: {
            int index;
            if (io->reading) {
                if (io->image) memcpy(&index, ptr, sizeof(index));
                else if (!load_bytes(f, &index, sizeof(index))) return false;
                if (!ReadMappedIndex(field, ptr, index)) return false;
            } else {
                if (!WriteMappedIndex(field, ptr, &index)) return false;
                if (io->image) { memset(ptr, 0, size); memcpy(ptr, &index, sizeof(index)); }
                else if (!save_bytes(f, &index, sizeof(index))) return false;
            }
            break;
        }
        default: return false;
        }
    }
    return true;
}

static BOOL WriteMappedFields(LPSTATEBUFFER f, field_t const *fields, BYTE *base) {
    SAVEIO io = { .buf = f, .special = GameSaveField };
    return save_fields(&io, fields, base);
}

static BOOL ReadMappedFields(LPSTATEBUFFER f, field_t const *fields, BYTE *base) {
    SAVEIO io = { .buf = f, .reading = true, .special = GameSaveField };
    return save_fields(&io, fields, base);
}

static BOOL WriteGroups(LPSTATEBUFFER f) {
    FOR_LOOP(i, level.num_groups) {
        ggroup_t *group = G_JassGroupByIndex(i);
        if (!group || !WriteMappedFields(f, group_fields, (BYTE *)group)) {
            fprintf(stderr, "WC3 SaveGame: failed at group %u\n", (unsigned)i);
            return false;
        }
    }
    return true;
}

static BOOL ReadGroups(LPSTATEBUFFER f, DWORD count) {
    if (!G_EnsureJassGroupSlots(count)) return false;
    level.first_free_group = count;
    FOR_LOOP(i, count) {
        ggroup_t *group = G_JassGroupByIndex(i);
        if (!group || !ReadMappedFields(f, group_fields, (BYTE *)group)) {
            fprintf(stderr, "WC3 LoadGame: failed at group %u\n", (unsigned)i);
            return false;
        }
        group->handle_id = i;
        if (!group->inuse) {
            group->num_units = 0;
            if (i < level.first_free_group) level.first_free_group = i;
        }
    }
    return true;
}

static BOOL WriteEdict(LPSTATEBUFFER f, LPCEDICT ent) {
    SAVEIO io = { .buf = f, .special = GameSaveField };
    return state_image(&io, &MAKE(STATEBLOCK, .data = (void *)ent, .size = sizeof(*ent), .fields = edict_fields));
}

static BOOL WriteClient(LPSTATEBUFFER f, LPCGAMECLIENT client) {
    SAVEIO io = { .buf = f, .special = GameSaveField };
    return state_image(&io, &MAKE(STATEBLOCK, .data = (void *)client, .size = sizeof(*client), .fields = client_fields));
}

/* Client references use the same schema as edicts; only derived views of inline text need rebinding. */
static BOOL ReadClient(LPSTATEBUFFER f, LPGAMECLIENT client) {
    SAVEIO io = { .buf = f, .reading = true, .special = GameSaveField };
    if (!state_image(&io, &MAKE(STATEBLOCK, .data = client, .size = sizeof(*client), .fields = client_fields))) return false;
    client->ps.name = client->jass.name;
    FOR_LOOP(i, PLAYERTEXT_COUNT) client->ps.texts[i] = client->playerTextCursor[i] ?
        client->playerTextStorage[i][client->playerTextCursor[i] & PLAYER_TEXT_MASK] : NULL;
    client->mapplayer = level.mapinfo && client->ps.number < MAX_PLAYERS ? level.mapinfo->players + client->ps.number : NULL;
    return true;
}

static BOOL ReadEdict(LPSTATEBUFFER f, LPEDICT ent) {
    SAVEIO io = { .buf = f, .reading = true, .special = GameSaveField };
    if (!state_image(&io, &MAKE(STATEBLOCK, .data = ent, .size = sizeof(*ent), .fields = edict_fields))) return false;
    /* Table rows are process-owned; C callbacks already came back through F_CFUNCTION. */
    if (ent->class_id) {
        G_BindEntityData(ent);
        /* animation is a process-owned model pointer and is deliberately not serialized.
         * Re-resolve it from the persisted logical request plus per-unit animation tags. */
        if (ent->animation_request[0])
            ent->animation = G_GetUnitAnimation(ent, ent->animation_request);
    }
    return true;
}

BOOL G_WriteState(LPSTATEBUFFER f) {
    SAVEHEADER header = {
        .magic = save_magic, .version = save_version, .abi = SaveABI(), .edict_size = sizeof(edict_t), .num_edicts = globals.num_edicts,
        .max_clients = game.max_clients, .script_identity = level.vm ? jass_programidentity(level.vm) : 0,
        .quests = ActiveQuestCount(), .groups = level.num_groups, .triggers = level.num_triggers, .timers = level.num_timers,
        .events = ActiveEventCount()
    };
    strlcpy(header.map_path, level.map_path, sizeof(header.map_path));

    if (level.num_groups > MAX_SAVE_GROUP_HANDLES) {
        fprintf(stderr, "WC3 SaveGame: group handle count %u exceeds save safety bound %u\n",
                (unsigned)level.num_groups, (unsigned)MAX_SAVE_GROUP_HANDLES);
        return false;
    }
    BOOL ok = false;
    if (!save_bytes(f, &header, sizeof(header))) { fprintf(stderr, "WC3 SaveGame: failed at header\n"); goto done; }
    if (!WriteMappedFields(f, level_fields, (BYTE *)&level)) {
        fprintf(stderr, "WC3 SaveGame: failed at level fields\n"); goto done;
    }
    if (!WriteGroups(f)) goto done;
    FOR_LOOP(i, game.max_clients) {
        if (!WriteClient(f, game.clients + i)) { fprintf(stderr, "WC3 SaveGame: failed at client %d\n", i); goto done; }
    }
    FOR_LOOP(i, globals.num_edicts) {
        BOOL used = g_edicts[i].inuse;
        if (!save_bytes(f, &used, sizeof(used))) { fprintf(stderr, "WC3 SaveGame: failed at edict %d inuse\n", i); goto done; }
        if (used && !save_bytes(f, &i, sizeof(i))) { fprintf(stderr, "WC3 SaveGame: failed at edict %d index\n", i); goto done; }
        if (used && !WriteEdict(f, g_edicts + i)) {
            fprintf(stderr, "WC3 SaveGame: failed at edict %d class=%08x\n", i, g_edicts[i].class_id); goto done;
        }
    }
    if (!WriteJass(f)) { fprintf(stderr, "WC3 SaveGame: failed at jass\n"); goto done; }
    ok = true;
done:
    return ok;
}

BOOL G_ReadState(LPSTATEBUFFER f) {
    SAVEHEADER header = { 0 };
    DWORD index;

    if (!load_bytes(f, &header.magic, sizeof(header.magic)) || !load_bytes(f, &header.version, sizeof(header.version)) ||
        (f->pos = 0)) {
        fprintf(stderr, "WC3 LoadGame: invalid header\n"); return false;
    }
    if (header.version != save_version || !load_bytes(f, &header, sizeof(header))) {
        fprintf(stderr, "WC3 LoadGame: invalid header\n"); return false;
    }
    {
        DWORD script = level.vm ? jass_programidentity(level.vm) : 0;
        LPCSTR field = NULL;
        if (header.magic != save_magic) field = "magic";
        else if (header.abi != SaveABI()) field = "abi";
        else if (header.edict_size != sizeof(edict_t)) field = "edict_size";
        else if (header.num_edicts > globals.max_edicts) field = "num_edicts";
        else if (header.max_clients != game.max_clients) field = "max_clients";
        else if (header.script_identity != script) field = "script_identity";
        else if (header.quests != ActiveQuestCount()) field = "quests";
        else if (header.groups < level.num_groups) field = "groups";
        else if (header.triggers < level.num_triggers) field = "triggers";
        else if (header.timers < level.num_timers) field = "timers";
        else if (header.events < ActiveEventCount()) field = "events";
        else if (!header.map_path[0] || strcasecmp(header.map_path, level.map_path)) field = "map_path";
        else if (!RestoreRegistrySlots(header.groups, header.timers, header.triggers, header.events)) field = "registry_slots";
        if (field) {
            fprintf(stderr, "WC3 LoadGame: header mismatch field=%s version=%u edict_size=%u/%zu edicts=%u/%u\n",
                    field, header.version, header.edict_size, sizeof(edict_t), header.num_edicts, globals.max_edicts);
            fprintf(stderr, "WC3 LoadGame: clients=%u/%u script=%u/%u quests=%u/%u groups=%u/%u triggers=%u/%u\n",
                    header.max_clients, game.max_clients, header.script_identity, script,
                        header.quests, ActiveQuestCount(), header.groups, level.num_groups, header.triggers, level.num_triggers);
            fprintf(stderr, "WC3 LoadGame: timers=%u/%u events=%u/%u map='%s'/'%s'\n",
                        header.timers, level.num_timers, header.events, ActiveEventCount(), header.map_path, level.map_path);
            return false;
        }
    }
    if (!ReadMappedFields(f, level_fields, (BYTE *)&level) || level.waypoints.count > MAX_WAYPOINTS ||
        (level.waypoints.count && (level.waypoints.count != MAX_WAYPOINTS || level.waypoints.cursor >= MAX_WAYPOINTS ||
        header.num_edicts < level.waypoints.count ||
        level.waypoints.base > header.num_edicts - level.waypoints.count)) ||
        (!level.waypoints.count && (level.waypoints.base || level.waypoints.cursor))) {
        fprintf(stderr, "WC3 LoadGame: failed at level state\n"); return false;
    }
    G_ResetJassGroupDebug();
    if (!ReadGroups(f, header.groups)) { return false; }
    /* Restore the Q2-style server tick before the next frame; all persisted deadlines use it. */
    gi.SetGameTime(level.time);
    FOR_LOOP(i, game.max_clients) if (!ReadClient(f, game.clients + i)) {
        fprintf(stderr, "WC3 LoadGame: failed at client %d\n", i); return false;
    }
    /* The baseline map already linked these same edict addresses. Clear its
     * spatial tree before raw records overwrite their area links, then rebuild
     * one authoritative set below; retaining both creates cyclic area lists. */
    gi.ClearWorld();
    memset(g_edicts, 0, sizeof(edict_t) * globals.max_edicts);
    globals.num_edicts = header.num_edicts;
    FOR_LOOP(i, header.num_edicts) {
        BOOL used;
        if (!load_bytes(f, &used, sizeof(used))) {
            fprintf(stderr, "WC3 LoadGame: failed at edict %d inuse\n", i); return false;
        }
        if (!used) continue;
        if (!load_bytes(f, &index, sizeof(index)) || index >= globals.max_edicts || !ReadEdict(f, g_edicts + index)) {
            fprintf(stderr, "WC3 LoadGame: failed at edict %d data\n", i); return false;
        }
    }
    /* JASS sound-handle playback parameters are transient presentation state,
     * not VM-owned payload bytes. Clear old pointer keys before snapshot handles
     * are reconstructed so a reused allocation cannot inherit stale state. */
    G_JassSoundRuntimeReset();
    if (!ReadJass(f) || f->pos != f->size) { fprintf(stderr, "WC3 LoadGame: failed at jass\n"); return false; }
    FOR_LOOP(i, game.max_clients) g_edicts[i].client = game.clients + i;
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = g_edicts + i;
        if (ent->inuse && ent->rally_indicator && ent->owner && ent->owner->client)
            ent->owner->client->rally_indicator = ent;
    }
    FOR_LOOP(i, globals.num_edicts) if (g_edicts[i].inuse && gi.LinkEntity) gi.LinkEntity(g_edicts + i);
    /* Client-side decoders are presentation state, not part of the save file.
     * Re-emit the restored semantic music state for clients that remained
     * connected across the load. */
    FOR_LOOP(i, game.max_clients) if (game.clients[i].connected) G_MusicSyncClient(game.clients + i);
    G_DisableStartingResourceCheatForLoadedGame();
    fprintf(stderr, "WC3 LoadGame: restored %s edicts=%u\n", header.map_path, header.num_edicts);
    return true;
}

#ifdef BZ_TESTS
/* Tests exercise the same engine envelope and buffer callbacks as server slot operations. */
BOOL WriteGame(LPCSTR path) {
    STATEBUFFER buf = {0};
    BOOL ok = G_WriteState(&buf) && state_write(path, &buf);
    state_free(&buf);
    return ok;
}
BOOL ReadGame(LPCSTR path) {
    STATEBUFFER buf = {0};
    BOOL ok = state_read(path, &buf) == SAVE_LOADED && G_ReadState(&buf);
    state_free(&buf);
    return ok;
}
BOOL G_GetSaveMap(LPCSTR path, LPSTR map, DWORD size) {
    STATEBUFFER buf = {0};
    BOOL ok = state_read(path, &buf) == SAVE_LOADED && G_SaveMap(&buf, map, size);
    state_free(&buf);
    return ok;
}
#endif
