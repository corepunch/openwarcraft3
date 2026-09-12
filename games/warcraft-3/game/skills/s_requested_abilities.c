#include "s_skills.h"

static void whirlwind_think(LPEDICT ent);

typedef struct {
    LPEDICT caster;
    spellTarget_t target;
    spell_info_t const *spell;
    FLOAT scale;
    BOOL random_jumps;
} bounceParams_t;

BOOL S_UnitHasStatus(LPCEDICT unit, DWORD code) {
    if (!unit) return false;
    FOR_LOOP(i, MAX_UNIT_STATUSES)
        if (unit->abilstatus[i].level && unit->abilstatus[i].code == code &&
            (!unit->abilstatus[i].timestamp || unit->abilstatus[i].timestamp > G_Time())) return true;
    return false;
}

static LPCSTR spell_buff(spell_info_t const *spell, DWORD level) {
    LPCSTR buff = G_AbilityLevel(spell->code, level)->buffID;
    return buff && strlen(buff) >= 4 ? buff : NULL;
}

static void target_status_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPCSTR buff = spell_buff(spell, level);
    if (!st.entity || !buff) return;
    unit_addtimedstatus(st.entity, buff, level, S_SpellDuration(spell->code, level, G_UnitIsHero(st.entity)));
    G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, st.entity, NULL, true);
}

static void toggle_status_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        heroabilitystatus_t *status = caster->abilstatus + i;
        if (status->level && status->code == spell->code) { memset(status, 0, sizeof(*status)); return; }
    }
    unit_addstatus(caster, (LPCSTR)&spell->code, S_SpellLevel(caster, spell->code));
}

static void area_status_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    LPCSTR buff = spell_buff(spell, level);
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) &&
                  Vector2_distance(&target->s.origin2, &st.point) <= area) {
        if (buff) unit_addtimedstatus(target, buff, level, S_SpellDuration(spell->code, level, G_UnitIsHero(target)));
    }
}

static void mass_teleport_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = 1;
    VECTOR2 src = caster->s.origin2, dst = st.entity ? st.entity->s.origin2 : st.point;
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    DWORD limit = (DWORD)S_SpellData(spell->code, level, 1);
    FILTER_EDICTS(unit, count < limit && unit != caster && S_SpellIsAliveTarget(unit) && S_SpellIsFriend(caster, unit) &&
                  Vector2_distance(&unit->s.origin2, &src) <= area) {
        VECTOR2 offset = Vector2_sub(&unit->s.origin2, &src);
        VECTOR2 requested = S_SpellData(spell->code, level, 3) ? dst : Vector2_add(&dst, &offset);
        if (G_FindUnitUnstuckPosition(unit, &requested, &unit->s.origin2)) {
            unit->s.origin.x = unit->s.origin2.x; unit->s.origin.y = unit->s.origin2.y; count++;
        }
    }
    if (G_FindUnitUnstuckPosition(caster, &dst, &caster->s.origin2)) {
        caster->s.origin.x = caster->s.origin2.x; caster->s.origin.y = caster->s.origin2.y;
    }
}

static void radial_damage_status(LPEDICT caster, VECTOR2 point, spell_info_t const *spell, DWORD data) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    LPCSTR buff = spell_buff(spell, level);
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) &&
                  Vector2_distance(&target->s.origin2, &point) <= area) {
        S_SpellDamage(target, caster, (int)MAX(1.0f, S_SpellData(spell->code, level, data)));
        if (buff && !M_IsDead(target))
            unit_addtimedstatus(target, buff, level, S_SpellDuration(spell->code, level, G_UnitIsHero(target)));
    }
}

static void stomp_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();
    thinker->owner = caster; thinker->class_id = spell->code; thinker->s.origin2 = st.point;
    thinker->spawn_time = G_Time() + (DWORD)(S_SpellDuration(spell->code, level, false) * 1000.0f);
    thinker->freetime = G_Time(); thinker->think = whirlwind_think;
}

