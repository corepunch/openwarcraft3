#include "s_skills.h"

/* Campaign rawcodes keep their own spell descriptor so every lookup uses the campaign AbilityData row. */
static LPCSTR campaign_buff(spell_info_t const *spell, DWORD level) {
    LPCSTR buff = G_AbilityLevel(spell->code, level)->buffID;
    return buff && strlen(buff) >= 4 ? buff : NULL;
}

static void campaign_status_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    LPCSTR buff = campaign_buff(spell, level);
    if (st.entity && buff) unit_addtimedstatus(st.entity, buff, level, S_SpellDuration(spell->code, level, G_UnitIsHero(st.entity)));
}

static void campaign_area_damage_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    DWORD damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) && Vector2_distance(&target->s.origin2, &st.point) <= area)
        S_SpellDamage(target, caster, damage);
}

static void campaign_stomp_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level), duration = S_SpellDuration(spell->code, level, false);
    DWORD damage = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    FILTER_EDICTS(target, target != caster && S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) && target->targtype == TARG_GROUND && Vector2_distance(&target->s.origin2, &caster->s.origin2) <= area) {
        S_SpellDamage(target, caster, damage);
        if (!M_IsDead(target) && duration > 0.0f) unit_addtimedstatus(target, "Bstu", 1, duration);
    }
}

static void campaign_summon_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1));
    DWORD unit = S_SpellUnitId(spell->code, level); FLOAT duration = S_SpellDuration(spell->code, level, false);
    if (st.type == SPELL_TARGET_POINT) { FOR_LOOP(i, count) S_SummonAt(caster, unit, &st.point, duration); }
    else S_SummonUnits(caster, unit, count, duration);
}

static void campaign_toggle_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        heroabilitystatus_t *status = caster->abilstatus + i;
        if (status->level && status->code == spell->code) { memset(status, 0, sizeof(*status)); return; }
    }
    unit_addstatus(caster, (LPCSTR)&spell->code, S_SpellLevel(caster, spell->code));
}

static void campaign_dispel_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), count = 0; FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FILTER_EDICTS(target, count < (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1)) && S_SpellIsAliveTarget(target) && S_SpellIsEnemy(caster, target) && Vector2_distance(&target->s.origin2, &st.point) <= area) {
        FOR_LOOP(i, MAX_UNIT_STATUSES) if (target->abilstatus[i].level && target->abilstatus[i].timestamp) memset(target->abilstatus + i, 0, sizeof(target->abilstatus[i]));
        count++;
    }
}

static void campaign_battle_roar_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code); LPCSTR buff = campaign_buff(spell, level);
    FLOAT area = S_SpellNumber(spell->code, ABILITY_NUMBER_AREA, level);
    FILTER_EDICTS(target, S_SpellIsAliveTarget(target) && S_SpellIsFriend(caster, target) && Vector2_distance(&target->s.origin2, &caster->s.origin2) <= area)
        if (buff) unit_addtimedstatus(target, buff, level, S_SpellDuration(spell->code, level, false));
}

static void campaign_storm_bolt_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code);
    if (!st.entity || !S_SpellIsAliveTarget(st.entity)) return;
    S_SpellDamage(st.entity, caster, (DWORD)MAX(1.0f, S_SpellData(spell->code, level, 1)));
    if (!M_IsDead(st.entity)) unit_addtimedstatus(st.entity, "Bstu", 1, S_SpellDuration(spell->code, level, G_UnitIsHero(st.entity)));
}

static void campaign_morph_execute(LPEDICT caster, spellTarget_t st, spell_info_t const *spell) {
    DWORD level = S_SpellLevel(caster, spell->code), form = S_SpellUnitId(spell->code, level);
    if (form) G_TransformUnitType(caster, form);
}

#define CAMPAIGN_SPELL(NAME, CODE, TARGET, FLAGS, EXECUTE) \
    static spell_info_t spell_##NAME = { .code = MAKEFOURCC CODE, .name = #NAME, .target_type = TARGET, .flags = FLAGS, .execute = EXECUTE }; \
    ability_t C##NAME = { .cmd = spell_cmd, .spell = &spell_##NAME }

