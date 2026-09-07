#ifndef s_skills_h
#define s_skills_h

#include "../g_local.h"

extern ability_t a_harvest;
extern ability_t a_move;
extern ability_t a_attack;
extern ability_t a_build;
extern ability_t a_train;
extern ability_t a_goldmine;
extern ability_t a_cancel;
extern ability_t a_repair;
extern ability_t a_stop;
extern ability_t a_holdpos;
BOOL S_HoldPosition(LPEDICT unit);
extern ability_t a_patrol;
extern ability_t a_rally;
extern ability_t a_call_to_arms;
extern ability_t a_militia;
BOOL S_MilitiaEnsureHallAbility(LPEDICT hall);
FLOAT S_MilitiaPairSearchRadius(DWORD ability);
extern ability_t a_selectskill;
extern ability_t a_devotionaura;
extern ability_t a_holylight;
extern ability_t a_thunderbolt;
extern ability_t a_firebolt;
extern ability_t a_water_elemental;
extern ability_t a_feral_spirit;
extern ability_t a_force_of_nature;
extern ability_t a_summon_bear;
extern ability_t a_summon_quilbeast;
extern ability_t a_summon_hawk;
extern ability_t a_mirror_image;
extern ability_t a_blizzard;
extern ability_t a_starfall;
extern ability_t a_carrion_swarm;
extern ability_t a_shockwave;
extern ability_t a_rain_of_fire;
extern ability_t a_death_and_decay;
extern ability_t a_thunder_clap;
extern ability_t a_frost_nova;
extern ability_t a_tranquility;
extern ability_t a_channel_test;
extern ability_t a_immolation;
extern ability_t a_phoenix_fire;
extern ability_t a_cold_arrows;
extern ability_t a_charm;
extern ability_t a_eat_tree;
extern ability_t a_moon_well;
extern ability_t a_invulnerable;
extern ability_t a_goldmine_overlayed;
extern ability_t a_blighted_goldmine;
extern ability_t a_blight;
extern ability_t a_acolyte_harvest;
extern ability_t a_return_resources;
extern ability_t a_wisp_harvest;
extern ability_t a_harvest_lumber;
extern ability_t a_repair_generic;
extern ability_t a_root;
extern ability_t a_blink;
extern ability_t a_fan_of_knives;
extern ability_t a_shadow_strike;
extern ability_t a_entangle_goldmine;
extern ability_t a_entangled_mine;
extern ability_t a_cargo_hold;
extern ability_t a_cargo_hold_burrow;
extern ability_t a_cargo_hold_entangled_mine;
extern ability_t a_stand_down;
extern ability_t a_load;
extern ability_t a_drop;
extern ability_t a_drop_instant;
extern ability_t a_inventory;
extern ability_t a_shop_purchase_item;
extern ability_t a_neutral_building;
extern ability_t a_shop_sharing;
extern ability_t a_couple_instant;
extern ability_t a_item_heal;
extern ability_t a_item_mana_regain;
extern ability_t a_item_attack_bonus;
extern ability_t a_item_stat_bonus;
extern ability_t a_item_permanent_stat_gain;
extern ability_t a_item_defense_bonus;
extern ability_t a_item_life_bonus;
extern ability_t a_item_mana_bonus;
extern ability_t a_item_figurine_summon;
extern ability_t a_item_permanent_life_gain;
extern ability_t a_item_experience_gain;
extern ability_t a_item_level_gain;
extern ability_t a_item_defense_aoe;
extern ability_t a_item_change_time;
extern ability_t a_flame_strike;
extern ability_t a_siphon_mana;
extern ability_t a_flame_strike_human;
extern ability_t a_siphon_mana_human;
extern ability_t a_mana_burn;
extern ability_t a_war_stomp;
extern ability_t a_aura_endurance;
extern ability_t a_wind_walk;
extern ability_t a_bash;
extern ability_t a_entangling_roots;
extern ability_t a_dark_ritual;
extern ability_t a_frost_armor;
extern ability_t a_frost_armor_variant;
extern ability_t a_divine_shield;
extern ability_t a_brilliance_aura;
extern ability_t a_critical_strike;
extern ability_t a_spiked_carapace;
extern ability_t a_unholy_aura;
extern ability_t a_evasion;
extern ability_t a_vampiric_aura;
extern ability_t a_aura_spell;
extern ability_t a_mana_shield;
extern ability_t a_drunken_brawler;
extern ability_t a_cleaving_attack;
FLOAT S_BrillianceManaRegen(LPEDICT unit);
FLOAT S_UnholyHealthRegen(LPEDICT unit);
FLOAT S_UnholyMoveBonus(LPEDICT unit);
FLOAT S_VampiricLifeSteal(LPEDICT unit);
FLOAT S_TrueshotAttackBonus(LPEDICT unit);
int S_SearingArrowDamage(LPEDICT attacker, int damage);
FLOAT S_ThornsDamageReturn(LPCEDICT target, LPCEDICT attacker, FLOAT damage);
BOOL S_EvasionRoll(LPEDICT target);
int S_CriticalStrikeDamage(LPEDICT attacker, int damage);
FLOAT S_SpikedArmorBonus(LPCEDICT unit);
FLOAT S_SpikedDamageReturn(LPCEDICT unit, FLOAT damage);
int S_ManaShieldDamage(LPEDICT target, int damage);
void S_SummonUnits(LPEDICT caster, DWORD unit_id, DWORD count, FLOAT duration);
LPEDICT S_SummonAt(LPEDICT caster, DWORD unit_id, LPCVECTOR2 loc, FLOAT duration);
BOOL S_UnitHasStatus(LPCEDICT unit, DWORD code);
int S_BlackArrowDamage(LPEDICT attacker, int damage);
void S_BlackArrowDeath(LPEDICT attacker, LPEDICT target);
void S_ResolveAttackHit(LPEDICT attacker, LPEDICT target, int damage);
void S_ReincarnationOnDeath(LPEDICT unit);