static void impale_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    VECTOR2 offset = Vector2_sub(&st.point, &caster->s.origin2);
    FLOAT distance = Vector2_distance(&caster->s.origin2, &st.point);
    VECTOR2 direction;
    LPCSTR buff = spell_buff(spell, level);
    if (distance <= 0.0f) return;
    direction = Vector2_scale(&offset, 1.0f / distance);
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target)) {
        VECTOR2 delta = Vector2_sub(&target->s.origin2, &caster->s.origin2);
        FLOAT along = Vector2_dot(&delta, &direction);
        FLOAT across = delta.x * direction.y - delta.y * direction.x;
        if (along < 0.0f || along > S_SpellData(spell->code, level, 1) ||
            fabsf(across) > S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level)) continue;
        S_SpellDamage(target, caster, (int)S_SpellData(spell->code, level, 3));
        if (!M_IsDead(target) && buff)
            unit_addtimedstatus(target, buff, level, S_SpellDuration(spell->code, level, G_UnitIsHero(target)));
    }
}

static void earthquake_think(LPEDICT ent) {
    DWORD level = S_SpellLevel(ent->owner, ent->class_id), now = G_Time();
    spell_info_t const *spell = S_SpellInfoForCode(ent->class_id);
    LPCSTR buff = spell_buff(spell, level);
    if (now >= ent->spawn_time) { S_SpellCancelChannel(ent->owner); G_FreeEdict(ent); return; }
    if (ent->freetime && now < ent->freetime) return;
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsEnemy(ent->owner, target) &&
                  Vector2_distance(&target->s.origin2, &ent->s.origin2) <= S_SpellNumber(ent->class_id, ABILITY_NUMBER_AREA, level)) {
        if (G_UnitIsBuilding(target->class_id)) S_SpellDamage(target, ent->owner, (int)S_SpellData(ent->class_id, level, 2));
        else if (buff) unit_addtimedstatus(target, buff, level, 1.5f);
    }
    ent->freetime = now + 1000;
}

static void earthquake_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();
    thinker->owner = caster; thinker->class_id = spell->code; thinker->s.origin2 = st.point;
    thinker->spawn_time = G_Time() + (DWORD)(S_SpellDuration(spell->code, level, true) * 1000.0f);
    thinker->think = earthquake_think; earthquake_think(thinker);
}

static void whirlwind_think(LPEDICT ent) {
    DWORD data = ent->class_id == MAKEFOURCC('A','N','s','t') ? 2 : 1;
    if (G_Time() >= ent->spawn_time) { S_SpellCancelChannel(ent->owner); G_FreeEdict(ent); return; }
    if (ent->freetime && G_Time() < ent->freetime) return;
    if (ent->class_id == MAKEFOURCC('A','O','w','w') || ent->class_id == MAKEFOURCC('A','N','t','o'))
        ent->s.origin2 = ent->owner->s.origin2;
    radial_damage_status(ent->owner, ent->s.origin2, S_SpellInfoForCode(ent->class_id), data);
    ent->freetime = G_Time() + 1000;
}

static void whirlwind_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPEDICT thinker = G_Spawn();
    thinker->owner = caster; thinker->class_id = spell->code;
    thinker->spawn_time = G_Time() + (DWORD)(S_SpellDuration(spell->code, level, true) * 1000.0f);
    thinker->think = whirlwind_think; whirlwind_think(thinker);
}

static void morph_end(LPEDICT thinker) {
    if (G_Time() < thinker->spawn_time) return;
    if (thinker->owner && thinker->owner->inuse) G_TransformUnitType(thinker->owner, thinker->resources);
    G_FreeEdict(thinker);
}

static void morph_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), form = S_SpellUnitId(spell->code, level), original = caster->class_id;
    FLOAT duration = S_SpellDuration(spell->code, level, true);
    if (!form || !G_TransformUnitType(caster, form) || duration <= 0.0f) return;
    LPEDICT thinker = G_Spawn();
    thinker->owner = caster; thinker->resources = original;
    thinker->spawn_time = G_Time() + (DWORD)(duration * 1000.0f); thinker->think = morph_end;
}

static void summon_execute_requested(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    S_SummonUnits(caster, S_SpellUnitId(spell->code, level), (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1)),
                  S_SpellDuration(spell->code, level, false));
}

static void carrion_beetles_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    FLOAT range = S_SpellRange(spell->code, level);
    LPEDICT corpse = NULL;
    FILTER_EDICTS(unit, unit->inuse && M_IsDead(unit) && !G_UnitIsHero(unit) &&
                  Vector2_distance(&unit->s.origin2, &caster->s.origin2) <= range) { corpse = unit; break; }
    if (!corpse) return;
    FOR_LOOP(i, count) S_SummonAt(caster, S_SpellDataId(spell->code, level, 3), &corpse->s.origin2,
                                  S_SpellDuration(spell->code, level, false));
    G_FreeEdict(corpse);
}

