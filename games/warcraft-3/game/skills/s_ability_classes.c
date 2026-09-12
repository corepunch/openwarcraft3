/* TFT ability class hierarchy — base class definitions.
 *
 * Mirrors the CAbility* inheritance tree extracted from Game.dll
 * (tft-ability-classes.txt). Each concrete ability_t sets its .parent
 * to one of these base classes (or to another concrete ability that
 * serves as an intermediate base). The chain terminates at CAbility
 * whose .parent is NULL.
 *
 * Hierarchy (ability-only subset):
 *
 *   CAbility (abil)
 *   ├── CAbilityInterfaced (AAin)
 *   │   ├── CAbilityBaseBuild (ABbs)
 *   │   │   └── CAbilityBuild (ABld), CAbilityBuildQueue (ABqu), ...
 *   │   ├── CAbilityHero (AHer)
 *   │   ├── CAbilityInventory (AInv)
 *   │   └── CAbilityRally (ARal)
 *   ├── CBonusBase (ABon)
 *   │   ├── CAbilityPersistentBonus (APbo)
 *   │   │   └── CAbilityRegenBase (AREB)
 *   │   └── CAbilityAura (aura)
 *   └── CPower (powr)
 *       ├── CAbilityButton (AAbt)
 *       │   ├── CAbilitySpell (AAsp)
 *       │   │   ├── CAbilitySimpleSpell (AAsm)
 *       │   │   │   ├── CAbilityModalSpell (AAms)
 *       │   │   │   │   └── CAbilityAutoTargetSpell (AAat)
 *       │   │   │   │       └── CAbilityRangerArrow (AHRa)
 *       │   │   │   └── (most concrete spells)
 *       │   │   ├── CAbilityMorph (Amor)
 *       │   │   └── CAbilityWaterElemental (AHwe) — summon spells
 *       │   ├── CAbilityPassive (APas)
 *       │   └── CAbilityNeutralSpell (AAns)
 *       ├── CAbilityAttack (Aatk)
 *       ├── CAbilityMove (Amov)
 *       ├── CAbilityCargoHold (Acar)
 *       └── CPower (Ahrb)
 */

#include "s_skills.h"

/* ── abstract base classes (no gameplay behavior of their own) ── */

ability_t CAbility          = {0};                                     /* abil  — root */
ability_t CAbilityInterfaced = { .parent = &CAbility };                /* AAin */
ability_t CBonusBase         = { .parent = &CAbility };                /* ABon */
ability_t CPower             = { .parent = &CAbilityInterfaced };      /* powr */
ability_t CAbilityButton     = { .parent = &CPower };                  /* AAbt */
ability_t CAbilitySpell      = { .parent = &CAbilityButton };          /* AAsp */
ability_t CAbilitySimpleSpell = { .parent = &CAbilitySpell,
                                   .cmd = spell_cmd };                 /* AAsm */
ability_t CAbilityModalSpell = { .parent = &CAbilitySimpleSpell };     /* AAms */
ability_t CAbilityAutoTargetSpell = { .parent = &CAbilityModalSpell }; /* AAat */
ability_t CAbilityRangerArrow = { .parent = &CAbilityAutoTargetSpell };/* AHRa */
ability_t CAbilityPassive    = { .parent = &CAbilityButton,
                                  .flags = ABILITY_PASSIVE };          /* APas */
ability_t CAbilityNeutralSpell = { .parent = &CAbilityButton };        /* AAns */
ability_t CAbilityMorph      = { .parent = &CAbilitySpell };           /* Amor */
ability_t CAbilityPersistentBonus = { .parent = &CBonusBase };         /* APbo */
ability_t CAbilityRegenBase  = { .parent = &CAbilityPersistentBonus }; /* AREB */
ability_t CAbilityAura       = { .parent = &CBonusBase };              /* aura */
ability_t CAbilityBaseBuild  = { .parent = &CAbilityInterfaced };      /* ABbs */

/* ── intermediate / leaf base classes from the TFT registry ── */

