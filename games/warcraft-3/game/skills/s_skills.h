#ifndef s_skills_h
#define s_skills_h

#include "../g_local.h"

/* TFT class hierarchy — abstract base classes (s_ability_classes.c) */
extern ability_t CAbility;               /* abil  — root */
extern ability_t CAbilityInterfaced;     /* AAin */
extern ability_t CBonusBase;             /* ABon */
extern ability_t CPower;                 /* powr */
extern ability_t CAbilityButton;         /* AAbt */
extern ability_t CAbilitySpell;          /* AAsp */
extern ability_t CAbilitySimpleSpell;    /* AAsm */
extern ability_t CAbilityModalSpell;     /* AAms */
extern ability_t CAbilityAutoTargetSpell;/* AAat */
extern ability_t CAbilityRangerArrow;    /* AHRa */
extern ability_t CAbilityPassive;        /* APas */
extern ability_t CAbilityNeutralSpell;   /* AAns */
extern ability_t CAbilityMorph;          /* Amor */
extern ability_t CAbilityPersistentBonus;/* APbo */
extern ability_t CAbilityRegenBase;      /* AREB */
extern ability_t CAbilityAura;           /* aura */
extern ability_t CAbilityBaseBuild;      /* ABbs */
void S_WireAbilityParents(void);

extern ability_t CAbilityHarvest;
extern ability_t CAbilityMove;
extern ability_t CAbilityRavenForm;
extern ability_t CAbilityAttack;
extern ability_t CAbilityBuild;
extern ability_t CAbilityTrain;
extern ability_t CAbilityGoldMine;
extern ability_t CAbilityCancel;
extern ability_t CAbilityRepair;
extern ability_t CAbilityStop;
extern ability_t CAbilityHoldPosition;
BOOL S_HoldPosition(LPEDICT unit);
extern ability_t CAbilityPatrol;
extern ability_t CAbilityRally;
extern ability_t CAbilityMilitiaConvert;
extern ability_t CAbilityMilitia;
BOOL S_MilitiaEnsureHallAbility(LPEDICT hall);
FLOAT S_MilitiaPairSearchRadius(DWORD ability);
extern ability_t CAbilitySelectSkill;
extern ability_t CAbilityAuraDevotion;
extern ability_t CAbilityHolyBolt;
extern ability_t CAbilityThunderBolt;
extern ability_t CAbilityFireBolt;
extern ability_t CAbilityWaterElemental;
extern ability_t CAbilitySpiritWolf;
extern ability_t CAbilityForceOfNature;
extern ability_t CAbilitySummonGrizzly;
extern ability_t CAbilitySummonQuillbeast;
extern ability_t CAbilitySummonWarEagle;
extern ability_t CAbilityMirrorImage;
extern ability_t CAbilityBlizzard;
extern ability_t CAbilityStarfall;
extern ability_t CAbilityCarrionSwarm;
extern ability_t CAbilityShockwave;
extern ability_t CAbilityRainOfFire;
extern ability_t CAbilityDeathAndDecay;
extern ability_t CAbilityThunderClap;
extern ability_t CAbilityFrostNova;
extern ability_t CAbilityTranquility;
extern ability_t CAbilityChannel;
extern ability_t CAbilityImmolation;
extern ability_t CAbilityPhoenixFire;
extern ability_t CAbilityColdArrows;
extern ability_t CAbilityCharm;
extern ability_t CAbilityEatTree;
extern ability_t CAbilityManaBattery;
extern ability_t CAbilityInvulnerable;
extern ability_t CAbilityGoldMineOverlayed;
extern ability_t CAbilityBlightedGoldMine;
extern ability_t CAbilityBlightGrowth;
extern ability_t CAbilityAcolyteHarvest;
extern ability_t CAbilityReturn;
extern ability_t CAbilityWispHarvest;
extern ability_t CAbilityHarvestLumber;
extern ability_t CAbilityRepairGeneric;
extern ability_t CAbilityRoot;
extern ability_t CAbilityBlink;
extern ability_t CAbilityFanOfKnives;
extern ability_t CAbilityShadowStrike;
extern ability_t CAbilityEntangle;
extern ability_t CAbilityEntangledGoldMine;
extern ability_t CAbilityCargoHold;
extern ability_t CAbilityBunker;
extern ability_t CAbilityEntangleCargo;
extern ability_t CAbilityBattlestations;
extern ability_t CAbilityStandDown;
extern ability_t CAbilityCargoLoad;
extern ability_t CAbilityCargoDrop;
extern ability_t CAbilityCargoDropInstant;
extern ability_t CAbilityInventory;
extern ability_t CAbilityPurchaseItem;
extern ability_t CAbilityNeutral;
extern ability_t CAbilityAllied;
extern ability_t CAbilityCoupleInstant;
extern ability_t CAbilityItemHeal;
extern ability_t CAbilityItemManaRestore;
extern ability_t CAbilityAttackBonus;
extern ability_t CAbilityAttributeBonus;
extern ability_t CAbilityStrengthMod;
extern ability_t CAbilityDefenseBonus;
extern ability_t CAbilityMaxLifeBonus;
extern ability_t CAbilityMaxManaBonus;
extern ability_t CAbilityFigurineSkeleton;
extern ability_t CAbilityMaxLifeMod;
extern ability_t CAbilityExperienceMod;
extern ability_t CAbilityLevelMod;
extern ability_t CAbilityItemDefenseAoe;
extern ability_t CAbilityItemChangeTOD;
extern ability_t CAbilityFlameStrikeNeutral;
extern ability_t CAbilityDrainNeutral;
extern ability_t CAbilityFlameStrike;
extern ability_t CAbilityDrain;
extern ability_t CAbilityManaBurn;
extern ability_t CAbilityStomp;
extern ability_t CAbilityAuraEndurance;
extern ability_t CAbilityWindWalk;
extern ability_t CAbilityBash;
extern ability_t CAbilityEntanglingRoots;
extern ability_t CAbilityDarkRitual;
extern ability_t CAbilityFrostArmor;
extern ability_t CAbilityFrostArmorAuto;
extern ability_t CAbilityDivineShield;
extern ability_t CAbilityAuraBrilliance;
extern ability_t CAbilityCriticalStrike;
extern ability_t CAbilityThornyShield;
extern ability_t CAbilityAuraUnholy;
extern ability_t CAbilityEvasion;
extern ability_t CAbilityAuraVampiric;
extern ability_t CAbilityAuraSpell;
extern ability_t CAbilityManaShield;
extern ability_t CAbilityDrunkenBrawler;
extern ability_t CAbilityCleavingAttack;
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
BOOL S_UnitPolymorphed(LPCEDICT unit);
void S_PolymorphRemove(LPEDICT unit);
int S_BlackArrowDamage(LPEDICT attacker, int damage);
void S_BlackArrowDeath(LPEDICT attacker, LPEDICT target);
void S_ResolveAttackHit(LPEDICT attacker, LPEDICT target, int damage);
void S_ReincarnationOnDeath(LPEDICT unit);

