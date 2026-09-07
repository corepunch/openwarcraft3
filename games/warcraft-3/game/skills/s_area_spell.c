#include "s_skills.h"

#define ID_BLIZZARD MAKEFOURCC('A', 'H', 'b', 'z')
#define ID_CARRION_SWARM MAKEFOURCC('A', 'U', 'c', 's')
#define ID_STARFALL MAKEFOURCC('A', 'E', 's', 'f')
#define ID_SHOCKWAVE MAKEFOURCC('A', 'O', 's', 'h')
#define ID_RAIN_OF_FIRE MAKEFOURCC('A', 'N', 'r', 'f')
#define ID_TRANQUILITY MAKEFOURCC('A', 'E', 't', 'q')
#define ID_DEATH_AND_DECAY MAKEFOURCC('A', 'U', 'd', 'd')
#define ID_THUNDER_CLAP MAKEFOURCC('A', 'H', 't', 'c')
#define ID_FROST_NOVA MAKEFOURCC('A', 'U', 'f', 'n')

typedef struct {
    LPEDICT caster;
    VECTOR2 direction;
    FLOAT length, width;
} shockwaveContext_t;

/* Deal ent->damage to every enemy within ent->collision of ent.  maxtotal > 0
 * caps the combined damage of this burst (WC3 "Max Damage" / "Maximum Damage per
 * Wave"): when damage*targets would exceed it, the per-target damage is scaled
 * down so the total lands on the cap. */
static void area_spell_damage(LPEDICT ent, FLOAT maxtotal) {
    LPEDICT caster = ent->owner;
    FLOAT radius = ent->collision;
    FLOAT damage = (FLOAT)ent->damage;
    DWORD ntargets = 0;

#define AREA_HITS(t) ((t)->inuse && (t) != caster && S_SpellIsAliveTarget(t) && \
                      S_SpellIsEnemy(caster, t) &&                              \
                      Vector2_distance(&(t)->s.origin2, &ent->s.origin2) <= radius)

    if (maxtotal > 0.0f) {
        FILTER_EDICTS(target, AREA_HITS(target)) {
            ntargets++;
        }
        if (ntargets > 0 && damage * (FLOAT)ntargets > maxtotal) {
            damage = MAX(1.0f, maxtotal / (FLOAT)ntargets);
        }
    }
    FILTER_EDICTS(target, AREA_HITS(target)) {
        T_Damage(target, caster, (DWORD)damage);
    }
#undef AREA_HITS
}

void blizzard_think(LPEDICT ent) {
    DWORD now = G_Time();

    if (ent->freetime && now < ent->freetime)
        return;
    area_spell_damage(ent, ent->velocity); /* velocity reused: max damage per wave */
    if (ent->resources > 0)
        ent->resources--;
    if (ent->resources == 0 || (ent->spawn_time && now >= ent->spawn_time)) {
        LPEDICT caster = ent->owner;
        G_FreeEdict(ent);
        if (caster && caster->channel.code == ID_BLIZZARD)
            S_SpellCancelChannel(caster);
        return;
    }
    ent->freetime = now + 1000;
}

/* Blizzard: channeled point-target AoE.  SPELL_CHANNEL flag causes the unified
 * pipeline to lock the caster via channel_code/cast_origin; spell_run_frame()
 * enforces movement-cancel.  The thinker entity runs the per-wave damage. */
static void blizzard_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    DWORD waves = (DWORD)S_SpellData(spell->code, level, 1);
    DWORD damage = (DWORD)S_SpellData(spell->code, level, 2);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    LPEDICT thinker;

    thinker = G_Spawn();
    thinker->owner = caster;
    thinker->s.origin2 = st.point;
    thinker->s.origin.x = st.point.x;
    thinker->s.origin.y = st.point.y;
    thinker->collision = area > 0 ? area : 200.0f;
    thinker->damage = damage ? damage : 1;
    thinker->resources = waves ? waves : 1;
    thinker->velocity = S_SpellData(spell->code, level, 6); /* DataF = Max Damage per Wave */
    thinker->spawn_time = G_Time() + (DWORD)(MAX(1.0f, S_SpellDuration(spell->code, level, false)) * 1000.0f);
    thinker->think = blizzard_think;
    blizzard_think(thinker); /* first wave immediately */
}