CAMPAIGN_SPELL(AbilityAttributeModSkill, ('A','a','m','k'), SPELL_TARGET_NONE, 0, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilitySpawnTentacle, ('A','C','t','n'), SPELL_TARGET_POINT, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilityAvatarCampaign, ('A','N','a','v'), SPELL_TARGET_NONE, 0, campaign_morph_execute);
CAMPAIGN_SPELL(AbilityShockwaveCampaign, ('A','N','s','h'), SPELL_TARGET_POINT, 0, campaign_area_damage_execute);
CAMPAIGN_SPELL(AbilityWarStompCampaign, ('A','O','w','2'), SPELL_TARGET_NONE, 0, campaign_stomp_execute);
CAMPAIGN_SPELL(AbilityFeralSpiritCampaign, ('A','C','s','7'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilitySpiritBeast, ('A','C','s','8'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilityReincarnationCampaign, ('A','N','r','2'), SPELL_TARGET_NONE, 0, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilityFeedbackCampaign, ('A','f','b','b'), SPELL_TARGET_NONE, SPELL_TOGGLE, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilityAbolishMagic, ('A','n','d','m'), SPELL_TARGET_POINT, 0, campaign_dispel_execute);
CAMPAIGN_SPELL(AbilitySubmergeMyrmidon, ('A','s','b','1'), SPELL_TARGET_NONE, SPELL_TOGGLE, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilitySubmergeRoyalGuard, ('A','s','b','2'), SPELL_TARGET_NONE, SPELL_TOGGLE, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilitySubmergeSnapDragon, ('A','s','b','3'), SPELL_TARGET_NONE, SPELL_TOGGLE, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilityEnsnare, ('A','N','e','n'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilityFrostArmorCampaign, ('A','C','f','u'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilityParasiteCampaign, ('A','N','p','a'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilityCycloneCampaign, ('A','c','n','y'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilitySummoningRitual, ('A','h','n','l'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilitySummonQuilbeastCampaign, ('A','r','s','q'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilitySummonMisha, ('A','r','s','g'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilityStampedeCampaign, ('A','r','s','p'), SPELL_TARGET_POINT, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilityBattleRoar, ('A','N','b','r'), SPELL_TARGET_NONE, 0, campaign_battle_roar_execute);
CAMPAIGN_SPELL(AbilityStormBoltCampaign, ('A','N','s','b'), SPELL_TARGET_UNIT, 0, campaign_storm_bolt_execute);
CAMPAIGN_SPELL(AbilityBreathOfFireCampaign, ('A','N','c','f'), SPELL_TARGET_POINT, 0, campaign_area_damage_execute);
CAMPAIGN_SPELL(AbilityDrunkenHazeCampaign, ('A','c','d','h'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilityStormEarthFire, ('A','c','e','f'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilityHealingWaveCampaign, ('A','N','h','w'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilityHexCampaign, ('A','N','h','x'), SPELL_TARGET_UNIT, 0, campaign_status_execute);
CAMPAIGN_SPELL(AbilitySerpentWard, ('A','r','s','w'), SPELL_TARGET_POINT, 0, campaign_summon_execute);
CAMPAIGN_SPELL(AbilityShockwaveCairne, ('A','O','s','2'), SPELL_TARGET_POINT, 0, campaign_area_damage_execute);
CAMPAIGN_SPELL(AbilityEnduranceAuraCampaign, ('A','O','r','2'), SPELL_TARGET_NONE, 0, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilityReincarnationCairne, ('A','O','r','3'), SPELL_TARGET_NONE, 0, campaign_toggle_execute);
CAMPAIGN_SPELL(AbilityVoodooSpirits, ('A','O','l','s'), SPELL_TARGET_NONE, 0, campaign_summon_execute);