int S_BlackArrowDamage(LPEDICT attacker, int damage) {
    DWORD level = G_UnitStatusLevel(attacker, MAKEFOURCC('A','N','b','a'));
    return level ? damage + (int)S_SpellData(MAKEFOURCC('A','N','b','a'), level, 1) : damage;
}

void S_BlackArrowDeath(LPEDICT attacker, LPEDICT target) {
    DWORD level = G_UnitStatusLevel(attacker, MAKEFOURCC('A','N','b','a'));
    if (level && target && M_IsDead(target))
        S_SummonAt(attacker, S_SpellUnitId(MAKEFOURCC('A','N','b','a'), level), &target->s.origin2,
                   S_SpellData(MAKEFOURCC('A','N','b','a'), level, 3));
}

static BOOL death_coil_validate(LPEDICT caster, spellTarget_t st) {
    LPCSTR race = st.entity && st.entity->data.UnitData ? st.entity->data.UnitData->race : NULL;
    return S_SpellIsAliveTarget(st.entity) && race && ((!strcmp(race, STR_UNDEAD) && S_SpellIsFriend(caster, st.entity)) ||
           (strcmp(race, STR_UNDEAD) && S_SpellIsEnemy(caster, st.entity)));
}

static void death_coil_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT amount = S_SpellData(spell->code, level, 1);
    if (!strcmp(st.entity->data.UnitData->race, STR_UNDEAD)) S_SpellHeal(st.entity, amount);
    else S_SpellDamage(st.entity, caster, (int)(amount * 0.5f));
}

static void death_pact_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT life = st.entity->health.value;
    S_SpellHeal(caster, S_SpellData(spell->code, level, 2) * life);
    caster->mana.value = MIN(caster->mana.max_value, caster->mana.value + S_SpellData(spell->code, level, 1) * life);
    S_SpellDamage(st.entity, caster, (int)MAX(1.0f, st.entity->health.value));
}

/* Resolve chained damage jumps while keeping target selection separate from spell metadata. */
static void bounce_execute(bounceParams_t const *params) {
    LPEDICT caster = params->caster;
    spellTarget_t st = params->target;
    spell_info_t const *spell = params->spell;
    FLOAT scale = params->scale;
    BOOL random_jumps = params->random_jumps;
    DWORD level = S_SpellLevel(caster, spell->code), hits = (DWORD)S_SpellData(spell->code, level, 2);
    FLOAT damage = S_SpellData(spell->code, level, 1);
    LPEDICT current = st.entity, visited[32] = {0};
    DWORD nvisited = 0;
    FOR_LOOP(i, MIN(hits, 32)) {
        LPEDICT candidates[MAX_GROUP_SIZE];
        DWORD candidate_count = 0;
        if (!current) break;
        S_SpellDamage(current, caster, (int)MAX(1.0f, damage));
        G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, current, NULL, true);
        visited[nvisited++] = current; damage *= scale;
        FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) &&
                      Vector2_distance(&target->s.origin2, &current->s.origin2) <= S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level)) {
            BOOL seen = false;
            FOR_LOOP(j, nvisited) seen |= target == visited[j];
            if (!seen && candidate_count < MAX_GROUP_SIZE) candidates[candidate_count++] = target;
        }
        current = candidate_count ? candidates[random_jumps ? rand() % candidate_count : 0] : NULL;
    }
}

static void chain_lightning_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    bounceParams_t params = { .caster = caster, .target = st, .spell = spell,
        .scale = 1.0f - S_SpellData(spell->code, level, 3), .random_jumps = true };
    bounce_execute(&params);
}

static void forked_lightning_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    bounceParams_t params = { .caster = caster, .target = st, .spell = spell,
        .scale = 1.0f, .random_jumps = false };
    bounce_execute(&params);
}

static void animate_dead_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = 0, limit = (DWORD)S_SpellData(spell->code, level, 1);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FILTER_EDICTS(unit, count < limit && unit->inuse && M_IsDead(unit) && !G_UnitIsHero(unit) &&
                  Vector2_distance(&unit->s.origin2, &caster->s.origin2) <= area) {
        G_SetHealth(unit, unit->health.max_value); unit->svflags &= ~SVF_DEADMONSTER; unit->s.flags &= ~EF_NOT_SELECTABLE;
        unit->s.player = caster->s.player; unit->owner = caster;
        unit_addtimedstatus(unit, "BTLF", level, S_SpellDuration(spell->code, level, false));
        if (unit->stand) unit->stand(unit);
        count++; /* Keep the revived-unit limit independent of the optional animation callback. */
    }
}