static spell_info_t spell_blizzard = {
    .code = ID_BLIZZARD,
    .name = "Blizzard",
    .target_type = SPELL_TARGET_POINT,
    .flags = SPELL_CHANNEL,
    .execute = blizzard_execute,
};

/* Carrion Swarm: instant point-target AoE blast. */
static void carrion_swarm_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT blast;

    blast = G_Spawn();
    blast->owner = caster;
    blast->s.origin2 = st.point;
    blast->collision = MAX(96.0f, S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level));
    blast->damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    area_spell_damage(blast, S_SpellData(spell->code, level, 2)); /* DataB = Max Damage */
    G_FreeEdict(blast);
}

static spell_info_t spell_carrion_swarm = {
    .code = ID_CARRION_SWARM,
    .name = "Carrion Swarm",
    .target_type = SPELL_TARGET_POINT,
    .execute = carrion_swarm_execute,
};

static BOOL shockwave_hits(LPEDICT target, shockwaveContext_t const *ctx) {
    VECTOR2 offset;
    FLOAT along, across;

    if (!target->inuse || target == ctx->caster || !S_SpellIsAliveTarget(target) ||
        !S_SpellIsEnemy(ctx->caster, target)) return false;
    offset = Vector2_sub(&target->s.origin2, &ctx->caster->s.origin2);
    along = Vector2_dot(&offset, &ctx->direction);
    across = offset.x * ctx->direction.y - offset.y * ctx->direction.x;
    return along >= 0.0f && along <= ctx->length && fabsf(across) <= ctx->width;
}

/* Shockwave uses the authored damage, cap, travel distance, and corridor width
 * rather than treating the line spell as a circular point-target burst. */
static void shockwave_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), ntargets = 0;
    shockwaveContext_t ctx = { .caster = caster };
    VECTOR2 offset = Vector2_sub(&st.point, &caster->s.origin2);
    FLOAT distance = Vector2_distance(&caster->s.origin2, &st.point);
    FLOAT damage = MAX(1.0f, S_SpellData(spell->code, level, 1));
    FLOAT maxtotal = S_SpellData(spell->code, level, 2);

    if (distance <= 0.0f) return;
    ctx.direction = Vector2_scale(&offset, 1.0f / distance);
    ctx.length = MIN(distance, S_SpellData(spell->code, level, 3));
    ctx.width = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FILTER_EDICTS(target, shockwave_hits(target, &ctx)) ntargets++;
    if (maxtotal > 0.0f && ntargets && damage * ntargets > maxtotal)
        damage = MAX(1.0f, maxtotal / (FLOAT)ntargets);
    FILTER_EDICTS(target, shockwave_hits(target, &ctx)) T_Damage(target, caster, (DWORD)damage);
}

static spell_info_t spell_shockwave = {
    .code = ID_SHOCKWAVE,
    .name = "Shockwave",
    .target_type = SPELL_TARGET_POINT,
    .execute = shockwave_execute,
};

static void rain_of_fire_think(LPEDICT ent) {
    DWORD now = G_Time();

    if (ent->freetime && now < ent->freetime) return;
    area_spell_damage(ent, 0.0f);
    if (ent->resources == 0) {
        G_FreeEdict(ent);
        return;
    }
    ent->resources--;
    ent->freetime = now + (DWORD)(MAX(0.1f, ent->velocity) * 1000.0f);
}

static void rain_of_fire_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();

    thinker->owner = caster;
    thinker->s.origin2 = st.point;
    thinker->s.origin.x = st.point.x;
    thinker->s.origin.y = st.point.y;
    thinker->collision = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    thinker->damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 2));
    thinker->resources = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    thinker->velocity = MAX(0.1f, S_SpellDuration(spell->code, level, false));
    thinker->think = rain_of_fire_think;
    rain_of_fire_think(thinker);
}

static spell_info_t spell_rain_of_fire = {
    .code = ID_RAIN_OF_FIRE,
    .name = "Rain of Fire",
    .target_type = SPELL_TARGET_POINT,
    .flags = SPELL_CHANNEL,
    .execute = rain_of_fire_execute,
};

/* Starfall: self-centered periodic area damage.  AbilityData stores the
 * authored damage in DataA, wave interval in DataB, area in Area, and the
 * channel lifetime in Dur. */
