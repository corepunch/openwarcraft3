#include "s_skills.h"

#define ID_THUNDER_BOLT MAKEFOURCC('A', 'H', 't', 'b')
#define ID_FIRE_BOLT MAKEFOURCC('A', 'N', 'f', 'b')
#define ID_STUN_BUFF "Bstu"

static LPCSTR thunderbolt_missile_art;
static LPCSTR firebolt_missile_art;
static FLOAT thunderbolt_missile_speed;
static FLOAT firebolt_missile_speed;

static void thunderbolt_projectile_hit(LPEDICT missile);

static umove_t thunderbolt_projectile_move = { "stand", NULL, thunderbolt_projectile_hit, &a_thunderbolt };
static umove_t firebolt_projectile_move = { "stand", NULL, thunderbolt_projectile_hit, &a_firebolt };
static umove_t spell_cast_move = { "spell", ai_idle, NULL, &a_thunderbolt };

static LPCSTR bolt_missile_art(DWORD code) {
    return code == ID_FIRE_BOLT ? firebolt_missile_art : thunderbolt_missile_art;
}

static FLOAT bolt_missile_speed(DWORD code) {
    FLOAT speed = code == ID_FIRE_BOLT ? firebolt_missile_speed : thunderbolt_missile_speed;
    return speed > 0 ? speed : 1000;
}

static void thunderbolt_projectile_hit(LPEDICT missile) {
    LPEDICT target = missile->goalentity;
    LPEDICT caster = missile->owner;

    if (S_SpellIsAliveTarget(target)) {
        T_Damage(target, caster, missile->damage);
        if (!M_IsDead(target)) {
            unit_addtimedstatus(target, ID_STUN_BUFF, 1, missile->wait);
        }
    }
    G_FreeEdict(missile);
}

static void thunderbolt_fire(LPEDICT caster, LPEDICT target, DWORD code, DWORD level) {
    LPEDICT missile;
    LPCSTR art = bolt_missile_art(code);
    FLOAT speed = bolt_missile_speed(code);
    FLOAT duration = S_SpellDuration(code, level, UNIT_LEVEL(target->class_id) >= 5);

    unit_setmove(caster, &spell_cast_move);
    missile = G_Spawn();
    missile->s.origin = caster->s.origin;
    missile->s.angle = caster->s.angle;
    missile->s.model = art ? gi.ModelIndex(art) : 0;
    missile->goalentity = target;
    missile->owner = caster;
    missile->velocity = speed / 1000.0f;
    missile->damage = (DWORD)S_SpellData(code, level, 1);
    missile->wait = duration;
    missile->movetype = MOVETYPE_FLYMISSILE;
    missile->currentmove = code == ID_FIRE_BOLT ? &firebolt_projectile_move : &thunderbolt_projectile_move;
}

static BOOL thunderbolt_selecttarget(LPEDICT clent, LPEDICT target) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, ID_THUNDER_BOLT);
    DWORD level = S_SpellLevel(caster, code);
    FLOAT range = S_SpellRange(code, level);

    if (!S_SpellIsAliveTarget(target) || !S_SpellIsEnemy(caster, target) ||
        !S_SpellAllowsTarget(code, caster, target)) {
        return false;
    }
    if (!S_SpellTargetInRange(caster, target, range)) {
        return false;
    }
    if (!S_SpellCooldownReady(caster, code)) {
        return false;
    }
    if (!S_SpellSpendMana(caster, code, level)) {
        return false;
    }

    S_SpellStartCooldown(caster, code, level);
    thunderbolt_fire(caster, target, code, level);
    return true;
}

static void thunderbolt_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
    clent->client->menu.on_entity_selected = thunderbolt_selecttarget;
}

static void SP_ability_thunderbolt(LPCSTR classname, ability_t *self) {
    (void)self;
    thunderbolt_missile_art = FindConfigValue(classname, "Missileart");
    thunderbolt_missile_speed = AB_Number(classname, "Missilespeed");
}

static void SP_ability_firebolt(LPCSTR classname, ability_t *self) {
    (void)self;
    firebolt_missile_art = FindConfigValue(classname, "Missileart");
    firebolt_missile_speed = AB_Number(classname, "Missilespeed");
}

ability_t a_thunderbolt = {
    .init = SP_ability_thunderbolt,
    .cmd = thunderbolt_command,
};

ability_t a_firebolt = {
    .init = SP_ability_firebolt,
    .cmd = thunderbolt_command,
};