ability_t CAbilityClosestTargetSpell = { .parent = &CAbilitySimpleSpell };  /* AAcs */
ability_t CAbilityBuildQueue    = { .parent = &CAbilityBaseBuild };         /* ABqu — kept as alias for ABbs until queue behavior is split */
ability_t CAbilityBuildInProgress = { .parent = &CAbilityInProgress };      /* ABnP */
ability_t CAbilityInProgress    = { .parent = &CAbilityBaseBuild };         /* AInP */
ability_t CAbilityUpgradeInProgress = { .parent = &CAbilityInProgress };    /* AUnP */
ability_t CAbilitySacrificeInProgress = { .parent = &CAbilityBaseBuild };   /* ASnP */
ability_t CAbilityResearch      = { .parent = &CAbilityBaseBuild };         /* ARes */
ability_t CAbilityBaseSell      = { .parent = &CAbilityBaseBuild };         /* ASbs */
ability_t CAbilitySellItem      = { .parent = &CAbilityBaseSell };          /* Asei */
ability_t CAbilitySellUnit      = { .parent = &CAbilityBaseSell };          /* Asel */
ability_t CAbilityMakeItem      = { .parent = &CAbilitySellItem };          /* Amai */
ability_t CAbilityQueue         = { .parent = &CAbilityBaseBuild };         /* Aque */
ability_t CAbilityUpgrade       = { .parent = &CAbilityBaseBuild };         /* Aupg */
ability_t CAbilityRegenMana     = { .parent = &CAbilityRegenBase };         /* Arem */
ability_t CAbilityStarfallDrain = { .parent = &CAbility };                  /* AEsd */
ability_t CAbilityTranquilityRegen = { .parent = &CAbilityRegenBase };      /* AEtr → Arel → AREB (Arel = CAbilityRegenLife, not yet defined) */
ability_t CAbilityDamageBonusBase = { .parent = &CAbilityAttackBonus };     /* AIDB → AIat → APbo */
ability_t CAbilityDamageBonusBaseEx = { .parent = &CAbilityDamageBonusBase }; /* AIDE */
ability_t CAbilityItemCureAoe   = { .parent = &CAbilitySimpleSpell };       /* AIca */
ability_t CAbilityItemFlyingCarpet = { .parent = &CAbilitySimpleSpell };    /* AIfc */
ability_t CAbilityMaxManaMod    = { .parent = &CAbilitySimpleSpell };       /* AImn → AItm → AAsm */
ability_t CAbilityItemPermInvis = { .parent = &CAbility };                  /* AIpi → Apiv → ABon */
ability_t CAbilityItemTeleport  = { .parent = &CAbilitySimpleSpell };       /* AIte */
ability_t CAbilityNeutralG2L    = { .parent = &CAbilityNeutralSpell };      /* ANgl */
ability_t CAbilityNeutralL2G    = { .parent = &CAbilityNeutralSpell };      /* ANlg */
ability_t CAbilityMonsoonDrain  = { .parent = &CAbilityStarfallDrain };     /* ANmd */
ability_t CAbilityNeutralRegenLife = { .parent = &CAbilityRegenBase };      /* ANrl → Arel → AREB */
ability_t CAbilityNeutralRegenMana = { .parent = &CAbilityRegenMana };      /* ANrm → Arem → AREB */
ability_t CAbilityNeutralSpies  = { .parent = &CAbilityNeutralSpell };      /* ANsp */
ability_t CAbilityBarkskin      = { .parent = &CAbilityModalSpell };        /* Abar */
ability_t CAbilityBlightRegen   = { .parent = &CAbility };                  /* Ablr */
ability_t CAbilityBounce        = { .parent = &CAbilitySpell };             /* Abou */
ability_t CAbilityCallToArms    = { .parent = &CPower };                    /* Acal */
ability_t CAbilityChaosBase     = { .parent = &CAbility };                  /* Achb */
ability_t CAbilityCouple        = { .parent = &CPower };                    /* Acou */
ability_t CAbilityCargo         = { .parent = &CAbility };                  /* Acrg */
ability_t CAbilityNotifyDamage  = { .parent = &CAbility };                  /* Admg */
ability_t CAbilityDetectAoe     = { .parent = &CAbilitySimpleSpell };       /* Adta */
ability_t CAbilityGoldMineBase  = { .parent = &CAbility };                  /* Agmb */
ability_t CAbilityGraveyardCorpse = { .parent = &CAbility };                /* Agyc */
ability_t CAbilityHarvestBase   = { .parent = &CPower };                    /* Ahrb */
ability_t CAbilityHarvestReturn = { .parent = &CAbilityHarvestBase };       /* Ahrr */
ability_t CAbilityLeash         = { .parent = &CAbility };                  /* Alea */
ability_t CAbilityMount         = { .parent = &CAbilityMorph };             /* Amou */
ability_t CAbilityNeutralInteract = { .parent = &CAbilityNeutralSpell };    /* Anei */
ability_t CAbilityNightVision   = { .parent = &CAbility };                  /* Anit */
ability_t CAbilityPossessedGoldMine = { .parent = &CAbilityGoldMineBase };  /* Apgm */
ability_t CAbilityRescuable     = { .parent = &CAbility };                  /* Arsc */
ability_t CAbilityTankPilot     = { .parent = &CAbilitySimpleSpell };       /* Atpi */
ability_t CAbilityTarget        = { .parent = &CAbility };                  /* Atrg */