static void inferno_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    radial_damage_status(caster, st.point, spell, 1);
    S_SummonAt(caster, S_SpellUnitId(spell->code, level), &st.point, S_SpellData(spell->code, level, 2));
}

static void far_sight_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    G_FowSetStateRadius(&(FOGWRITE){ caster->s.player, WC3_FOG_STATE_VISIBLE, true }, &st.point,
                        S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level));
}

static void reincarnation_think(LPEDICT thinker) {
    if (!thinker->owner || !thinker->owner->inuse) { G_FreeEdict(thinker); return; }
    if (G_Time() < thinker->spawn_time) return;
    if (M_IsDead(thinker->owner)) G_ReviveHero(thinker->owner, thinker->s.origin2.x, thinker->s.origin2.y);
    G_FreeEdict(thinker);
}

void S_ReincarnationOnDeath(LPEDICT unit) {
    DWORD code = MAKEFOURCC('A', 'O', 'r', 'e'), level = G_UnitAbilityLevel(unit, code);
    LPEDICT thinker;
    if (!level || !S_SpellCooldownReady(unit, code)) return;
    thinker = G_Spawn(); thinker->owner = unit; thinker->s.origin2 = unit->s.origin2;
    thinker->spawn_time = G_Time() + (DWORD)(S_SpellData(code, level, 1) * 1000.0f);
    thinker->think = reincarnation_think; S_SpellStartCooldown(unit, code, level);
}

static void healing_wave_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = (DWORD)S_SpellData(spell->code, level, 2);
    FLOAT amount = S_SpellData(spell->code, level, 1), loss = S_SpellData(spell->code, level, 3);
    LPEDICT current = st.entity, visited[32] = {0};
    FOR_LOOP(i, MIN(count ? count : 1, 32)) {
        if (!current || !S_SpellIsAliveTarget(current) || !S_SpellIsFriend(caster, current)) break;
        S_SpellHeal(current, amount); visited[i] = current; amount *= 1.0f - loss;
        current = NULL;
        FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsFriend(caster, target) &&
                      Vector2_distance(&target->s.origin2, &visited[i]->s.origin2) <= S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level)) {
            BOOL seen = false; FOR_LOOP(j, i + 1) seen |= target == visited[j];
            if (!seen) { current = target; break; }
        }
    }
}

static void big_bad_voodoo_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPCSTR buff = spell_buff(spell, level);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    if (!buff) return;
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsFriend(caster, target) &&
                  Vector2_distance(&target->s.origin2, &caster->s.origin2) <= area)
        unit_addtimedstatus(target, buff, level, S_SpellDuration(spell->code, level, false));
}

static void acid_bomb_think(LPEDICT thinker) {
    LPEDICT target = thinker->goalentity;
    if (G_Time() >= thinker->spawn_time || !target || !target->inuse || M_IsDead(target)) { G_FreeEdict(thinker); return; }
    if (!thinker->freetime || G_Time() >= thinker->freetime) {
        S_SpellDamage(target, thinker->owner, thinker->damage); thinker->freetime = G_Time() + 1000;
    }
}

static void acid_bomb_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPCSTR buff = spell_buff(spell, level);
    LPEDICT thinker;
    if (!st.entity || !S_SpellIsAliveTarget(st.entity)) return;
    if (buff) unit_addtimedstatus(st.entity, buff, level, S_SpellDuration(spell->code, level, false));
    thinker = G_Spawn(); thinker->owner = caster; thinker->goalentity = st.entity; thinker->damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 3));
    thinker->spawn_time = G_Time() + (DWORD)(S_SpellDuration(spell->code, level, false) * 1000.0f); thinker->think = acid_bomb_think;
}

static void revive_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = 0;
    DWORD limit = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    FLOAT radius = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FILTER_EDICTS(target, count < limit && target != caster && target->inuse && G_UnitIsHero(target) &&
                  M_IsDead(target) && S_SpellIsFriend(caster, target) &&
                  Vector2_distance(&target->s.origin2, &st.point) <= radius) {
        G_ReviveHero(target, target->s.origin2.x, target->s.origin2.y);
        count++;
    }
}

static void breath_of_fire_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT radius = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    DWORD damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    FILTER_EDICTS(target, target != caster && S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) &&
                  Vector2_distance(&target->s.origin2, &st.point) <= radius)
        S_SpellDamage(target, caster, damage);
}

