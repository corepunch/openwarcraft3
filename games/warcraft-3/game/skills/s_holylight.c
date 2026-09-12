#include "s_skills.h"

void holylight_done(LPEDICT self);

static umove_t move_heal = { "stand channel", ai_idle, holylight_done, CAbilityHolyBolt };

void holylight_done(LPEDICT self) {
    self->stand(self);
}

/* Holy Light has unique target validation: friendlies are healed, undead
 * enemies take half-damage.  All other messages inherit CAbilitySimpleSpell. */
intptr_t CAbilityHolyBolt(LPEDICT caster, abilityMsg_t msg, abilityCall_t const *call) {
    switch (msg) {
    case A_VALIDATE: {
        spellTarget_t const *st = call ? call->target : NULL;
        LPEDICT target = st ? st->entity : NULL;

        if (!st || target == caster) return false;
        if (!S_SpellIsAliveTarget(target)) return false;
        if (S_SpellIsEnemy(caster, target)) {
            LPCSTR race = target->data.UnitData->race;
            return race && !strcmp(race, STR_UNDEAD);
        }
        return S_SpellIsFriend(caster, target);
    }
    case A_EXECUTE: {
        abilityitem_t const *spell = call ? call->item : NULL;
        spellTarget_t const *st = call ? call->target : NULL;
        LPEDICT target = st ? st->entity : NULL;
        DWORD level;
        FLOAT amount;

        if (!spell || !st) return false;
        level = S_SpellLevel(caster, spell->code);
        amount = S_SpellData(spell->code, level, 1);
        G_SpawnAbilityEffectTarget(spell->code, WC3_EFFECT_TARGET, 0, target, NULL, true);
        unit_setmove(caster, &move_heal);
        if (S_SpellIsFriend(caster, target))
            S_SpellHeal(target, amount);
        else
            S_SpellDamage(target, caster, (int)(amount * 0.5f));
        return true;
    }
    default:
        return CAbilitySimpleSpell(caster, msg, call);
    }
}