static void starfall_think(LPEDICT ent) {
    DWORD now = G_Time();

    if (ent->freetime && now < ent->freetime)
        return;
    area_spell_damage(ent, 0.0f);
    if (ent->spawn_time && now >= ent->spawn_time) {
        LPEDICT caster = ent->owner;
        G_FreeEdict(ent);
        if (caster && caster->channel.code == ID_STARFALL)
            S_SpellCancelChannel(caster);
        return;
    }
    ent->freetime = now + (DWORD)MAX(1.0f, ent->velocity * 1000.0f);
}

static void starfall_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();

    thinker->owner = caster;
    thinker->s.origin2 = caster->s.origin2;
    thinker->s.origin.x = caster->s.origin.x;
    thinker->s.origin.y = caster->s.origin.y;
    thinker->collision = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    thinker->damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    thinker->velocity = MAX(1.0f, S_SpellData(spell->code, level, 2));
    thinker->spawn_time = G_Time() + (DWORD)(MAX(1.0f, S_SpellDuration(spell->code, level, true)) * 1000.0f);
    thinker->think = starfall_think;
    starfall_think(thinker);
}

static spell_info_t spell_starfall = {
    .code = ID_STARFALL,
    .name = "Starfall",
    .target_type = SPELL_TARGET_NONE,
    .flags = SPELL_CHANNEL,
    .execute = starfall_execute,
};

/* Name=Blizzard
 * Ubertip="Calls down an icy storm that damages enemy units in a target area."
 */
ability_t a_blizzard = {
    .cmd = spell_cmd,
    .spell = &spell_blizzard,
};

/* Name=Carrion Swarm
 * Ubertip="Sends a wave of bats that damages enemy units in a line."
 */
ability_t a_carrion_swarm = {
    .cmd = spell_cmd,
    .spell = &spell_carrion_swarm,
};

/* Name=Shockwave
 * Ubertip="A wave of force that ripples outward, causing <AOsh,DataA1> damage to land units in a line."
 */
ability_t a_shockwave = {
    .cmd = spell_cmd,
    .spell = &spell_shockwave,
};

/* Name=Rain of Fire
 * Ubertip="Calls down waves of fire that damage enemy units in a target area."
 */
ability_t a_rain_of_fire = {
    .cmd = spell_cmd,
    .spell = &spell_rain_of_fire,
};

/* Death and Decay deals the authored percentage of each enemy's maximum life
 * on every pulse; unlike Rain of Fire, DataA is not a fixed damage amount. */
static void death_and_decay_think(LPEDICT ent) {
    DWORD now = G_Time();
    LPEDICT caster = ent->owner;

    if (ent->freetime && now < ent->freetime) return;
    FILTER_EDICTS(target, target->inuse && S_SpellIsAliveTarget(target) &&
                  S_SpellIsEnemy(caster, target) &&
                  Vector2_distance(&target->s.origin2, &ent->s.origin2) <= ent->collision) {
        T_Damage(target, caster, (DWORD)MAX(1.0f, target->health.max_value * ent->wait));
    }
    if (ent->spawn_time && now >= ent->spawn_time) {
        G_FreeEdict(ent);
        if (caster && caster->channel.code == ID_DEATH_AND_DECAY)
            S_SpellCancelChannel(caster);
        return;
    }
    ent->freetime = now + (DWORD)(MAX(0.1f, ent->velocity) * 1000.0f);
}

static void death_and_decay_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();

    thinker->owner = caster;
    thinker->s.origin2 = st.point;
    thinker->s.origin.x = st.point.x;
    thinker->s.origin.y = st.point.y;
    thinker->collision = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    thinker->wait = S_SpellData(spell->code, level, 1);
    thinker->velocity = MAX(0.1f, S_SpellData(spell->code, level, 2));
    thinker->spawn_time = G_Time() + (DWORD)(MAX(0.1f, S_SpellDuration(spell->code, level, true)) * 1000.0f);
    thinker->think = death_and_decay_think;
    death_and_decay_think(thinker);
}

static spell_info_t spell_death_and_decay = {
    .code = ID_DEATH_AND_DECAY,
    .name = "Death and Decay",
    .target_type = SPELL_TARGET_POINT,
    .flags = SPELL_CHANNEL,
    .execute = death_and_decay_execute,
};