static void area_buff_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    AbilityData_t const *data = G_AbilityData(spell->code);
    FLOAT radius = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FLOAT duration = S_SpellDuration(spell->code, level, false);
    LPCSTR buff = data->level[level - 1].buffID;
    FILTER_EDICTS(target, target != caster && S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) &&
                  Vector2_distance(&target->s.origin2, &caster->s.origin2) <= radius)
        if (buff && strlen(buff) >= 4) unit_addtimedstatus(target, buff, level, duration);
}

#define SPELL(NAME, TARGET, FLAGS, EXECUTE) \
    static spell_info_t spell_##NAME = { .name = #NAME, .target_type = TARGET, .flags = FLAGS, .execute = EXECUTE }; \
    ability_t C##NAME = { .cmd = spell_cmd, .spell = &spell_##NAME }

/* Name=Mass Teleport
 * Ubertip="Teleports the caster and nearby friendly units to a target location."
 */
SPELL(AbilityMassTeleport, SPELL_TARGET_UNIT, 0, mass_teleport_execute);
/* Name=Stampede
 * Ubertip="Calls down hordes of rampaging thunder lizards to explode upon the Beastmaster's enemies."
 */
SPELL(AbilityStampede, SPELL_TARGET_POINT, SPELL_CHANNEL, stomp_execute);
/* Name=Bladestorm
 * Ubertip="Causes a Blademaster to spin violently, damaging nearby enemy units."
 */
SPELL(AbilityWhirlwind, SPELL_TARGET_NONE, SPELL_CHANNEL, whirlwind_execute);
/* Name=Tornado
 * Ubertip="Creates a tornado that damages and disables enemy units."
 */
SPELL(AbilityTornado, SPELL_TARGET_NONE, SPELL_CHANNEL, whirlwind_execute);
/* Name=Banish
 * Ubertip="Turns a target unit ethereal, making it unable to attack or be attacked by physical attacks."
 */
SPELL(AbilityBanish, SPELL_TARGET_UNIT, 0, target_status_execute);
/* Name=Phoenix
 * Ubertip="Summons a Phoenix to fight for the caster."
 */
SPELL(AbilitySummonPhoenix, SPELL_TARGET_NONE, 0, summon_execute_requested);
/* Name=Carrion Beetles
 * Ubertip="Raises carrion beetles from a nearby corpse."
 */
SPELL(AbilityCarrionScarabs, SPELL_TARGET_NONE, SPELL_AUTOCAST, carrion_beetles_execute);
/* Name=Impale
 * Ubertip="Slams the ground, impaling enemy units in a line and stunning them."
 */
SPELL(AbilityImpale, SPELL_TARGET_POINT, 0, impale_execute);
/* Name=Locust Swarm
 * Ubertip="Summons a swarm of locusts that damages enemy units and returns life to the caster."
 */
SPELL(AbilityLocustSwarm, SPELL_TARGET_NONE, SPELL_CHANNEL, summon_execute_requested);
/* Name=Black Arrow
 * Ubertip="Adds bonus damage to attacks and summons a skeleton when an attacked unit dies."
 * Untip="Right-click to activate auto-casting."
 * Unubertip="Right-click to deactivate auto-casting."
 */
SPELL(AbilityBlackArrow, SPELL_TARGET_NONE, SPELL_TOGGLE | SPELL_AUTOCAST, toggle_status_execute);
/* Name=Poison Arrows
 * Ubertip="Adds <AHfa,DataA1> bonus fire damage to an attack against enemies, but drains mana with each shot fired."
 * Untip="Right-click to activate auto-casting."
 * Unubertip="Right-click to deactivate auto-casting."
 */
SPELL(AbilityPoisonArrows, SPELL_TARGET_NONE, SPELL_TOGGLE | SPELL_AUTOCAST, toggle_status_execute);
/* Name=Silence
 * Ubertip="Stops enemy units in an area from casting spells."
 */
SPELL(AbilitySilence, SPELL_TARGET_POINT, 0, area_status_execute);
/* Name=Animate Dead
 * Ubertip="Raises a number of corpses to serve the caster for a limited time."
 */
SPELL(AbilityAnimateDead, SPELL_TARGET_NONE, 0, animate_dead_execute);
static spell_info_t spell_death_coil = { .name = "Death Coil", .target_type = SPELL_TARGET_UNIT, .validate = death_coil_validate, .execute = death_coil_execute };
ability_t CAbilityDeathCoil = { .cmd = spell_cmd, .spell = &spell_death_coil };
/* Name=Death Pact
 * Ubertip="Sacrifices a friendly undead unit to restore the Death Knight's life and mana."
 */