extern ability_t a_mass_teleport;
extern ability_t a_stomp;
extern ability_t a_whirlwind;
extern ability_t a_tornado;
extern ability_t a_banish;
extern ability_t a_phoenix;
extern ability_t a_carrion_beetles;
extern ability_t a_impale;
extern ability_t a_locust_swarm;
extern ability_t a_black_arrow;
extern ability_t a_silence;
extern ability_t a_animate_dead;
extern ability_t a_death_coil;
extern ability_t a_death_pact;
extern ability_t a_metamorphosis;
extern ability_t a_sleep;
extern ability_t a_inferno;
extern ability_t a_chain_lightning;
extern ability_t a_forked_lightning;
extern ability_t a_earthquake;
extern ability_t a_far_sight;
extern ability_t a_revive;
extern ability_t a_breath_of_fire;
extern ability_t a_howl_of_terror;
extern ability_t a_searing_arrows;
extern ability_t a_trueshot_aura;
extern ability_t a_reincarnation;
extern ability_t a_healing_wave;
extern ability_t a_hex;
extern ability_t a_vengeance;
extern ability_t a_big_bad_voodoo;
extern ability_t a_acid_bomb;
extern ability_t a_unimplemented;
extern ability_t a_attribute_bonus, a_spawn_tentacle, a_avatar_campaign, a_shockwave_campaign, a_war_stomp_campaign;
extern ability_t a_feral_spirit_campaign, a_spirit_beast, a_reincarnation_campaign, a_feedback, a_abolish_magic;
extern ability_t a_submerge_myrmidon, a_submerge_royal_guard, a_submerge_snap_dragon, a_ensnare, a_frost_armor_campaign;
extern ability_t a_parasite, a_cyclone_campaign, a_summoning_ritual, a_summon_quilbeast_campaign, a_summon_misha;
extern ability_t a_stampede_campaign, a_battle_roar, a_storm_bolt_campaign, a_breath_of_fire_campaign;
extern ability_t a_drunken_haze_campaign, a_storm_earth_fire, a_healing_wave_campaign, a_hex_campaign, a_serpent_ward;
extern ability_t a_shockwave_cairne, a_endurance_aura_campaign, a_reincarnation_cairne, a_voodoo_spirits;