/* Name=Death and Decay
 * Ubertip="Damages enemy units in a target area over time."
 */
ability_t a_death_and_decay = { .cmd = spell_cmd, .spell = &spell_death_and_decay };

static void area_damage_status_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    AbilityData_t const *data = G_AbilityData(spell->code);
    FLOAT radius = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FLOAT duration = S_SpellDuration(spell->code, level, false);
    DWORD damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    LPCSTR buff = data->level[level - 1].buffID;

    FILTER_EDICTS(target, target->inuse && target != caster && S_SpellIsAliveTarget(target) &&
                  S_SpellIsEnemy(caster, target) &&
                  Vector2_distance(&target->s.origin2, &caster->s.origin2) <= radius) {
        T_Damage(target, caster, damage);
        if (!M_IsDead(target) && buff && strlen(buff) >= 4 && duration > 0.0f)
            unit_addtimedstatus(target, buff, level, duration);
    }
}

static spell_info_t spell_thunder_clap = {
    .code = ID_THUNDER_CLAP,
    .name = "Thunder Clap",
    .target_type = SPELL_TARGET_NONE,
    .execute = area_damage_status_execute,
};

static spell_info_t spell_frost_nova = {
    .code = ID_FROST_NOVA,
    .name = "Frost Nova",
    .target_type = SPELL_TARGET_NONE,
    .execute = area_damage_status_execute,
};

/* Name=Thunder Clap
 * Ubertip="Slams the ground, damaging and slowing nearby enemy units."
 */
ability_t a_thunder_clap = { .cmd = spell_cmd, .spell = &spell_thunder_clap };
/* Name=Frost Nova
 * Ubertip="Blasts nearby enemy units with frost, damaging and slowing them."
 */
ability_t a_frost_nova = { .cmd = spell_cmd, .spell = &spell_frost_nova };

static void tranquility_think(LPEDICT ent) {
    DWORD now = G_Time();
    LPEDICT caster = ent->owner;

    if (ent->freetime && now < ent->freetime) return;
    FILTER_EDICTS(target, target->inuse && S_SpellIsAliveTarget(target) &&
                  S_SpellIsFriend(caster, target) &&
                  Vector2_distance(&target->s.origin2, &ent->s.origin2) <= ent->collision)
        S_SpellHeal(target, ent->damage);
    if (ent->spawn_time && now >= ent->spawn_time) {
        G_FreeEdict(ent);
        if (caster && caster->channel.code == ID_TRANQUILITY)
            S_SpellCancelChannel(caster);
        return;
    }
    ent->freetime = now + (DWORD)(MAX(0.1f, ent->velocity) * 1000.0f);
}

static void tranquility_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();

    thinker->owner = caster;
    thinker->s.origin2 = caster->s.origin2;
    thinker->s.origin.x = caster->s.origin.x;
    thinker->s.origin.y = caster->s.origin.y;
    thinker->collision = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    thinker->damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    thinker->velocity = MAX(0.1f, S_SpellData(spell->code, level, 2));
    thinker->spawn_time = G_Time() + (DWORD)(MAX(0.1f, S_SpellDuration(spell->code, level, true)) * 1000.0f);
    thinker->think = tranquility_think;
    tranquility_think(thinker);
}

static spell_info_t spell_tranquility = {
    .code = ID_TRANQUILITY,
    .name = "Tranquility",
    .target_type = SPELL_TARGET_NONE,
    .flags = SPELL_CHANNEL,
    .execute = tranquility_execute,
};

/* Name=Tranquility
 * Ubertip="Heals nearby friendly units over time."
 */
ability_t a_tranquility = {
    .cmd = spell_cmd,
    .spell = &spell_tranquility,
};

/* Name=Starfall
 * Ubertip="Calls down falling stars that damage nearby enemy units over time."
 */
ability_t a_starfall = {
    .cmd = spell_cmd,
    .spell = &spell_starfall,
};

static void channel_test_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
    S_SpellCursorSplat(clent, 200.0f);
}

/* a_channel_test remains a non-spell ability for ad-hoc testing. */
ability_t a_channel_test = {
    .cmd = channel_test_command,
};