SPELL(AbilityDeathPact, SPELL_TARGET_UNIT, 0, death_pact_execute);
/* Name=Metamorphosis
 * Ubertip="Transforms the Demon Hunter into a powerful demon."
 */
SPELL(AbilityMetamorphosis, SPELL_TARGET_NONE, 0, morph_execute);
/* Name=Sleep
 * Ubertip="Puts a target enemy unit to sleep."
 */
SPELL(AbilitySleep, SPELL_TARGET_UNIT, 0, target_status_execute);
/* Name=Inferno
 * Ubertip="Calls down an infernal that damages nearby enemy units."
 */
SPELL(AbilityDreadLordInferno, SPELL_TARGET_POINT, 0, inferno_execute);
/* Name=Chain Lightning
 * Ubertip="Hurls a bolt of lightning that jumps between enemy units."
 */
SPELL(AbilityChainLightning, SPELL_TARGET_UNIT, 0, chain_lightning_execute);
/* Name=Forked Lightning
 * Ubertip="Strikes multiple enemy units with lightning."
 */
SPELL(AbilityForkedLightning, SPELL_TARGET_UNIT, 0, forked_lightning_execute);
/* Name=Earthquake
 * Ubertip="Causes the earth to shake, damaging enemy buildings and slowing enemy units."
 */
SPELL(AbilityEarthquake, SPELL_TARGET_POINT, SPELL_CHANNEL, earthquake_execute);
/* Name=Far Sight
 * Ubertip="Reveals a specified area of the map."
 */
SPELL(AbilityFarSight, SPELL_TARGET_POINT, 0, far_sight_execute);
/* Name=Resurrection
 * Ubertip="Brings dead friendly Heroes back to life."
 */
SPELL(AbilityResurrection, SPELL_TARGET_POINT, 0, revive_execute);
/* Name=Breath of Fire
 * Ubertip="Breathes a cone of fire at enemy units, dealing <ANcf,DataA1> initial damage."
 */
SPELL(AbilityBreathOfFire, SPELL_TARGET_POINT, 0, breath_of_fire_execute);
/* Name=Howl of Terror
 * Ubertip="Reduces the attack damage of nearby enemy units."
 */
SPELL(AbilityHowlOfTerror, SPELL_TARGET_NONE, 0, area_buff_execute);
/* Name=Drunken Haze
 * Ubertip="Slows enemy units and gives them a chance to miss on attacks."
 */
SPELL(AbilityDrunkenHaze, SPELL_TARGET_UNIT, 0, target_status_execute);
/* Name=Doom
 * Ubertip="Curses a target enemy unit, preventing it from casting spells and damaging it over time."
 */
SPELL(AbilityDoom, SPELL_TARGET_UNIT, 0, target_status_execute);
/* Name=Healing Wave
 * Ubertip="Heals a target friendly unit and bounces to nearby friendlies, healing less each jump."
 */
SPELL(AbilityHealingWave, SPELL_TARGET_UNIT, 0, healing_wave_execute);
/* Name=Hex
 * Ubertip="Transforms an enemy unit into a random critter for <ANhx,Dur1> seconds."
 */
SPELL(AbilityHex, SPELL_TARGET_UNIT, 0, target_status_execute);
SPELL(AbilitySpiritOfVengeance, SPELL_TARGET_NONE, 0, summon_execute_requested);
SPELL(AbilityVoodoo, SPELL_TARGET_NONE, 0, big_bad_voodoo_execute);
SPELL(AbilityAcidBomb, SPELL_TARGET_UNIT, 0, acid_bomb_execute);

static void searing_arrows_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    toggle_status_execute(caster, st, spell);
}

static spell_info_t spell_searing_arrows = {
    .name = "Searing Arrows", .target_type = SPELL_TARGET_NONE,
    .flags = SPELL_TOGGLE | SPELL_AUTOCAST, .execute = searing_arrows_execute,
};

ability_t CAbilityFlamingArrows = { .cmd = spell_cmd, .spell = &spell_searing_arrows };
ability_t CAbilityAuraTrueshot = { .flags = ABILITY_PASSIVE };
ability_t CAbilityReincarnation = { .flags = ABILITY_PASSIVE };
