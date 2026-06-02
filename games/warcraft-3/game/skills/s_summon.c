#include "s_skills.h"

#define ID_TIMED_LIFE "BTLF"
#define ID_WATER_ELEMENTAL MAKEFOURCC('A', 'H', 'w', 'e')
#define ID_FERAL_SPIRIT MAKEFOURCC('A', 'O', 's', 'f')

static void summon_unit(LPEDICT caster, DWORD unit_id, DWORD index, DWORD count, FLOAT duration) {
    VECTOR2 loc;
    FLOAT angle;
    LPEDICT summon;

    if (!caster || !unit_id) {
        return;
    }

    angle = count > 0 ? (2.0f * (FLOAT)M_PI * (FLOAT)index) / (FLOAT)count : 0.0f;
    loc = caster->s.origin2;
    loc.x += cosf(angle) * MAX(64.0f, caster->collision + 32.0f);
    loc.y += sinf(angle) * MAX(64.0f, caster->collision + 32.0f);
    SP_FindEmptySpaceAround(caster, unit_id, &loc, &angle);

    summon = SP_SpawnAtLocation(unit_id, caster->s.player, &loc);
    if (!summon) {
        return;
    }
    summon->owner = caster;
    if (summon->stand) {
        summon->stand(summon);
    }
    if (duration > 0) {
        unit_addtimedstatus(summon, ID_TIMED_LIFE, 1, duration);
    }
}

static void summon_command_with_fallback(LPEDICT clent, DWORD fallback) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    DWORD code = S_SpellCurrentCode(clent, fallback);
    DWORD level = S_SpellLevel(caster, code);
    DWORD unit_id = S_SpellUnitId(code, level);
    DWORD count = (DWORD)S_SpellData(code, level, 2);
    FLOAT duration = S_SpellDuration(code, level, false);

    if (!caster || !unit_id) {
        return;
    }
    if (!S_SpellCooldownReady(caster, code) || !S_SpellCanPay(caster, code, level)) {
        return;
    }
    if (count == 0) {
        count = 1;
    }
    S_SpellSpendMana(caster, code, level);
    S_SpellStartCooldown(caster, code, level);
    FOR_LOOP(i, count) {
        summon_unit(caster, unit_id, i, count, duration);
    }
}

static void water_elemental_command(LPEDICT clent) {
    summon_command_with_fallback(clent, ID_WATER_ELEMENTAL);
}

static void feral_spirit_command(LPEDICT clent) {
    summon_command_with_fallback(clent, ID_FERAL_SPIRIT);
}

ability_t a_water_elemental = {
    .cmd = water_elemental_command,
};

ability_t a_feral_spirit = {
    .cmd = feral_spirit_command,
};