/* ── parent wiring for concrete abilities ──
 *
 * Sets the .parent pointer on every registered concrete ability_t
 * to match the TFT class hierarchy. Called once from InitAbilities
 * after all ability globals are initialized. Abilities whose parent
 * is another concrete ability (e.g. CAbilityFireBolt → CAbilityThunderBolt)
 * point directly at that concrete global — no intermediate abstract class
 * is needed.
 */

void S_WireAbilityParents(void) {
    /* ── engine commands & fundamental powers → CPower ── */
    CAbilityStop.parent = &CPower;
    CAbilityMove.parent = &CPower;
    CAbilityAttack.parent = &CPower;
    CAbilityHoldPosition.parent = &CPower;
    CAbilityPatrol.parent = &CPower;
    CAbilityCancel.parent = &CPower;
    CAbilityCargoHold.parent = &CPower;
    CAbilityAcolyteHarvest.parent = &CPower;

    /* ── interfaced (UI-carrying) abilities ── */
    CAbilityHero.parent = &CAbilityInterfaced;
    CAbilityInventory.parent = &CAbilityInterfaced;
    CAbilityRally.parent = &CAbilityInterfaced;
    CAbilityAwaken.parent = &CAbilityInterfaced;
    CAbilitySelectSkill.parent = &CAbilityInterfaced;

    /* ── build hierarchy → CAbilityBaseBuild ── */
    CAbilityBuild.parent = &CAbilityBaseBuild;
    CAbilityTrain.parent = &CAbilityBaseBuild;
    CAbilityRevive.parent = &CAbilityBaseBuild;

    /* ── bonus / aura / regen hierarchy ── */
    CAbilityInvulnerable.parent = &CBonusBase;
    CAbilitySphere.parent = &CBonusBase;
    CAbilityAuraDevotion.parent = &CAbilityAura;
    CAbilityAuraBrilliance.parent = &CAbilityAura;
    CAbilityAuraEndurance.parent = &CAbilityAura;
    CAbilityAuraTrueshot.parent = &CAbilityAura;
    CAbilityAuraSpell.parent = &CAbilitySpell; /* Aasp parent=AAsp */
    CAbilityAuraUnholy.parent = &CAbilityAura;
    CAbilityAuraVampiric.parent = &CAbilityAura;
    CAbilityAttackBonus.parent = &CAbilityPersistentBonus;
    CAbilityDefenseBonus.parent = &CAbilityPersistentBonus;
    CAbilityMaxLifeBonus.parent = &CAbilityPersistentBonus;
    CAbilityMaxManaBonus.parent = &CAbilityPersistentBonus;
    CAbilityAttributeBonus.parent = &CAbilityPersistentBonus;

    /* ── CAbilityButton subclasses ── */
    CAbilityDefend.parent = &CAbilityButton;
    CAbilityRoot.parent = &CAbilityButton;

    /* ── passive abilities → CAbilityPassive ── */
    CAbilityAttributeModSkill.parent = &CAbilityPassive;
    CAbilityBurrowDetector.parent = &CAbilityPassive; /* Abdt→Adet→APas */
    CAbilityDetector.parent = &CAbilityPassive;
    CAbilityCriticalStrike.parent = &CAbilityPassive;
    CAbilityEvasion.parent = &CAbilityPassive;
    CAbilityBash.parent = &CAbilityCriticalStrike; /* AHbh→AOcr→APas */
    CAbilityDrunkenBrawler.parent = &CAbilityCriticalStrike; /* ANdb→AOcr */
    CAbilityCleavingAttack.parent = &CAbilityPassive;
    CAbilityPhoenixFire.parent = &CAbilityPassive;
    CAbilityFeedback.parent = &CAbilityPassive;
    CAbilityFlakCannon.parent = &CAbilityPassive;
    CAbilityFragShards.parent = &CAbilityPassive;
    CAbilityBarrage.parent = &CAbilityPassive;
    CAbilityGyroBombs.parent = &CAbilityPassive;
    CAbilityStormHammers.parent = &CAbilityPassive;
    CAbilityGyroVision.parent = &CAbilityPassive; /* Agyv→Adet→APas */
    CAbilityMagicSentry.parent = &CAbilityPassive;
    CAbilityThornyShield.parent = &CAbilityPassive; /* AUts→ANth→APas */

    /* ── autocast spells → CAbilityAutoTargetSpell ── */
    CAbilityHeal.parent = &CAbilityAutoTargetSpell;
    CAbilityInnerFire.parent = &CAbilityAutoTargetSpell;
    CAbilitySlow.parent = &CAbilityAutoTargetSpell;
    CAbilitySpellSteal.parent = &CAbilityAutoTargetSpell;
    CAbilityRepair.parent = &CAbilityAutoTargetSpell;
    CAbilityRepairGeneric.parent = &CAbilityRepair; /* Aren→Arep */
    CAbilityManaBattery.parent = &CAbilityAutoTargetSpell;
    CAbilityFrostArmorAuto.parent = &CAbilityAutoTargetSpell;

    /* ── ranger arrow variants → CAbilityRangerArrow ── */
    CAbilityPoisonArrows.parent = &CAbilityRangerArrow;
    CAbilityColdArrows.parent = &CAbilityRangerArrow;
    CAbilityFlamingArrows.parent = &CAbilityRangerArrow;
    CAbilityBlackArrow.parent = &CAbilityFlamingArrows; /* ANba→AHfa→AHRa */

    /* ── simple spells → CAbilitySimpleSpell ── */
    CAbilityHolyBolt.parent = &CAbilitySimpleSpell;
    CAbilityThunderBolt.parent = &CAbilitySimpleSpell;
    CAbilityFireBolt.parent = &CAbilityThunderBolt; /* ANfb→AHtb */
    CAbilityBlizzard.parent = &CAbilitySimpleSpell;
    CAbilityFlameStrike.parent = &CAbilitySimpleSpell;
    CAbilityFlameStrikeNeutral.parent = &CAbilityFlameStrike; /* implicit */
    CAbilityBanish.parent = &CAbilitySimpleSpell;
    CAbilityMagicLeash.parent = &CAbilitySimpleSpell;
    CAbilityControlMagic.parent = &CAbilitySimpleSpell;
    CAbilityDrain.parent = &CAbilitySimpleSpell;
    CAbilityDrainNeutral.parent = &CAbilityDrain;
    CAbilityDispelMagic.parent = &CAbilitySimpleSpell;
    CAbilityInvisibility.parent = &CAbilitySimpleSpell;
    CAbilityPolymorph.parent = &CAbilitySimpleSpell;
    CAbilityCloudOfFog.parent = &CAbilitySilence; /* Aclf→ANsi */
    CAbilityDrunkenHaze.parent = &CAbilitySilence; /* ANdh→ANsi */
    CAbilitySilence.parent = &CAbilitySimpleSpell;
    CAbilityManaShield.parent = &CAbilitySimpleSpell;
    CAbilityMassTeleport.parent = &CAbilitySimpleSpell;
    CAbilityBlink.parent = &CAbilitySimpleSpell;
    CAbilityFanOfKnives.parent = &CAbilitySimpleSpell;
    CAbilityShadowStrike.parent = &CAbilitySimpleSpell;
    CAbilityManaBurn.parent = &CAbilitySimpleSpell;
    CAbilityEntanglingRoots.parent = &CAbilitySimpleSpell;
    CAbilityForceOfNature.parent = &CAbilitySimpleSpell;
    CAbilityEatTree.parent = &CAbilitySimpleSpell;
    CAbilityCarrionSwarm.parent = &CAbilitySimpleSpell;
    CAbilityShockwave.parent = &CAbilityCarrionSwarm; /* AOsh→AUcs */
    CAbilityBreathOfFire.parent = &CAbilityCarrionSwarm; /* ANbf→AUcs */
    CAbilityFrostNova.parent = &CAbilitySimpleSpell;
    CAbilityThunderClap.parent = &CAbilityFrostNova; /* AHtc→AUfn */
    CAbilityStomp.parent = &CAbilityThunderClap; /* AOws→AHtc */
    CAbilityFrostArmor.parent = &CAbilitySimpleSpell;
    CAbilityDarkRitual.parent = &CAbilitySimpleSpell;
    CAbilityDeathPact.parent = &CAbilityDarkRitual; /* AUdp→AUdr */
    CAbilityDeathCoil.parent = &CAbilitySimpleSpell;
    CAbilityDeathAndDecay.parent = &CAbilitySimpleSpell;
    CAbilityAnimateDead.parent = &CAbilitySimpleSpell;
    CAbilityResurrection.parent = &CAbilityAnimateDead; /* AHre→AUan */
    CAbilitySleep.parent = &CAbilitySimpleSpell;
    CAbilityDreadLordInferno.parent = &CAbilitySimpleSpell; /* AUin→ANin→AAsm */
    CAbilityImpale.parent = &CAbilitySimpleSpell;
    CAbilityLocustSwarm.parent = &CAbilitySimpleSpell;
    CAbilityChainLightning.parent = &CAbilitySimpleSpell;
    CAbilityForkedLightning.parent = &CAbilitySimpleSpell;
    CAbilityHealingWave.parent = &CAbilityChainLightning; /* AOhw→AOcl */
    CAbilityHex.parent = &CAbilitySimpleSpell; /* AOhx→Aply→AAsm */
    CAbilityEarthquake.parent = &CAbilitySimpleSpell;
    CAbilityFarSight.parent = &CAbilitySimpleSpell; /* AOfs→Adta→AAsm */
    CAbilityStampede.parent = &CAbilitySimpleSpell;
    CAbilityAcidBomb.parent = &CAbilitySimpleSpell;
    CAbilityDoom.parent = &CAbilitySimpleSpell;
    CAbilityTornado.parent = &CAbilitySimpleSpell;
    CAbilityVoodoo.parent = &CAbilitySimpleSpell;
    CAbilityBattleRoar.parent = &CAbilitySimpleSpell;
    CAbilityChannel.parent = &CAbilitySimpleSpell;
    CAbilityFlare.parent = &CAbilitySimpleSpell;
    CAbilityCargoLoad.parent = &CAbilitySimpleSpell;
    CAbilityCargoDrop.parent = &CAbilitySimpleSpell;

    /* ── spell (non-simple) abilities → CAbilitySpell ── */
    CAbilityAvatar.parent = &CAbilitySpell;
    CAbilityDivineShield.parent = &CAbilitySpell;
    CAbilityWindWalk.parent = &CAbilitySpell;
    CAbilityMirrorImage.parent = &CAbilitySpell;
    CAbilityReincarnation.parent = &CAbilitySpell;
    CAbilityBattlestations.parent = &CAbilitySpell;
    CAbilityStandDown.parent = &CAbilitySpell;
    CAbilityImmolation.parent = &CAbilitySpell;
    CAbilityCargoDropInstant.parent = &CAbilitySpell;
    CAbilityHowlOfTerror.parent = &CAbilitySpell; /* ANht→Aroa→AAsp */

    /* ── summon spells → CAbilityWaterElemental (AHwe→AAsp) ── */
    CAbilityWaterElemental.parent = &CAbilitySpell;
    CAbilitySpiritWolf.parent = &CAbilitySpell; /* AOsf→AAsp */
    CAbilitySummonGrizzly.parent = &CAbilityWaterElemental; /* ANsg→AHwe */
    CAbilitySummonQuillbeast.parent = &CAbilityWaterElemental;
    CAbilitySummonWarEagle.parent = &CAbilityWaterElemental;
    CAbilitySummonPhoenix.parent = &CAbilityWaterElemental;
    CAbilitySpiritOfVengeance.parent = &CAbilityWaterElemental;

    /* ── morph abilities → CAbilityMorph ── */
    CAbilityRavenForm.parent = &CAbilityMorph;
    CAbilityMetamorphosis.parent = &CAbilityMorph;
    CAbilityPhoenix.parent = &CAbilityMorph; /* Aphx→Amor */

    /* ── tranquility / starfall → CAbilitySpell ── */
    CAbilityTranquility.parent = &CAbilitySpell;
    CAbilityStarfall.parent = &CAbilityTranquility; /* AEsf→AEtq→AAsp */

    /* ── cargo variants → CAbilityCargoHold ── */
    CAbilityBunker.parent = &CAbilityCargoHold; /* Abun→Acar */
    CAbilityEntangleCargo.parent = &CAbilityCargoHold;

    /* ── harvest hierarchy → CPower ── */
    CAbilityHarvest.parent = &CPower; /* Ahar→...→powr */
    CAbilityHarvestLumber.parent = &CPower;
    CAbilityWispHarvest.parent = &CPower;

    /* ── goldmine → CAbility ── */
    CAbilityGoldMine.parent = &CAbility;
    CAbilityBlightedGoldMine.parent = &CAbility;
    CAbilityEntangledGoldMine.parent = &CAbility;
    CAbilityGoldMineOverlayed.parent = &CAbility;
    CAbilityEntangle.parent = &CPower;
    CAbilityReturn.parent = &CAbility;

    /* ── misc root-level abilities → CAbility ── */
    CAbilityAlarm.parent = &CAbility;
    CAbilityOnFireHuman.parent = &CAbility;
    CAbilityLocust.parent = &CAbility;
    CAbilityBlightGrowth.parent = &CAbility;
    CAbilityNeutral.parent = &CAbility;
    CAbilityAllied.parent = &CAbility;
    CAbilityPurchaseItem.parent = &CAbility;
    CAbilityMilitia.parent = &CPower; /* Amil→Acal→powr */
    CAbilityMilitiaConvert.parent = &CPower;

    /* ── item abilities ── */
    CAbilityItemHeal.parent = &CAbilitySimpleSpell;
    CAbilityItemManaRestore.parent = &CAbilitySimpleSpell;
    CAbilityItemDefenseAoe.parent = &CAbilitySimpleSpell;
    CAbilityCharm.parent = &CAbilitySimpleSpell;
    CAbilityFigurineSkeleton.parent = &CAbilitySimpleSpell;
    CAbilityStrengthMod.parent = &CAbilitySimpleSpell; /* AIsm→AIxm→AAsm */
    CAbilityMaxLifeMod.parent = &CAbilitySimpleSpell; /* AImi→AItm→AAsm */
    CAbilityExperienceMod.parent = &CAbilitySimpleSpell;
    CAbilityLevelMod.parent = &CAbilitySimpleSpell;
    CAbilityItemChangeTOD.parent = &CAbilitySpell;

    /* ── couple / misc ── */
    CAbilityCoupleInstant.parent = &CAbilitySimpleSpell;
    CAbilityCarrionScarabs.parent = &CAbilityAutoTargetSpell; /* AUcb→Arai→AAat */
    CAbilityMagicDefense.parent = &CAbilityDefend; /* Amdf→Adef */
}