extern ability_t CAbilityMassTeleport;
extern ability_t CAbilityStampede;
extern ability_t CAbilityWhirlwind;
extern ability_t CAbilityTornado;
extern ability_t CAbilityBanish;
extern ability_t CAbilitySummonPhoenix;
extern ability_t CAbilityCarrionScarabs;
extern ability_t CAbilityImpale;
extern ability_t CAbilityLocustSwarm;
extern ability_t CAbilityBlackArrow;
extern ability_t CAbilitySilence;
extern ability_t CAbilityAnimateDead;
extern ability_t CAbilityDeathCoil;
extern ability_t CAbilityDeathPact;
extern ability_t CAbilityMetamorphosis;
extern ability_t CAbilitySleep;
extern ability_t CAbilityDreadLordInferno;
extern ability_t CAbilityChainLightning;
extern ability_t CAbilityForkedLightning;
extern ability_t CAbilityEarthquake;
extern ability_t CAbilityFarSight;
extern ability_t CAbilityResurrection;
extern ability_t CAbilityBreathOfFire;
extern ability_t CAbilityHowlOfTerror;
extern ability_t CAbilityFlamingArrows;
extern ability_t CAbilityAuraTrueshot;
extern ability_t CAbilityReincarnation;
extern ability_t CAbilityHealingWave;
extern ability_t CAbilityHex;
extern ability_t CAbilitySpiritOfVengeance;
extern ability_t CAbilityVoodoo;
extern ability_t CAbilityAcidBomb;
extern ability_t a_unimplemented;
extern ability_t CAbilityPoisonArrows;
extern ability_t CAbilityBurrowDetector, CAbilityRevive, CAbilityAwaken, CAbilityDetector, CAbilityHero, CAbilityAlarm;
extern ability_t CAbilityOnFireHuman;
extern ability_t CAbilityLocust, CAbilityTankTurret;
extern ability_t CAbilityAttributeModSkill, CAbilitySpawnTentacle, CAbilityAvatarCampaign, CAbilityShockwaveCampaign, CAbilityWarStompCampaign;
extern ability_t CAbilityFeralSpiritCampaign, CAbilitySpiritBeast, CAbilityReincarnationCampaign, CAbilityFeedback, CAbilityAbolishMagic;
extern ability_t CAbilitySubmergeMyrmidon, CAbilitySubmergeRoyalGuard, CAbilitySubmergeSnapDragon, CAbilityEnsnare, CAbilityFrostArmorCampaign;
extern ability_t CAbilityParasiteCampaign, CAbilityCycloneCampaign, CAbilitySummoningRitual, CAbilitySummonQuilbeastCampaign, CAbilitySummonMisha;
extern ability_t CAbilityStampedeCampaign, CAbilityBattleRoar, CAbilityStormBoltCampaign, CAbilityBreathOfFireCampaign;
extern ability_t CAbilityDrunkenHazeCampaign, CAbilityStormEarthFire, CAbilityHealingWaveCampaign, CAbilityHexCampaign, CAbilitySerpentWard;
extern ability_t CAbilityShockwaveCairne, CAbilityEnduranceAuraCampaign, CAbilityReincarnationCairne, CAbilityVoodooSpirits;
extern ability_t CAbilityMagicLeash, CAbilityFeedback, CAbilityControlMagic, CAbilityFlakCannon, CAbilityFragShards, CAbilityBarrage;
extern ability_t CAbilityMagicDefense, CAbilitySphere, CAbilitySpellSteal, CAbilityCloudOfFog, CAbilityPhoenix, CAbilityGyroBombs;
extern ability_t CAbilityStormHammers, CAbilityGyroVision, CAbilityDefend, CAbilityFlare, CAbilityMagicSentry, CAbilityInnerFire;
extern ability_t CAbilityDispelMagic, CAbilityHeal, CAbilitySlow, CAbilityInvisibility, CAbilityPolymorph, CAbilityAvatar;
extern ability_t CAbilityDoom, CAbilityDrunkenHaze;
void S_InitHumanAbilities(void);
void human_ability_think(LPEDICT thinker);
BOOL S_HumanCanAttack(LPCEDICT unit);
FLOAT S_HumanMoveFactor(LPCEDICT unit);
FLOAT S_HumanArmorBonus(LPCEDICT unit);
int S_HumanAttackDamage(LPEDICT attacker, LPEDICT target, int damage);
void S_HumanAttackSplash(LPEDICT attacker, LPEDICT target, int damage);
void S_HumanBreakInvisibility(LPEDICT unit);
void S_HumanStatusExpired(LPEDICT unit, DWORD code, DWORD level);
BOOL S_UnitSpellImmune(LPCEDICT unit);
BOOL S_SpellDamage(LPEDICT target, LPEDICT caster, int damage);
void S_AvatarExpire(LPEDICT unit);

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
FLOAT S_SpellCooldownRemaining(LPEDICT caster, DWORD code);
FLOAT S_SpellCooldownLength(LPEDICT caster, DWORD code);
BOOL S_SpellCooldownWindow(LPEDICT caster, DWORD code, abilityCooldownWindow_t *window);
FLOAT S_SpellCooldownFraction(LPEDICT caster, DWORD code, DWORD level);
void S_SpellStartCooldownDuration(LPEDICT caster, DWORD code, FLOAT seconds);
void S_SpellStartCooldown(LPEDICT caster, DWORD code, DWORD level);
void S_SpellEndCooldown(LPEDICT caster, DWORD code);
void S_SpellResetCooldowns(LPEDICT caster);
BOOL S_SpellSpendMana(LPEDICT caster, DWORD code, DWORD level);
BOOL S_SpellCanPay(LPEDICT caster, DWORD code, DWORD level);
BOOL S_CastNoTargetSpell(LPEDICT caster, DWORD code);
BOOL S_CastPointTargetSpell(LPEDICT caster, DWORD code, LPCVECTOR2 point);
BOOL S_CastUnitTargetSpell(LPEDICT caster, DWORD code, LPEDICT target);
BOOL S_IssueUnitTargetSpell(LPEDICT caster, DWORD code, LPEDICT target);
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