FLOAT AB_Data(LPCSTR classname, DWORD level, DWORD index);
DWORD AB_DataId(LPCSTR classname, DWORD level, DWORD index);

typedef enum {
	RETURN_RESOURCE_GOLD = 1,
	RETURN_RESOURCE_LUMBER = 2,
} returnResource_t;

BOOL S_CanReturnResourceAt(LPEDICT unit, LPEDICT building, returnResource_t resource);
LPEDICT S_FindNearestResourceDropoff(LPEDICT unit, returnResource_t resource);
void S_SetCarriedResource(LPEDICT unit, returnResource_t resource, DWORD amount);

typedef enum {
	ABILITY_NUMBER_CAST,
	ABILITY_NUMBER_DURATION,
	ABILITY_NUMBER_HERO_DURATION,
	ABILITY_NUMBER_COOLDOWN,
	ABILITY_NUMBER_COST,
	ABILITY_NUMBER_AREA,
	ABILITY_NUMBER_RANGE
} abilityNumber_t;
DWORD S_SpellCurrentCode(LPEDICT clent, DWORD fallback);
spell_info_t const *S_SpellInfoForCode(DWORD code);
DWORD S_SpellLevel(LPEDICT caster, DWORD code);
FLOAT S_SpellNumber(DWORD code, abilityNumber_t field, DWORD level);
LPCSTR S_SpellString(DWORD code, LPCSTR field, DWORD level);
FLOAT S_SpellData(DWORD code, DWORD level, DWORD index);
DWORD S_SpellDataId(DWORD code, DWORD level, DWORD index);
DWORD S_SpellUnitId(DWORD code, DWORD level);
FLOAT S_SpellRange(DWORD code, DWORD level);
FLOAT S_SpellDuration(DWORD code, DWORD level, BOOL hero);
BOOL S_SpellCooldownReady(LPEDICT caster, DWORD code);
FLOAT S_SpellCooldownFraction(LPEDICT caster, DWORD code, DWORD level);
void S_SpellStartCooldown(LPEDICT caster, DWORD code, DWORD level);
BOOL S_SpellSpendMana(LPEDICT caster, DWORD code, DWORD level);
BOOL S_SpellCanPay(LPEDICT caster, DWORD code, DWORD level);
BOOL S_CastNoTargetSpell(LPEDICT caster, DWORD code);
BOOL S_SpellTargetInRange(LPEDICT caster, LPEDICT target, FLOAT range);
BOOL S_SpellIsAliveTarget(LPEDICT target);
BOOL S_SpellIsEnemy(LPEDICT caster, LPEDICT target);
BOOL S_SpellIsFriend(LPEDICT caster, LPEDICT target);
BOOL S_SpellAllowsTarget(DWORD code, LPEDICT caster, LPEDICT target);
void S_SpellHeal(LPEDICT target, FLOAT amount);
void S_SpellCursorSplat(LPEDICT clent, FLOAT radius);
void S_SpellCodeString(DWORD code, LPSTR out);
BOOL S_SpellIsChanneling(LPEDICT caster);
void S_SpellCancelChannel(LPEDICT caster);

/* Unified spell pipeline — replaces per-spell command boilerplate.
 * Single entry point for all spell abilities; handles target setup,
 * validation, and execution via the spell_info_t attached to ability_t. */
void spell_cmd(LPEDICT clent);
void spell_run_frame(LPEDICT ent);
void SP_ability_item_attack_bonus(LPCSTR classname, ability_t *self);
void SP_ability_item_defense_bonus(LPCSTR classname, ability_t *self);
void SP_ability_item_life_bonus(LPCSTR classname, ability_t *self);
void SP_ability_item_mana_bonus(LPCSTR classname, ability_t *self);
void SP_ability_item_stat_bonus(LPCSTR classname, ability_t *self);

#endif
