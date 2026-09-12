#include "s_skills.h"

#ifdef WC3_DEBUG_AUTOCAST
int G_AutocastDebugLevel(void) {
    LPCSTR value;

    value = gi.CvarString("wc3_autocast_debug", "0");
    return value ? atoi(value) : 0;
}
#endif

typedef struct {
    LPCSTR classname;
    ability_t *ability;
} abilityitem_t;

static abilityitem_t abilitylist[] = {
    { STR_CmdStop, &CAbilityStop },  // Stop — engine command
    { STR_CmdMove, &CAbilityMove },  // Move — engine command
    { STR_CmdAttack, &CAbilityAttack },  // Attack — engine command
    { STR_CmdBuild, &CAbilityBuild },  // Build — engine command
    { STR_CmdHoldPos, &CAbilityHoldPosition },  // Hold Position — engine command
    { STR_CmdPatrol, &CAbilityPatrol },  // Patrol — engine command
    { STR_CmdRally, &CAbilityRally },  // Rally — engine command
    { STR_CmdCancel, &CAbilityCancel },  // Cancel — engine command
    { STR_CmdCancelBuild, &CAbilityCancel },  // Cancel Build — engine command
    { STR_CmdSelectSkill, &CAbilitySelectSkill },  // Select Skill — engine command

    { "Amrf", &CAbilityRavenForm },  /* Medivh Crow Form */
    { "Arav", &CAbilityRavenForm },  /* Storm Crow Form */

    /* BEGIN GENERATED ABILITY STRINGS */

    /* CampaignAbilityStrings.txt */
    { "Aamk", &CAbilityAttributeModSkill },  /* Attribute Bonus */
    { "ACtn", &CAbilitySpawnTentacle },  /* Spawn Tentacle */
    { "ACs7", &CAbilityFeralSpiritCampaign },  /* Feral Spirit */
    { "ANav", &CAbilityAvatarCampaign },  /* Avatar */
    { "ANsh", &CAbilityShockwaveCampaign },  /* Shockwave */
    { "ACs8", &CAbilitySpiritBeast },  /* Spirit Beast */
    { "ANr2", &CAbilityReincarnationCampaign },  /* Reincarnation */
    { "Afbb", &CAbilityFeedback },  /* Feedback */
    { "Andm", &CAbilityAbolishMagic },  /* Abolish Magic */
    { "Asb1", &CAbilitySubmergeMyrmidon },  /* Submerge */
    { "Asb2", &CAbilitySubmergeRoyalGuard },  /* Submerge */
    { "Asb3", &CAbilitySubmergeSnapDragon },  /* Submerge */
    { "ANha", &CAbilityHarvest },  /* Harvest */
    { "ANen", &CAbilityEnsnare },  /* Ensnare */
    { "ACfu", &CAbilityFrostArmorCampaign },  /* Frost Armor */
    { "ANpa", &CAbilityParasiteCampaign },  /* Parasite */
    { "Acny", &CAbilityCycloneCampaign },  /* Cyclone */
    { "Ahnl", &CAbilitySummoningRitual },  /* Summoning Ritual */
    { "ANcl", &CAbilityChannel },  /* Channel */
    { "Arsq", &CAbilitySummonQuilbeastCampaign },  /* Summon Quilbeast */
    { "Arsg", &CAbilitySummonMisha },  /* Summon Misha */
    { "Arsp", &CAbilityStampedeCampaign },  /* Stampede */
    { "ANbr", &CAbilityBattleRoar },  /* Battle Roar */
    { "ANsb", &CAbilityStormBoltCampaign },  /* Storm Bolt */
    { "ANcf", &CAbilityBreathOfFireCampaign },  /* Breath of Fire */
    { "Acdh", &CAbilityDrunkenHazeCampaign },  /* Drunken Haze */
    { "Acef", &CAbilityStormEarthFire },  /* "Storm, Earth, And Fire" */
    { "ANhw", &CAbilityHealingWaveCampaign },  /* Healing Wave */
    { "ANhx", &CAbilityHexCampaign },  /* Hex */
    { "Arsw", &CAbilitySerpentWard },  /* Serpent Ward */
    { "AOr2", &CAbilityEnduranceAuraCampaign },  /* Endurance Aura */
    { "AOs2", &CAbilityShockwaveCairne },  /* Shockwave */
    { "AOr3", &CAbilityReincarnationCairne },  /* Reincarnation */
    { "AOw2", &CAbilityWarStompCampaign },  /* War Stomp */
    { "AOls", &CAbilityVoodooSpirits },  /* Voodoo Spirits */

    /* CommonAbilityStrings.txt */
    { "Aall", &CAbilityAllied },  /* Shop Sharing, Allied Bldg. */
    { "Abdt", &CAbilityBurrowDetector },  /* Burrow Detection */
    { "Apit", &CAbilityPurchaseItem },  /* Shop Purchase Item */
    { "Ahar", &CAbilityHarvest },  /* Harvest */
    { "Ahrl", &CAbilityHarvestLumber },  /* Harvest */
    { "Arev", &CAbilityRevive },  /* Revive Hero */
    { "Aawa", &CAbilityAwaken },  /* Revive Hero Instantly */
    { "Adet", &CAbilityDetector },  /* Detector */
    { "Arep", &CAbilityRepair },  /* Repair */
    { "AEpa", &CAbilityPoisonArrows },  /* Poison Arrows */
    { "AEbu", &CAbilityBuild },  /* Build (Night Elf) */
    { "AGbu", &CAbilityBuild },  /* Build (Naga) */
    { "AHbu", &CAbilityBuild },  /* Build (Human) */
    { "AHer", &CAbilityHero },  /* Hero */
    { "ANbu", &CAbilityBuild },  /* Build (Neutral) */
    { "AObu", &CAbilityBuild },  /* Build (Orc) */
    { "ARal", &CAbilityRally },  /* Rally */
    { "AUbu", &CAbilityBuild },  /* Build (Undead) */
    { "Aalr", &CAbilityAlarm },  /* Alarm */
    { "Aatk", &CAbilityAttack },  /* Attack */
    { "Afih", &CAbilityOnFireHuman },  /* On Fire (Human) */
    { "Afin", &CAbilityOnFireHuman },  /* On Fire (Night Elf) */
    { "Afio", &CAbilityOnFireHuman },  /* On Fire (Orc) */
    { "Afir", &CAbilityOnFireHuman },  /* On Fire */
    { "Afiu", &CAbilityOnFireHuman },  /* On Fire (Undead) */
    { "Aloc", &CAbilityLocust },  /* Locust */
    { "Amov", &CAbilityMove },  /* Move */
    { "Atdp", &CAbilityCargoDrop },  /* Drop Pilot */
    { "Atlp", &CAbilityCargoLoad },  /* Load Pilot */
    { "Attu", &CAbilityTankTurret },  /* Turret */

    /* HumanAbilityStrings.txt */
    { "Amls", &CAbilityMagicLeash },  /* Aerial Shackles */
    { "Afbk", &CAbilityFeedback },  /* Feedback */
    { "Acmg", &CAbilityControlMagic },  /* Control Magic */
    { "AHdr", &CAbilityDrain },  /* Siphon Mana */
    { "Aflk", &CAbilityFlakCannon },  /* Flak Cannons */
    { "Afsh", &CAbilityFragShards },  /* Fragmentation Shards */
    { "Aroc", &CAbilityBarrage },  /* Barrage */
    { "Amdf", &CAbilityMagicDefense },  /* Magic Defense */
    { "Asph", &CAbilitySphere },  /* Sphere */
    { "Asps", &CAbilitySpellSteal },  /* Spell Steal */
    { "Aclf", &CAbilityCloudOfFog },  /* Cloud */
    { "AHfs", &CAbilityFlameStrike },  /* Flame Strike */
    { "AHbn", &CAbilityBanish },  /* Banish */
    { "AHpx", &CAbilitySummonPhoenix },  /* Phoenix */
    { "Aphx", &CAbilityPhoenix },  /* Phoenix Morphing (Egg Related) */
    { "Apxf", &CAbilityPhoenixFire },  /* Phoenix Fire */
    { "Agyb", &CAbilityGyroBombs },  /* Flying Machine Bombs */
    { "Asth", &CAbilityStormHammers },  /* Storm Hammers */
    { "Agyv", &CAbilityGyroVision },  /* True Sight */
    { "Adef", &CAbilityDefend },  /* Defend */
    { "Afla", &CAbilityFlare },  /* Flare */
    { "Adts", &CAbilityMagicSentry },  /* Magic Sentry */
    { "Ainf", &CAbilityInnerFire },  /* Inner Fire */
    { "Adis", &CAbilityDispelMagic },  /* Dispel Magic */
    { "Ahea", &CAbilityHeal },  /* Heal */
    { "Aslo", &CAbilitySlow },  /* Slow */
    { "Aivs", &CAbilityInvisibility },  /* Invisibility */
    { "Aply", &CAbilityPolymorph },  /* Polymorph */
    { "AHbz", &CAbilityBlizzard },  /* Blizzard */
    { "AHwe", &CAbilityWaterElemental },  /* Summon Water Elemental */
    { "AHab", &CAbilityAuraBrilliance },  /* Brilliance Aura */
    { "AHmt", &CAbilityMassTeleport },  /* Mass Teleport */
    { "AHtb", &CAbilityThunderBolt },  /* Storm Bolt */
    { "AHtc", &CAbilityThunderClap },  /* Thunder Clap */
    { "AHbh", &CAbilityBash },  /* Bash */
    { "AHav", &CAbilityAvatar },  /* Avatar */
    { "AHhb", &CAbilityHolyBolt },  /* Holy Light */
    { "AHds", &CAbilityDivineShield },  /* Divine Shield */
    { "AHad", &CAbilityAuraDevotion },  /* Devotion Aura */
    { "AHre", &CAbilityResurrection },  /* Resurrection */
    { "Amil", &CAbilityMilitia },  /* Call to Arms */
    { "Amic", &CAbilityMilitiaConvert },  /* Call To Arms */

    /* ItemAbilityStrings.txt */
    { "AIsm", &CAbilityStrengthMod },  /* Item Strength Gain */
    { "AIam", &CAbilityStrengthMod },  /* Item Agility Gain */
    { "AIat", &CAbilityAttackBonus },  /* Item Damage Bonus */
    { "AIde", &CAbilityDefenseBonus },  /* Item Armor Bonus */
    { "AIem", &CAbilityExperienceMod },  /* Item Experience Gain */
    { "AIlm", &CAbilityLevelMod },  /* Item Level Gain */
    { "AIim", &CAbilityStrengthMod },  /* Item Intelligence Gain */
    { "AIxm", &CAbilityStrengthMod },  /* Item Int/Agi/Str gain */
    { "AIhe", &CAbilityItemHeal },  /* Item Healing */
    { "AIma", &CAbilityItemManaRestore },  /* Item Mana Regain */
    { "AIda", &CAbilityItemDefenseAoe },  /* Item Temporary Area Armor Bonus */
    { "AIco", &CAbilityCharm },  /* Item Command */
    { "AIfs", &CAbilityFigurineSkeleton },  /* Item Skeleton Summon */
    { "AImi", &CAbilityMaxLifeMod },  /* Item Permanent Life Gain */
    { "AIab", &CAbilityAttributeBonus },  /* Item Hero Stat Bonus */
    { "AIml", &CAbilityMaxLifeBonus },  /* Item Life Bonus */
    { "AImm", &CAbilityMaxManaBonus },  /* Item Mana Bonus */
    { "AIct", &CAbilityItemChangeTOD },  /* Change Time of Day */

    /* NeutralAbilityStrings.txt */
    { "ANab", &CAbilityAcidBomb },  /* Acid Bomb */
    { "ANms", &CAbilityManaShield },  /* Mana Shield */
    { "ANrf", &CAbilityRainOfFire },  /* Rain of Fire */
    { "AHca", &CAbilityColdArrows },  /* Cold Arrows */
    { "ANht", &CAbilityHowlOfTerror },  /* Howl of Terror */
    { "ANca", &CAbilityCleavingAttack },  /* Cleaving Attack */
    { "ANdo", &CAbilityDoom },  /* Doom */
    { "ANdr", &CAbilityDrainNeutral },  /* Life Drain */
    { "ANbf", &CAbilityBreathOfFire },  /* Breath of Fire */
    { "ANsg", &CAbilitySummonGrizzly },  /* Summon Bear */
    { "ANsq", &CAbilitySummonQuillbeast },  /* Summon Quilbeast */
    { "ANsw", &CAbilitySummonWarEagle },  /* Summon Hawk */
    { "ANst", &CAbilityStampede },  /* Stampede */
    { "ANfs", &CAbilityFlameStrikeNeutral },  /* Flame Strike */
    { "AInv", &CAbilityInventory },  /* Inventory */
    { "ANdb", &CAbilityDrunkenBrawler },  /* Drunken Brawler */
    { "ANdh", &CAbilityDrunkenHaze },  /* Drunken Haze */
    { "ANsi", &CAbilitySilence },  /* Silence */
    { "ANba", &CAbilityBlackArrow },  /* Black Arrow */
    { "ANch", &CAbilityCharm },  /* Charm */
    { "ANto", &CAbilityTornado },  /* Tornado */
    { "Abgm", &CAbilityBlightedGoldMine },  /* Blighted Gold Mine Ability */
    { "Aegm", &CAbilityEntangledGoldMine },  /* Entangled Gold Mine Ability */
    { "Aloa", &CAbilityCargoLoad },  /* Load */
    { "Adro", &CAbilityCargoDrop },  /* Unload */
    { "Adri", &CAbilityCargoDropInstant },  /* Unload Instant */
    { "Abun", &CAbilityBunker },  /* Cargo Hold (Orc Burrow) */
    { "Acar", &CAbilityCargoHold },  /* Cargo Hold */
    { "Aneu", &CAbilityNeutral },  /* Select Hero */
    { "ANfb", &CAbilityFireBolt },  /* Firebolt */
    { "Agld", &CAbilityGoldMine },  /* Gold Mine ability */
    { "Artn", &CAbilityReturn },  /* Return */
    { "Avul", &CAbilityInvulnerable },  /* Invulnerable */
    { "Abli", &CAbilityBlightGrowth },  /* Blight */
    { "ANfl", &CAbilityForkedLightning },  /* Forked Lightning */

    /* NightElfAbilityStrings.txt */
    { "AEbl", &CAbilityBlink },  /* Blink */
    { "AEfk", &CAbilityFanOfKnives },  /* Fan of Knives */
    { "AEsh", &CAbilityShadowStrike },  /* Shadow Strike */
    { "AEsv", &CAbilitySpiritOfVengeance },  /* Vengeance */
    { "Aeat", &CAbilityEatTree },  /* Eat Tree */
    { "Ambt", &CAbilityManaBattery },  /* Replenish Mana and Life */
    { "Awha", &CAbilityWispHarvest },  /* Gather */
    { "Aent", &CAbilityEntangle },  /* Entangle Gold Mine */
    { "Aenc", &CAbilityEntangleCargo },  /* Load */
    { "Aroo", &CAbilityRoot },  /* Root */
    { "AEmb", &CAbilityManaBurn },  /* Mana Burn */
    { "AEim", &CAbilityImmolation },  /* Immolation */
    { "AEev", &CAbilityEvasion },  /* Evasion */
    { "AEme", &CAbilityMetamorphosis },  /* Metamorphosis */
    { "AEer", &CAbilityEntanglingRoots },  /* Entangling Roots */
    { "AEfn", &CAbilityForceOfNature },  /* Force of Nature */
    { "AEah", &CAbilityAuraSpell },  /* Thorns Aura */
    { "AEtq", &CAbilityTranquility },  /* Tranquility */
    { "AHfa", &CAbilityFlamingArrows },  /* Searing Arrows */
    { "AEar", &CAbilityAuraTrueshot },  /* Trueshot Aura */
    { "AEsf", &CAbilityStarfall },  /* Starfall */
    { "Aren", &CAbilityRepairGeneric },  /* Renew */

    /* OrcAbilityStrings.txt */
    { "AOhw", &CAbilityHealingWave },  /* Healing Wave */
    { "AOhx", &CAbilityHex },  /* Hex */
    { "AOvd", &CAbilityVoodoo },  /* Big Bad Voodoo */
    { "Astd", &CAbilityStandDown },  /* Stand Down */
    { "AOwk", &CAbilityWindWalk },  /* Wind Walk */
    { "AOmi", &CAbilityMirrorImage },  /* Mirror Image */
    { "AOcr", &CAbilityCriticalStrike },  /* Critical Strike */
    { "AOww", &CAbilityWhirlwind },  /* Bladestorm */
    { "AOcl", &CAbilityChainLightning },  /* Chain Lightning */
    { "AOfs", &CAbilityFarSight },  /* Far Sight */
    { "AOsf", &CAbilitySpiritWolf },  /* Feral Spirit */
    { "AOeq", &CAbilityEarthquake },  /* Earthquake */
    { "AOsh", &CAbilityShockwave },  /* Shockwave */
    { "AOae", &CAbilityAuraEndurance },  /* Endurance Aura */
    { "AOre", &CAbilityReincarnation },  /* Reincarnation */
    { "AOws", &CAbilityStomp },  /* War Stomp */

    /* UndeadAbilityStrings.txt */
    { "AUim", &CAbilityImpale },  /* Impale */
    { "AUts", &CAbilityThornyShield },  /* Spiked Carapace */
    { "AUcb", &CAbilityCarrionScarabs },  /* Carrion Beetles */
    { "AUls", &CAbilityLocustSwarm },  /* Locust Swarm */
    { "Aaha", &CAbilityAcolyteHarvest },  /* Gather */
    { "AUdc", &CAbilityDeathCoil },  /* Death Coil */
    { "AUau", &CAbilityAuraUnholy },  /* Unholy Aura */
    { "AUdp", &CAbilityDeathPact },  /* Death Pact */
    { "AUan", &CAbilityAnimateDead },  /* Animate Dead */
    { "AUcs", &CAbilityCarrionSwarm },  /* Carrion Swarm */
    { "AUsl", &CAbilitySleep },  /* Sleep */
    { "AUav", &CAbilityAuraVampiric },  /* Vampiric Aura */
    { "AUfn", &CAbilityFrostNova },  /* Frost Nova */
    { "AUfa", &CAbilityFrostArmor },  /* Frost Armor */
    { "AUfu", &CAbilityFrostArmorAuto },  /* Frost Armor */
    { "AUdr", &CAbilityDarkRitual },  /* Dark Ritual */
    { "AUdd", &CAbilityDeathAndDecay },  /* Death And Decay */
    { "Arst", &CAbilityRepairGeneric },  /* Restore */
    { "AUin", &CAbilityDreadLordInferno },  /* Inferno */

    /* No AbilityStrings source file */
    { "Acoi", &CAbilityCoupleInstant },  /* Couple Instant */
    { "Agl2", &CAbilityGoldMineOverlayed },  /* Gold Mine ability */

    /* END GENERATED ABILITY STRINGS */
    /* BEGIN GENERATED TODO ABILITIES */

    /* CampaignAbilityStrings.txt */
    // TODO: { "Aamk", &a_attack_mod },  /* Attribute Bonus */
    // TODO: { "ANpa", &a_poison_attack },  /* Parasite */
    // TODO: { "ANbr", &CAbilityBash },  /* Battle Roar */

    /* ItemAbilityStrings.txt */
    // TODO: { "AIsp", &a_item_speed },  /* Item Temporary Speed Bonus */
    // TODO: { "AIdm", &a_bounce },  /* Item Area tree/wall damage */
    // TODO: { "AIfl", &a_button },  /* Item Capture The Flag */
    // TODO: { "AIfm", &a_button },  /* Item Capture The Flag */
    // TODO: { "AIfn", &a_button },  /* Item Capture The Flag */
    // TODO: { "AIfo", &a_button },  /* Item Capture The Flag */
    // TODO: { "AIfe", &a_button },  /* Item Capture The Flag */
    // TODO: { "AIha", &a_item_heal_aoe },  /* Item Area Healing */
    // TODO: { "AIvi", &a_unknown },  /* Item Temporary Invisibility */
    // TODO: { "AIvu", &a_item_invul },  /* Item Temporary Invulnerability */
    // TODO: { "AImr", &a_item_mana_restore_aoe },  /* Item Area Mana Regain */
    // TODO: { "AIre", &a_item_restore },  /* Item Heal/Mana Regain */
    // TODO: { "AIra", &a_item_restore_aoe },  /* Item Area Heal/Mana Regain */
    // TODO: { "AIta", &a_item_town_portal },  /* Item Area Detection */
    // TODO: { "AIrm", &a_item_regen_mana },  /* Item Mana Regeneration */
    // TODO: { "AIil", &a_item_illusion },  /* Item Illusions */
    // TODO: { "AIdi", &a_item_dispel_aoe },  /* Item Dispel */
    // TODO: { "AIfb", &CAbilityOnFireHuman },  /* Item Attack Fire Bonus */
    // TODO: { "AIlb", &a_bounce },  /* Item Attack Lightning Bonus */
    // TODO: { "AIlp", &a_lightning_purge },  /* Item Purge */
    // TODO: { "AIob", &CAbilityFrostNova },  /* Item Attack Frost Bonus */
    // TODO: { "AIpb", &a_item_mana_restore },  /* Item Attack Poison Bonus */
    // TODO: { "AIcb", &a_button },  /* Item Attack Corruption Bonus */
    // TODO: { "AIsi", &a_sight_bonus },  /* Item Sight Range Bonus */
    // TODO: { "AIso", &a_simple_spell },  /* Item Soul Theft */
    // TODO: { "Asou", &a_simple_spell },  /* Item Soul Possession */
    // TODO: { "AIrc", &a_item_reincarnation },  /* Item Reincarnation */
    // TODO: { "AIrt", &a_item_recall },  /* Item Recall */
    // TODO: { "AItp", &a_item_town_portal },  /* Item Town Portal */
    // TODO: { "AIpm", &a_button },  /* Item Place Goblin Land Mine */
    // TODO: { "AIaa", &a_damage_bonus_base },  /* Item Permanent Damage Gain */
    // TODO: { "AIva", &a_attack_mod },  /* Item Life Steal */
    // TODO: { "AIcf", &CAbilityImmolation },  /* Item Immolation */
    // TODO: { "AIzb", &CAbilityFrostNova },  /* Item Freeze Damage Bonus */
    // TODO: { "Arel", &a_aura_regen_life },  /* Item Life Regeneration */
    // TODO: { "Aami", &a_unknown },  /* Item Anti-Magic Shell */
    // TODO: { "AIas", &a_unknown },  /* Item Attack Speed Bonus */
    // TODO: { "AIan", &a_simple_spell },  /* Item Animate Dead */
    // TODO: { "AIrs", &a_item_reincarnation },  /* Item Resurrection */
    // TODO: { "AIms", &a_move_speed_bonus },  /* Item Move Speed Bonus */
    // TODO: { "AIgo", &a_attack_mod },  /* Chest of Gold */
    // TODO: { "AIlu", &a_item_heal_aoe },  /* Bundle of Lumber */
    // TODO: { "AIfa", &a_agility_mod },  /* Flare Gun */
    // TODO: { "AIrv", &a_item_heal_aoe },  /* Item Reveal Entire Map */
    // TODO: { "AIdc", &CAbilityItemDefenseAoe },  /* Item Chain Dispel */
    // TODO: { "AIwb", &a_button },  /* Item Web */
    // TODO: { "AImo", &a_item_mana_restore_aoe },  /* Monster Lure */
    // TODO: { "AIri", &a_item_speed },  /* Random Item */
    // TODO: { "Ablp", &CAbilityItemHeal },  /* Blight Placement */
    // TODO: { "Aste", &a_figurine_rock_golem },  /* Steal */
    // TODO: { "AIpv", &a_item_mana_restore_aoe },  /* Vampiric Potion */
    // TODO: { "AIsr", &a_item_speed },  /* Spell Damage Reduction */
    // TODO: { "AIbl", &CAbilityOnFireHuman },  /* Build Tiny Castle */
    // TODO: { "Ashs", &a_spell },  /* Wand of Shadowsight */
    // TODO: { "Aret", &CAbilityResurrection },  /* Tome of Retraining */
    // TODO: { "ANpr", &a_button },  /* Staff of Preservation */
    // TODO: { "Amec", &a_button },  /* Mechanical Critter */
    // TODO: { "ANss", &a_bounce },  /* Spell Shield */
    // TODO: { "ANse", &a_spell },  /* Spell Shield */
    // TODO: { "Aspb", &a_bounce },  /* Spell Book */
    // TODO: { "AIrd", &a_item_dispel_aoe },  /* Raise Dead (Item) */
    // TODO: { "ANsa", &a_bounce },  /* Staff of Sanctuary */
    // TODO: { "AIsa", &a_item_speed },  /* Scroll of Haste */
    // TODO: { "AItb", &a_button },  /* Dust of Appearance */
    // TODO: { "AIsb", &CAbilityItemHeal },  /* Orb of Slow */
    // TODO: { "ANbs", &a_spell },  /* Orb of Darkness */
    // TODO: { "AIrb", &a_item_heal_aoe },  /* Rebirth */
    // TODO: { "AUds", &CAbilityMassTeleport },  /* Dark Summoning */
    // TODO: { "AIdd", &CAbilityItemHeal },  /* Defend */
    // TODO: { "AIsh", &a_item_town_portal },  /* Summon Headhunter */

    /* NeutralAbilityStrings.txt */
    // TODO: { "ANic", &CAbilityInnerFire },  /* Incinerate */
    // TODO: { "ANia", &CAbilityInnerFire },  /* Incinerate */
    // TODO: { "ANso", &a_spell },  /* Soul Burn */
    // TODO: { "ANlm", &a_lightning_shield },  /* Summon Lava Spawn */
    // TODO: { "ANvc", &a_spell },  /* Volcano */
    // TODO: { "ANsy", &a_button },  /* Pocket Factory */
    // TODO: { "ANcs", &a_spell },  /* Cluster Rockets */
    // TODO: { "ANeg", &CAbilityEvasion },  /* Engineering Upgrade */
    // TODO: { "ANrg", &a_regen_base },  /* Robo-Goblin */
    // TODO: { "ANde", &a_button },  /* Demolish */
    // TODO: { "ANfy", &CAbilityOnFireHuman },  /* Factory */
    // TODO: { "ANhs", &CAbilityHeal },  /* Healing Spray */
    // TODO: { "ANcr", &CAbilityCriticalStrike },  /* Chemical Rage */
    // TODO: { "ANtm", &a_aura_regen_life },  /* Transmute */
    // TODO: { "Aasl", &CAbilitySlow },  /* Slow Aura */
    // TODO: { "Atdg", &CAbilityStampede },  /* Building Damage Aura */
    // TODO: { "Atsp", &a_aura },  /* Tornado Spin */
    // TODO: { "Atwa", &CAbilityStomp },  /* Tornado Wander */
    // TODO: { "ANef", &a_button },  /* "Storm, Earth, And Fire" */
    // TODO: { "ACbf", &CAbilityOnFireHuman },  /* Breath of Frost */
    // TODO: { "ANmr", &a_aura },  /* Mind Rot */
    // TODO: { "ANmo", &a_bounce },  /* Monsoon */
    // TODO: { "ANwm", &CAbilityStomp },  /* Watery Minion */
    // TODO: { "Arng", &a_revenge },  /* Revenge */
    // TODO: { "Atol", &a_upgrade },  /* Tree of Life upgrade ability */
    // TODO: { "Awrp", &a_warp },  /* Waygate ability */
    // TODO: { "ANdc", &a_spell },  /* Dark Conversion */
    // TODO: { "ANsl", &a_spell },  /* Soul Preservation */
    // TODO: { "ANfd", &a_spell },  /* Finger of Death */
    // TODO: { "ANdp", &CAbilityMassTeleport },  /* Dark Portal */
    // TODO: { "ANrc", &CAbilityRainOfFire },  /* Rain of Chaos */
    // TODO: { "Achd", &CAbilityCargoHold },  /* Cargo Hold Death */
    // TODO: { "Asla", &a_sleep_always },  /* Sleep Always; UnitCanSleepPerm recognizes ownership, behavior remains unresolved */
    // TODO: { "Advc", &a_devour_cargo },  /* Devour Cargo */
    // TODO: { "ANpi", &a_perm_immolation },  /* Permanent Immolation */
    // TODO: { "Apig", &a_harvest_return },  /* Permanent Immolation */
    // TODO: { "AAns", &a_neutral_spell },  /* Charge Gold and Lumber */
    // TODO: { "Andt", &a_evil_eye },  /* Reveal */
    // TODO: { "ANin", &a_creep_thunder_bolt },  /* Inferno */
    // TODO: { "Asds", &a_button },  /* Kaboom! */
    // TODO: { "Anhe", &CAbilityHeal },  /* Heal */
    // TODO: { "ACtc", &a_creep_thunder_clap },  /* Slam */
    // TODO: { "ACtb", &a_creep_thunder_bolt },  /* Hurl Boulder */
    // TODO: { "Afzy", &a_spell },  /* Frenzy */
    // TODO: { "ACdv", &a_creep_devour },  /* Devour */
    // TODO: { "ACsp", &a_creep_sleep },  /* Creep Sleep ability details; natural canSleep/night behavior lives in g_creep_sleep.c */
    // TODO: { "Asod", &a_simple_spell },  /* Spawn Skeleton */
    // TODO: { "Assp", &a_simple_spell },  /* Spawn Spiderlings */
    // TODO: { "Aspd", &a_simple_spell },  /* Spawn Spiders */
    // TODO: { "AOac", &a_unknown },  /* Command Aura */
    // TODO: { "ACad", &a_revenge },  /* Animate Dead */
    // TODO: { "ACrn", &a_creep_reincarnation },  /* Reincarnation */
    // TODO: { "Adda", &CAbilityOnFireHuman },  /* AOE damage upon death */
    // TODO: { "Agho", &a_ghost },  /* Ghost */
    // TODO: { "Aeth", &a_ghost },  /* Ghost */
    // TODO: { "Amin", &a_button },  /* Mine - exploding */
    // TODO: { "Apiv", &a_perm_invis },  /* Permanent Invisibility */
    // TODO: { "Awan", &a_wander },  /* Wander */
    // TODO: { "Aarm", &a_unknown },  /* Mana Regeneration Aura */
    // TODO: { "Asid", &a_button },  /* Sell Items */
    // TODO: { "Asud", &a_spell },  /* Sell Units */

    /* NightElfAbilityStrings.txt */
    // TODO: { "Avng", &a_revenge },  /* Spirit of Vengeance */
    // TODO: { "Amfl", &CAbilityFrostNova },  /* Mana Flare */
    // TODO: { "Apsh", &CAbilityStampede },  /* Phase Shift */
    // TODO: { "Aetl", &a_button },  /* Ethereal */
    // TODO: { "Agra", &a_poison_attack },  /* War Club */
    // TODO: { "Assk", &a_button },  /* Hardened Skin */
    // TODO: { "Arsk", &a_morph },  /* Resistant Skin */
    // TODO: { "Atau", &a_spell },  /* Taunt */
    // TODO: { "Amgl", &a_bounce },  /* Moon Glaive */
    // TODO: { "Aspo", &a_poison_attack },  /* Slow Poison */
    // TODO: { "Ashm", &a_shadow_meld },  /* Shadow Meld */
    // TODO: { "Ahid", &a_shadow_meld },  /* Shadow Meld */
    // TODO: { "Aesn", &a_button },  /* Sentinel */
    // TODO: { "Adtn", &a_button },  /* Detonate */
    // TODO: { "Abrf", &a_morph },  /* Bear Form */

    // TODO: { "Aadm", &a_auto_dispel_magic },  /* Abolish Magic */
    // TODO: { "Amim", &a_magic_immunity },  /* Spell Immunity */
    // TODO: { "Ault", &a_night_vision },  /* Ultravision */
    // TODO: { "Acoa", &a_morph },  /* Mount Hippogryph */
    // TODO: { "Acoh", &a_morph },  /* Pick up Archer */
    // TODO: { "Adec", &CAbilityDefend },  /* Dismount */
    // TODO: { "Acor", &a_bounce },  /* Corrosive Breath */
    // TODO: { "AEst", &a_button },  /* Scout */
    // TODO: { "Afae", &a_auto_target_spell },  /* Faerie Fire */
    // TODO: { "Acyc", &a_morph },  /* Cyclone */
    // TODO: { "Arej", &a_regen_life },  /* Rejuvenation */
    // TODO: { "Aroa", &a_roar },  /* Roar */
    // TODO: { "Alit", &a_lightning_attack },  /* Lightning Attack */

    /* OrcAbilityStrings.txt */
    // TODO: { "Abof", &a_aura_command },  /* Burning Oil */
    // TODO: { "Absk", &a_button },  /* Berserk */
    // TODO: { "Arbr", &a_button },  /* Reinforced Burrows Upgrade */
    // TODO: { "Aast", &a_aura },  /* Ancestral Spirit */
    // TODO: { "Adch", &a_spell },  /* Disenchant */
    // TODO: { "Acpf", &a_purge },  /* Corporeal Form */
    // TODO: { "Aetf", &a_morph },  /* Ethereal Form */
    // TODO: { "Aspl", &a_creep_sleep },  /* Spirit Link */
    // TODO: { "Aliq", &CAbilityOnFireHuman },  /* Liquid Fire */
    // TODO: { "Auco", &a_spell },  /* Unstable Concoction */
    // TODO: { "Acha", &a_unknown },  /* Chaos */
    // TODO: { "Achl", &a_cargo_load },  /* Chaos Cargo Load */
    // TODO: { "Awar", &a_bounce },  /* Pulverize */
    { "Abtl", &CAbilityBattlestations },  /* Battle Stations */
    // TODO: { "Aens", &CAbilityEnsnare },  /* Ensnare */
    // TODO: { "Adev", &a_devour },  /* Devour */
    // TODO: { "Aprg", &a_lightning_purge },  /* Purge */
    // TODO: { "Alsh", &a_lightning_shield },  /* Lightning Shield */
    // TODO: { "Ablo", &a_bloodlust },  /* Bloodlust */
    // TODO: { "Aeye", &a_button },  /* Sentry Ward */
    // TODO: { "Asta", &a_stasis_trap },  /* Stasis Trap */
    // TODO: { "Ahwd", &a_healing_ward },  /* Healing Ward */
    // TODO: { "Aoar", &a_aura_regen_life },  /* Healing Ward Aura */
    // TODO: { "Aven", &a_venom_spear },  /* Envenomed Spears */
    // TODO: { "Apoi", &a_poison_attack },  /* Poison Sting */
    // TODO: { "Apo2", &a_item_invul },  /* Poison Sting */
    // TODO: { "Aspi", &a_spiked },  /* Spiked Barricades */
    // TODO: { "Asal", &a_button },  /* Pillage */
    // TODO: { "Aakb", &a_aura_command },  /* War Drums */

    /* UndeadAbilityStrings.txt */
    // TODO: { "Arpb", &a_spell },  /* Replenish */
    // TODO: { "Arpl", &a_spell },  /* Essence of Blight */
    // TODO: { "Arpm", &a_spell },  /* Spirit Touch */
    // TODO: { "Aexh", &a_button },  /* Exhume Corpses */
    // TODO: { "Aave", &CAbilityStampede },  /* Destroyer Form */
    // TODO: { "Afak", &a_spell },  /* Orb of Annihilation */
    // TODO: { "Advm", &CAbilityDispelMagic },  /* Devour Magic */
    // TODO: { "Aabr", &a_aura_regen_life },  /* Aura of Blight */
    // TODO: { "Aabs", &a_aura },  /* Absorb Mana */
    // TODO: { "Abur", &a_creep_sleep },  /* Burrow */
    // TODO: { "Amtc", &a_unknown },  /* Cargo Hold */
    // TODO: { "Atru", &a_true_sight },  /* True Sight */
    // TODO: { "Auns", &a_button },  /* Unsummon Building */
    // TODO: { "Agyd", &a_simple_spell },  /* Create Corpse */
    // TODO: { "Alam", &a_spell },  /* Sacrifice */
    // TODO: { "Asac", &a_spell },  /* Sacrifice */
    // TODO: { "Acan", &a_cannibalize },  /* Cannibalize */
    // TODO: { "Aspa", &CAbilityAttack },  /* Spider Attack */
    // TODO: { "Aweb", &a_auto_target_spell },  /* Web */
    // TODO: { "Astn", &a_morph },  /* Stone Form */
    // TODO: { "Amel", &a_cargo_load },  /* Get Corpse */
    // TODO: { "Amed", &a_cargo_drop },  /* Drop Corpse */
    // TODO: { "Aapl", &a_unknown },  /* Disease Cloud */
    // TODO: { "Apts", &a_button },  /* Disease Cloud */
    // TODO: { "Afrb", &a_button },  /* Frost Breath */
    // TODO: { "Afra", &a_button },  /* Frost Attack */
    // TODO: { "Afrz", &CAbilityFrostNova },  /* Freezing Breath */
    // TODO: { "Arai", &a_simple_spell },  /* Raise Dead */
    // TODO: { "Auhf", &a_creep_sleep },  /* Unholy Frenzy */
    // TODO: { "Acrs", &a_auto_target_spell },  /* Curse */
    // TODO: { "Aams", &a_magic_immunity },  /* Anti-magic Shell */
    // TODO: { "Apos", &a_creep_sleep },  /* Possession */
    // TODO: { "Aps2", &a_creep_sleep },  /* Possession */
    // TODO: { "Acri", &a_cripple },  /* Cripple */

    /* No AbilityStrings source file */
    // TODO: { "AIgl", &a_unknown },  /* FortificationGlyph — CAbility [ITEM] other */
    // TODO: { "AIrg", &a_unknown },  /* Potion of Life Regen — CAbility [ITEM] other */
    // TODO: { "ANsu", &a_unknown },  /* Submerge (Myrmidon) — CAbility creeps */
    // TODO: { "AOwd", &a_unknown },  /* Shadow Hunter - Serpent Ward */
    // TODO: { "Aimp", &a_bounce },  /* Impaling Bolt — CAbilityBounce nightelf */
    // TODO: { "Ansp", &a_neutral_spell },  /* Neutral Spies — CAbilityNeutralSpell creeps */

    /* AbilityData rows without a generated AbilityStrings entry. */
    // TODO: { "AAns", &a_unknown },  /* Neutral Spell */
    // TODO: { "ACac", &a_unknown },  /* Aura - Command (Creep) */
    // TODO: { "ACad", &a_unknown },  /* Animate Dead (creep) */
    // TODO: { "ACah", &a_unknown },  /* Thorns Aura (creep) */
    // TODO: { "ACam", &a_unknown },  /* Anti-magic Shield (creep) */
    // TODO: { "ACat", &a_unknown },  /* Aura - Trueshot (Creep) */
    // TODO: { "ACav", &a_unknown },  /* Aura - Devotion (Creep) */
    // TODO: { "ACba", &a_unknown },  /* Aura - Brilliance (creep) */
    // TODO: { "ACbb", &a_unknown },  /* Bloodlust (creep, Hotkey B) */
    // TODO: { "ACbc", &a_unknown },  /* Breath of Fire(Creep) */
    // TODO: { "ACbf", &a_unknown },  /* Breath of Frost(Creep) */
    // TODO: { "ACbh", &a_unknown },  /* Bash (creep) */
    // TODO: { "ACbk", &a_unknown },  /* Black Arrow (melee, creep) */
    // TODO: { "ACbl", &a_unknown },  /* Bloodlust (Creep) */
    // TODO: { "ACbn", &a_unknown },  /* Banish(Creep) */
    // TODO: { "ACbz", &a_unknown },  /* Blizzard (creep) */
    // TODO: { "ACc2", &a_unknown },  /* Crushing Wave (Dragon Turtle) */
    // TODO: { "ACc3", &a_unknown },  /* Crushing Wave (Lesser) */
    // TODO: { "ACca", &a_unknown },  /* Carrion Swarm (creep) */
    // TODO: { "ACcb", &a_unknown },  /* Frost Bolt */
    // TODO: { "ACce", &a_unknown },  /* Cleaving Attack (Creep) */
    // TODO: { "ACch", &a_unknown },  /* Charm */
    // TODO: { "ACcl", &a_unknown },  /* Chain Lightning (creep) */
    // TODO: { "ACcn", &a_unknown },  /* Cannibalize (creep) */
    // TODO: { "ACcr", &a_unknown },  /* Cripple (creep) */
    // TODO: { "ACcs", &a_unknown },  /* Curse (creep) */
    // TODO: { "ACct", &a_unknown },  /* Critical Strike (creep) */
    // TODO: { "ACcv", &a_unknown },  /* Crushing Wave */
    // TODO: { "ACcw", &a_unknown },  /* Cold Arrows (creep) */
    // TODO: { "ACcy", &a_unknown },  /* Cyclone (creep) */
    // TODO: { "ACd2", &a_unknown },  /* Abolish Magic (Creep, 1,2 pos) */
    // TODO: { "ACdc", &a_unknown },  /* Death Coil (creep) */
    // TODO: { "ACde", &a_unknown },  /* Devour Magic(creep) */
    // TODO: { "ACdm", &a_unknown },  /* Abolish Magic (Creep) */
    // TODO: { "ACdr", &a_unknown },  /* Drain Life(Creep) */
    // TODO: { "ACds", &a_unknown },  /* Divine Shield (creep) */
    // TODO: { "ACdv", &a_unknown },  /* Devour (Dragon Creep) */
    // TODO: { "ACen", &a_unknown },  /* Ensnare (Creep) */
    // TODO: { "ACes", &a_unknown },  /* Evasion (creep 100%) */
    // TODO: { "ACev", &a_unknown },  /* Evasion (creep) */
    // TODO: { "ACf2", &a_unknown },  /* Frost Armor (creep,autocast) */
    // TODO: { "ACf3", &a_unknown },  /* Finger of Pain (2,1 Button) */
    // TODO: { "ACfa", &a_unknown },  /* Frost Armor (creep,old) */
    // TODO: { "ACfb", &a_unknown },  /* Fire Bolt (creep) */
    // TODO: { "ACfd", &a_unknown },  /* Finger of Pain */
    // TODO: { "ACff", &a_unknown },  /* Faerie Fire (creep) */
    // TODO: { "ACfl", &a_unknown },  /* Forked Lightning(creep) */
    // TODO: { "ACfn", &a_unknown },  /* Frost Nova (creep) */
    // TODO: { "ACfr", &a_unknown },  /* Force of Nature (creep) */
    // TODO: { "ACfs", &a_unknown },  /* Flame Strike (Creep) */
    // TODO: { "AChv", &a_unknown },  /* Healing Wave(Creep) */
    // TODO: { "AChw", &a_unknown },  /* Healing Ward (creep) */
    // TODO: { "AChx", &a_unknown },  /* Hex (Creep) */
    // TODO: { "ACif", &a_unknown },  /* Inner Fire (Creep) */
    // TODO: { "ACim", &a_unknown },  /* Immolation (creep) */
    // TODO: { "ACls", &a_unknown },  /* Lightning Shield (creep) */
    // TODO: { "ACm2", &a_unknown },  /* Magic Immunity (Archimonde) */
    // TODO: { "ACm3", &a_unknown },  /* Magic Immunity (Dragons) */
    // TODO: { "ACmf", &a_unknown },  /* Mana Shield(Creep) */
    // TODO: { "ACmi", &a_unknown },  /* Magic Immunity (Creep) */
    // TODO: { "ACmo", &a_unknown },  /* Monsoon(creep) */
    // TODO: { "ACmp", &a_unknown },  /* Impale(Creep) */
    // TODO: { "ACnr", &a_unknown },  /* Neutral Regen (health only) */
    // TODO: { "ACpa", &a_unknown },  /* Parasite(eredar) */
    // TODO: { "ACps", &a_unknown },  /* Possession (creep) */
    // TODO: { "ACpu", &a_unknown },  /* Purge (Creep) */
    // TODO: { "ACpv", &a_unknown },  /* Pulverize (Sea Giant) */
    // TODO: { "ACpy", &a_unknown },  /* Polymorph (creep) */
    // TODO: { "ACr1", &a_unknown },  /* Roar (creep) -- Skeletal Orc */
    // TODO: { "ACr2", &a_unknown },  /* Rejuvination (Furbolg) */
    // TODO: { "ACrd", &a_unknown },  /* Raise Dead (Creep) */
    // TODO: { "ACrf", &a_unknown },  /* Rain of Fire (creep) */
    // TODO: { "ACrg", &a_unknown },  /* Rain of Fire (creep,greater) */
    // TODO: { "ACrj", &a_unknown },  /* Rejuvination (creep) */
    // TODO: { "ACrk", &a_unknown },  /* Resistant Skin (creep) */
    // TODO: { "ACrn", &a_unknown },  /* Reincarnation (creep) */
    // TODO: { "ACro", &a_unknown },  /* Roar (creep) */
    // TODO: { "ACs9", &a_unknown },  /* Feral Spirit (creep - pig) */
    // TODO: { "ACsa", &a_unknown },  /* Searing Arrows (creep) */
    // TODO: { "ACsf", &a_unknown },  /* Feral Spirit (creep) */
    // TODO: { "ACsh", &a_unknown },  /* Shockwave (Creep) */
    // TODO: { "ACsi", &a_unknown },  /* Silence(Creep) */
    // TODO: { "ACsk", &a_unknown },  /* Resistant Skin(3,1 pos, creep) */
    // TODO: { "ACsl", &a_unknown },  /* Sleep (creep) */
    // TODO: { "ACsm", &a_unknown },  /* Siphon Mana (Creep) */
    // TODO: { "ACsp", &a_unknown },  /* Creep Sleep */
    // TODO: { "ACss", &a_unknown },  /* Shadow Strike(Creep) */
    // TODO: { "ACst", &a_unknown },  /* Shockwave (Trap) */
    // TODO: { "ACsw", &a_unknown },  /* Slow (Creep) */
    // TODO: { "ACt2", &a_unknown },  /* Thunder Clap (Thunder Lizard) */
    // TODO: { "ACtb", &a_unknown },  /* Thunder Bolt (Creep) */
    // TODO: { "ACtc", &a_unknown },  /* Thunder Clap (Creep) */
    // TODO: { "ACua", &a_unknown },  /* Unholy Aura (creep) */
    // TODO: { "ACuf", &a_unknown },  /* Unholy Frenzy (creep) */
    // TODO: { "ACvp", &a_unknown },  /* Vampiric Aura (creep) */
    // TODO: { "ACvs", &a_unknown },  /* Venom Spears (Creep) */
    // TODO: { "ACwb", &a_unknown },  /* Web (creep) */
    // TODO: { "ACwe", &a_unknown },  /* Summon Sea Elemental */
    // TODO: { "AEIl", &a_unknown },  /* Illidan - Metamorphosis */
    // TODO: { "AEsb", &a_unknown },  /* Cenarius - Beefy Starfall */
    // TODO: { "AEst", &a_unknown },  /* Priestess - Scout */
    // TODO: { "AEvi", &a_unknown },  /* Evil Illidan - Metamorphosis */
    // TODO: { "AHta", &a_unknown },  /* Reveal(Arcane Tower) */
    // TODO: { "AI2m", &a_unknown },  /* 200 mana bonus */
    // TODO: { "AIa1", &a_unknown },  /* AgilityBonus (+1) */
    // TODO: { "AIa3", &a_unknown },  /* AgilityBonus (+3) */
    // TODO: { "AIa4", &a_unknown },  /* AgilityBonus (+4) */
    // TODO: { "AIa6", &a_unknown },  /* AgilityBonus (+6) */
    // TODO: { "AIaa", &a_unknown },  /* AttackMod */
    // TODO: { "AIad", &a_unknown },  /* ItemAuraDevotion */
    // TODO: { "AIae", &a_unknown },  /* ItemAuraEndurance */
    // TODO: { "AIan", &a_unknown },  /* Animate Dead */
    // TODO: { "AIar", &a_unknown },  /* ItemAuraTrueshot */
    // TODO: { "AIau", &a_unknown },  /* ItemAuraUnholy */
    // TODO: { "AIav", &a_unknown },  /* ItemAuraVampiric */
    // TODO: { "AIaz", &a_unknown },  /* AgilityBonus (+10) */
    // TODO: { "AIba", &a_unknown },  /* ItemAuraBrilliance */
    // TODO: { "AIbb", &a_unknown },  /* Build Tiny Blacksmith */
    // TODO: { "AIbf", &a_unknown },  /* Build Tiny Farm */
    // TODO: { "AIbg", &a_unknown },  /* Build Tiny Great Hall */
    // TODO: { "AIbh", &a_unknown },  /* Build Tiny Altar */
    // TODO: { "AIbk", &a_unknown },  /* Blink (Item) */
    // TODO: { "AIbl", &a_unknown },  /* Build Tiny Castle */
    // TODO: { "AIbm", &a_unknown },  /* MaxManaBonus (Most) */
    // TODO: { "AIbr", &a_unknown },  /* Build Tiny Lumber Mill */
    // TODO: { "AIbs", &a_unknown },  /* Build Tiny Barracks */
    // TODO: { "AIbt", &a_unknown },  /* Build Tiny Scout Tower */
    // TODO: { "AIbx", &a_unknown },  /* Bash (item) */
    // TODO: { "AIcb", &a_unknown },  /* Orb of Corruption */
    // TODO: { "AIcd", &a_unknown },  /* ItemAuraCommand */
    // TODO: { "AIcf", &a_unknown },  /* ItemCloakOfFlames */
    // TODO: { "AIcl", &a_unknown },  /* Chain Lightning (item) */
    // TODO: { "AIcm", &a_unknown },  /* Control Magic (item) */
    // TODO: { "AIcs", &a_unknown },  /* Critical Strike (item) */
    // TODO: { "AIcy", &a_unknown },  /* Cyclone */
    // TODO: { "AId0", &a_unknown },  /* DefenseBonus (+10) */
    // TODO: { "AId1", &a_unknown },  /* DefenseBonus (+1) */
    // TODO: { "AId2", &a_unknown },  /* DefenseBonus (+2) */
    // TODO: { "AId3", &a_unknown },  /* DefenseBonus (+3) */
    // TODO: { "AId4", &a_unknown },  /* DefenseBonus (+4) */
    // TODO: { "AId5", &a_unknown },  /* DefenseBonus (+5) */
    // TODO: { "AId7", &a_unknown },  /* DefenseBonus (+7) */
    // TODO: { "AId8", &a_unknown },  /* DefenseBonus (+8) */
    // TODO: { "AIdb", &a_unknown },  /* ItemDefenseAoe (+ Healing) */
    // TODO: { "AIdc", &a_unknown },  /* ItemDispelChain */
    // TODO: { "AIdd", &a_unknown },  /* Defend (Item) */
    // TODO: { "AIdf", &a_unknown },  /* Orb of Darkness */
    // TODO: { "AIdi", &a_unknown },  /* ItemDispelAoe */
    // TODO: { "AIdm", &a_unknown },  /* ItemDamageAoe */
    // TODO: { "AIdn", &a_unknown },  /* Shadow Orb Ability */
    // TODO: { "AIdp", &a_unknown },  /* Death Pact (item) */
    // TODO: { "AIds", &a_unknown },  /* ItemDispelAoeWithCooldown */
    // TODO: { "AIdv", &a_unknown },  /* Divine Shield (Item) */
    // TODO: { "AIe2", &a_unknown },  /* ExperienceMod greater */
    // TODO: { "AIev", &a_unknown },  /* Evasion */
    // TODO: { "AIfa", &a_unknown },  /* FlareGun */
    // TODO: { "AIfb", &a_unknown },  /* Orb of Fire */
    // TODO: { "AIfd", &a_unknown },  /* FigurineRedDrake */
    // TODO: { "AIfe", &a_unknown },  /* Flag (Undead) */
    // TODO: { "AIff", &a_unknown },  /* FigurineFurbolg */
    // TODO: { "AIfg", &a_unknown },  /* Cloud of Fog (Item) */
    // TODO: { "AIfh", &a_unknown },  /* FigurineFelHound */
    // TODO: { "AIfl", &a_unknown },  /* Flag */
    // TODO: { "AIfm", &a_unknown },  /* Flag (Human) */
    // TODO: { "AIfn", &a_unknown },  /* Flag (Night Elf) */
    // TODO: { "AIfo", &a_unknown },  /* Flag (Orc) */
    // TODO: { "AIfr", &a_unknown },  /* FigurineRockGolem */
    // TODO: { "AIft", &a_unknown },  /* Frostguard - frost melee */
    // TODO: { "AIfu", &a_unknown },  /* FigurineDoomGuard */
    // TODO: { "AIfw", &a_unknown },  /* Searing Blade - fire melee */
    // TODO: { "AIfx", &a_unknown },  /* Flag (Orc Battle Standard) */
    // TODO: { "AIfz", &a_unknown },  /* Finger of Death (item) */
    // TODO: { "AIgd", &a_unknown },  /* Orb of Guldan */
    // TODO: { "AIgf", &a_unknown },  /* FortificationGlyph */
    // TODO: { "AIgm", &a_unknown },  /* AgilityMod +2 */
    // TODO: { "AIgo", &a_unknown },  /* GiveGold */
    // TODO: { "AIgu", &a_unknown },  /* UltraVisionGlyph */
    // TODO: { "AIgx", &a_unknown },  /* Aura - Regeneration (item) */
    // TODO: { "AIh1", &a_unknown },  /* ItemHeal (Lesser) */
    // TODO: { "AIh2", &a_unknown },  /* ItemHeal (Greater) */
    // TODO: { "AIh3", &a_unknown },  /* ItemHeal (Least) */
    // TODO: { "AIha", &a_unknown },  /* ItemHealAoe */
    // TODO: { "AIhb", &a_unknown },  /* ItemHealAoeGreater */
    // TODO: { "AIhl", &a_unknown },  /* Holy Light (item) */
    // TODO: { "AIhw", &a_unknown },  /* Healing Ward */
    // TODO: { "AIhx", &a_unknown },  /* ItemHeal (Leastest) */
    // TODO: { "AIi1", &a_unknown },  /* IntelligenceBonus (+1) */
    // TODO: { "AIi3", &a_unknown },  /* IntelligenceBonus (+3) */
    // TODO: { "AIi4", &a_unknown },  /* IntelligenceBonus (+4) */
    // TODO: { "AIi6", &a_unknown },  /* IntelligenceBonus (+6) */
    // TODO: { "AIil", &a_unknown },  /* ItemIllusion */
    // TODO: { "AIin", &a_unknown },  /* ItemInferno */
    // TODO: { "AIir", &a_unknown },  /* FigurineIceRevenant */
    // TODO: { "AIl1", &a_unknown },  /* MaxLifeBonus (Lesser) */
    // TODO: { "AIl2", &a_unknown },  /* MaxLifeBonus (Greater) */
    // TODO: { "AIlb", &a_unknown },  /* Orb of Lightning(old) */
    // TODO: { "AIlf", &a_unknown },  /* MaxLifeBonus (Least) */
    // TODO: { "AIll", &a_unknown },  /* Orb of Lightning */
    // TODO: { "AIlp", &a_unknown },  /* LightningPurge */
    // TODO: { "AIls", &a_unknown },  /* Lightning Shield */
    // TODO: { "AIlu", &a_unknown },  /* GiveLumber */
    // TODO: { "AIlx", &a_unknown },  /* Shaman Claws - lightning melee */
    // TODO: { "AIlz", &a_unknown },  /* MaxLifeBonus (Leastest) */
    // TODO: { "AIm1", &a_unknown },  /* ItemManaRestore (Lesser) */
    // TODO: { "AIm2", &a_unknown },  /* ItemManaRestore (Greater) */
    // TODO: { "AImb", &a_unknown },  /* MaxManaBonus (Least) */
    // TODO: { "AImh", &a_unknown },  /* Permanent Hit point Bonus */
    // TODO: { "AImo", &a_unknown },  /* ItemMonsterLure */
    // TODO: { "AImr", &a_unknown },  /* ItemManaRestoreAoe */
    // TODO: { "AIms", &a_unknown },  /* MoveSpeedBonus */
    // TODO: { "AImt", &a_unknown },  /* Staff o' Teleportation */
    // TODO: { "AImv", &a_unknown },  /* MaxManaBonus (Leastest, Really) */
    // TODO: { "AImx", &a_unknown },  /* Magic Immunity */
    // TODO: { "AImz", &a_unknown },  /* MaxManaBonus (Leastest) */
    // TODO: { "AInd", &a_unknown },  /* Animate Dead (item, special) */
    // TODO: { "AInm", &a_unknown },  /* StrengthMod +2 */
    // TODO: { "AIob", &a_unknown },  /* Orb of Frost */
    // TODO: { "AIos", &a_unknown },  /* Slow */
    // TODO: { "AIp1", &a_unknown },  /* Potion of Rejuv I */
    // TODO: { "AIp2", &a_unknown },  /* Potion of Rejuv II */
    // TODO: { "AIp3", &a_unknown },  /* Potion of Rejuv III */
    // TODO: { "AIp4", &a_unknown },  /* Potion of Rejuv IV */
    // TODO: { "AIp5", &a_unknown },  /* Scroll of Rejuv I */
    // TODO: { "AIp6", &a_unknown },  /* Scroll of Rejuv II */
    // TODO: { "AIpb", &a_unknown },  /* Orb of Venom */
    // TODO: { "AIpg", &a_unknown },  /* Purge(orb) */
    // TODO: { "AIpl", &a_unknown },  /* Potion of Mana Regen(lesser) */
    // TODO: { "AIpm", &a_unknown },  /* ItemPlaceMine */
    // TODO: { "AIpr", &a_unknown },  /* Potion of Mana Regen(greater) */
    // TODO: { "AIps", &a_unknown },  /* Purge(Totem, SP) */
    // TODO: { "AIpv", &a_unknown },  /* ItemPotionVampirism */
    // TODO: { "AIpx", &a_unknown },  /* Permanent Hit point Bonus (small) */
    // TODO: { "AIpz", &a_unknown },  /* Penguin Squeek */
    // TODO: { "AIra", &a_unknown },  /* ItemRestoreAoe */
    // TODO: { "AIrb", &a_unknown },  /* Rune of Rebirth */
    // TODO: { "AIrc", &a_unknown },  /* ItemReincarnation */
    // TODO: { "AIrd", &a_unknown },  /* Raise Dead (Item) */
    // TODO: { "AIre", &a_unknown },  /* ItemRestore */
    // TODO: { "AIri", &a_unknown },  /* ItemRandomItem */
    // TODO: { "AIrl", &a_unknown },  /* Potion of Life Regen */
    // TODO: { "AIrm", &a_unknown },  /* ItemRegenMana */
    // TODO: { "AIrn", &a_unknown },  /* ItemRegenMana lesser */
    // TODO: { "AIrr", &a_unknown },  /* Roar */
    // TODO: { "AIrs", &a_unknown },  /* Resurrection */
    // TODO: { "AIrt", &a_unknown },  /* ItemRecall */
    // TODO: { "AIrv", &a_unknown },  /* ItemRevealMap */
    // TODO: { "AIrx", &a_unknown },  /* Resurrection - Item */
    // TODO: { "AIs1", &a_unknown },  /* StrengthBonus (+1) */
    // TODO: { "AIs2", &a_unknown },  /* Attack Speed Increase(greater) */
    // TODO: { "AIs3", &a_unknown },  /* StrengthBonus (+3) */
    // TODO: { "AIs4", &a_unknown },  /* StrengthBonus (+4) */
    // TODO: { "AIs6", &a_unknown },  /* StrengthBonus (+6) */
    // TODO: { "AIsa", &a_unknown },  /* ItemSpeedAoe */
    // TODO: { "AIsb", &a_unknown },  /* Orb of Spells */
    // TODO: { "AIse", &a_unknown },  /* Silence(Item) */
    // TODO: { "AIsh", &a_unknown },  /* Summon Headhunter (item) */
    // TODO: { "AIsi", &a_unknown },  /* SightBonus */
    // TODO: { "AIsl", &a_unknown },  /* Scroll of Life Regen */
    // TODO: { "AIso", &a_unknown },  /* SoulTrap */
    // TODO: { "AIsp", &a_unknown },  /* ItemSpeed */
    // TODO: { "AIsr", &a_unknown },  /* Runed Bracers */
    // TODO: { "AIsw", &a_unknown },  /* Sentry Ward */
    // TODO: { "AIsx", &a_unknown },  /* Attack Speed Increase */
    // TODO: { "AIsz", &a_unknown },  /* Slow Poison (item) */
    // TODO: { "AIt6", &a_unknown },  /* AttackBonus */
    // TODO: { "AIt9", &a_unknown },  /* AttackBonus */
    // TODO: { "AIta", &a_unknown },  /* ItemDetectAoe */
    // TODO: { "AItb", &a_unknown },  /* Dust of Appearance */
    // TODO: { "AItc", &a_unknown },  /* AttackBonus */
    // TODO: { "AItf", &a_unknown },  /* AttackBonus */
    // TODO: { "AItg", &a_unknown },  /* AttackBonus +1 */
    // TODO: { "AIth", &a_unknown },  /* AttackBonus +2 */
    // TODO: { "AIti", &a_unknown },  /* AttackBonus +4 */
    // TODO: { "AItj", &a_unknown },  /* AttackBonus +5 */
    // TODO: { "AItk", &a_unknown },  /* AttackBonus +7 */
    // TODO: { "AItl", &a_unknown },  /* AttackBonus +8 */
    // TODO: { "AItm", &a_unknown },  /* IntelligenceMod +2 */
    // TODO: { "AItn", &a_unknown },  /* AttackBonus +10 */
    // TODO: { "AItp", &a_unknown },  /* ItemTownPortal */
    // TODO: { "AItx", &a_unknown },  /* AttackBonus +20 */
    // TODO: { "AIuf", &a_unknown },  /* Unholy Frenzy (item) */
    // TODO: { "AIuv", &a_unknown },  /* ItemUltravision */
    // TODO: { "AIuw", &a_unknown },  /* FigurineUrsaWarrior */
    // TODO: { "AIv1", &a_unknown },  /* ItemInvis (Lesser) */
    // TODO: { "AIv2", &a_unknown },  /* ItemInvis (Greater) */
    // TODO: { "AIva", &a_unknown },  /* Vampiric attack */
    // TODO: { "AIvl", &a_unknown },  /* ItemInvul */
    // TODO: { "AIvu", &a_unknown },  /* ItemInvul */
    // TODO: { "AIwb", &a_unknown },  /* ItemWeb */
    // TODO: { "AIwm", &a_unknown },  /* Watery Minion (item) */
    // TODO: { "AIx1", &a_unknown },  /* (All + 1) */
    // TODO: { "AIx2", &a_unknown },  /* (All + 2) */
    // TODO: { "AIx3", &a_unknown },  /* (All + 3) */
    // TODO: { "AIx4", &a_unknown },  /* (All + 4) */
    // TODO: { "AIx5", &a_unknown },  /* Crown of Kings (All + 5) */
    // TODO: { "AIxk", &a_unknown },  /* Beserk (item) */
    // TODO: { "AIxs", &a_unknown },  /* Anti-magic Shield */
    // TODO: { "AIzb", &a_unknown },  /* Orb of Freezing */
    // TODO: { "ANak", &a_unknown },  /* Orb of Annihilation (Quill Spray) */
    // TODO: { "ANb2", &a_unknown },  /* Bash (maul , SP Bear, level 3) */
    // TODO: { "ANbh", &a_unknown },  /* Bash (Beastmaster Bear) */
    // TODO: { "ANbl", &a_unknown },  /* Blink(Beastmaster Bear) */
    // TODO: { "ANbs", &a_unknown },  /* Orb of Darkness (Black Arrow) */
    // TODO: { "ANc1", &a_unknown },  /* Tinkerer - Cluster Rockets (Level 1) */
    // TODO: { "ANc2", &a_unknown },  /* Tinkerer - Cluster Rockets (Level 2) */
    // TODO: { "ANc3", &a_unknown },  /* Tinkerer - Cluster Rockets (Level 3) */
    // TODO: { "ANcr", &a_unknown },  /* Alchemist - Chemical Rage */
    // TODO: { "ANcs", &a_unknown },  /* Tinkerer - Cluster Rockets (Level 0) */
    // TODO: { "ANd1", &a_unknown },  /* Tinkerer - Demolish (Level 1) */
    // TODO: { "ANd2", &a_unknown },  /* Tinkerer - Demolish (Level 2) */
    // TODO: { "ANd3", &a_unknown },  /* Tinkerer - Demolish (Level 3) */
    // TODO: { "ANdc", &a_unknown },  /* Malganis - Dark Conversion */
    // TODO: { "ANde", &a_unknown },  /* Tinkerer - Demolish (Level 0) */
    // TODO: { "ANdp", &a_unknown },  /* Dark Portal */
    // TODO: { "ANef", &a_unknown },  /* Brewmaster - Storm, Earth and Fire */
    // TODO: { "ANeg", &a_unknown },  /* Tinkerer - Engineering Upgrade */
    // TODO: { "ANfa", &a_unknown },  /* Sea Witch - Frost Arrows */
    // TODO: { "ANfd", &a_unknown },  /* Finger of Death */
    // TODO: { "ANfy", &a_unknown },  /* Factory */
    // TODO: { "ANg1", &a_unknown },  /* Tinkerer - Robo-Goblin (Level 1) */
    // TODO: { "ANg2", &a_unknown },  /* Tinkerer - Robo-Goblin (Level 2) */
    // TODO: { "ANg3", &a_unknown },  /* Tinkerer - Robo-Goblin (Level 3) */
    // TODO: { "ANhs", &a_unknown },  /* Alchemist - Healing Spray */
    // TODO: { "ANia", &a_unknown },  /* Firelord - Incinerate */
    // TODO: { "ANic", &a_unknown },  /* Firelord - Incinerate */
    // TODO: { "ANin", &a_unknown },  /* Inferno */
    // TODO: { "ANlm", &a_unknown },  /* Firelord - Summon Lava Spawn */
    // TODO: { "ANmo", &a_unknown },  /* Monsoon */
    // TODO: { "ANmr", &a_unknown },  /* Mind Rot */
    // TODO: { "ANpi", &a_unknown },  /* Permanent Immolation */
    // TODO: { "ANpr", &a_unknown },  /* Preservation */
    // TODO: { "ANr3", &a_unknown },  /* Rain of Chaos(Button 0,2) */
    // TODO: { "ANrc", &a_unknown },  /* Rain of Chaos */
    // TODO: { "ANre", &a_unknown },  /* Neutral Regen (mana only) */
    // TODO: { "ANrg", &a_unknown },  /* Tinkerer - Robo-Goblin (Level 0) */
    // TODO: { "ANrn", &a_unknown },  /* Mannoroth - Reincarnation */
    // TODO: { "ANs1", &a_unknown },  /* Tinkerer - Summon Factory (Level 1) */
    // TODO: { "ANs2", &a_unknown },  /* Tinkerer - Summon Factory (Level 2) */
    // TODO: { "ANs3", &a_unknown },  /* Tinkerer - Summon Factory (Level 3) */
    // TODO: { "ANsa", &a_unknown },  /* Sanctuary */
    // TODO: { "ANse", &a_unknown },  /* Spell Shield AOE */
    // TODO: { "ANsl", &a_unknown },  /* Malganis - Soul Preservation */
    // TODO: { "ANso", &a_unknown },  /* Firelord - Soul Burn */
    // TODO: { "ANss", &a_unknown },  /* Spell Shield */
    // TODO: { "ANsy", &a_unknown },  /* Tinkerer - Summon Factory (Level 0) */
    // TODO: { "ANt2", &a_unknown },  /* Thorny Shield (Dragon Turtle) */
    // TODO: { "ANta", &a_unknown },  /* Taunt(Creep) */
    // TODO: { "ANth", &a_unknown },  /* Thorny Shield (Creep) */
    // TODO: { "ANtm", &a_unknown },  /* Alchemist - Transmute */
    // TODO: { "ANtr", &a_unknown },  /* Detect(War Eagle) */
    // TODO: { "ANvc", &a_unknown },  /* Firelord - Volcano */
    // TODO: { "ANwk", &a_unknown },  /* Wind Walk */
    // TODO: { "ANwm", &a_unknown },  /* Watery Minion */
    // TODO: { "AOsw", &a_unknown },  /* Shadow Hunter - Serpent Ward */
    // TODO: { "APdi", &a_unknown },  /* PowerupDispelAoe */
    // TODO: { "APh1", &a_unknown },  /* PowerupHealAoeLesser */
    // TODO: { "APh2", &a_unknown },  /* PowerupHealAoe */
    // TODO: { "APh3", &a_unknown },  /* PowerupHealAoeGreater */
    // TODO: { "APmg", &a_unknown },  /* RuneManaRestoreGreaterAoe */
    // TODO: { "APmr", &a_unknown },  /* RuneManaRestoreAoe */
    // TODO: { "APra", &a_unknown },  /* RuneRestoreAoe */
    // TODO: { "APrl", &a_unknown },  /* Rune of Lesser Resurrection */
    // TODO: { "APrr", &a_unknown },  /* Rune of Greater Resurrection */
    // TODO: { "APsa", &a_unknown },  /* RuneSpeedAoe */
    // TODO: { "APwt", &a_unknown },  /* Rune of the Watcher */
    // TODO: { "AUds", &a_unknown },  /* Dark Summoning */
    // TODO: { "Aabr", &a_unknown },  /* Aura - Regeneration (Statue) */
    // TODO: { "Aabs", &a_unknown },  /* Absorb Mana */
    // TODO: { "Aadm", &a_unknown },  /* Abolish Magic */
    // TODO: { "Aakb", &a_unknown },  /* Aura - War Drums */
    // TODO: { "Aam2", &a_unknown },  /* Anti-magic Shield (Matrix) */
    // TODO: { "Aams", &a_unknown },  /* Anti-magic Shield */
    // TODO: { "Aap1", &a_unknown },  /* Aura - Plague (Abomination) */
    // TODO: { "Aap2", &a_unknown },  /* Aura - Plague (Plague Ward) */
    // TODO: { "Aap3", &a_unknown },  /* Aura - Plague (Creep) */
    // TODO: { "Aap4", &a_unknown },  /* Aura - Plague (Creep gfx) */
    // TODO: { "Aasl", &a_unknown },  /* Aura - Slow */
    // TODO: { "Aast", &a_unknown },  /* Ancestral Spirit */
    // TODO: { "Aave", &a_unknown },  /* Avenger Form */
    // TODO: { "Abdl", &a_unknown },  /* Blight Dispel (Large) */
    // TODO: { "Abds", &a_unknown },  /* Blight Dispel (Small) */
    // TODO: { "Abgl", &a_unknown },  /* Blight Growth (Large) */
    // TODO: { "Abgs", &a_unknown },  /* Blight Growth (Small) */
    // TODO: { "Ablo", &a_unknown },  /* Bloodlust */
    // TODO: { "Ablp", &a_unknown },  /* BlightPlacement */
    // TODO: { "Abof", &a_unknown },  /* Balls of Fire */
    // TODO: { "Abrf", &a_unknown },  /* Bearform */
    // TODO: { "Absk", &a_unknown },  /* Beserk */
    // TODO: { "Abu2", &a_unknown },  /* Burrow(scarab lvl 2) */
    // TODO: { "Abu3", &a_unknown },  /* Burrow(scarab lvl 3) */
    // TODO: { "Abu5", &a_unknown },  /* Burrow(Barbed Arachnathid) */
    // TODO: { "Abur", &a_unknown },  /* Burrow */
    // TODO: { "Acan", &a_unknown },  /* Cannibalize */
    // TODO: { "Acdb", &a_unknown },  /* Chen- Drunken Brawler */
    // TODO: { "Achd", &a_unknown },  /* Cargo Hold Death */
    // TODO: { "Ache", &a_unknown },  /* Chain Dispel */
    // TODO: { "Achl", &a_unknown },  /* Chaos Cargo Load */
    // TODO: { "Acht", &a_unknown },  /* Howl of Terror */
    // TODO: { "Acn2", &a_unknown },  /* Cannibalize (Abomination) */
    // TODO: { "Aco2", &a_unknown },  /* Couple Instant (Archer) */
    // TODO: { "Aco3", &a_unknown },  /* Couple Instant (Hippogryph) */
    // TODO: { "Acoa", &a_unknown },  /* Couple (Archer) */
    // TODO: { "Acoh", &a_unknown },  /* Couple (Hippogryph) */
    // TODO: { "Acor", &a_unknown },  /* Corrosive Breath */
    // TODO: { "Acpf", &a_unknown },  /* Corporeal Form */
    // TODO: { "Acri", &a_unknown },  /* Cripple */
    // TODO: { "Acrs", &a_unknown },  /* Curse */
    // TODO: { "Acyc", &a_unknown },  /* Cyclone */
    // TODO: { "Adch", &a_unknown },  /* Disenchant(old) */
    // TODO: { "Adcn", &a_unknown },  /* Disenchant(new) */
    // TODO: { "Adda", &a_unknown },  /* Death Damage (sapper) */
    // TODO: { "Adec", &a_unknown },  /* Decouple */
    // TODO: { "Adev", &a_unknown },  /* Devour */
    // TODO: { "Adsm", &a_unknown },  /* Dispel Magic (creep) */
    // TODO: { "Adt1", &a_unknown },  /* Detect (Sentry Ward) */
    // TODO: { "Adtg", &a_unknown },  /* Detect (general) */
    // TODO: { "Adtn", &a_unknown },  /* Detonate */
    // TODO: { "Advc", &a_unknown },  /* Cargo Hold (Devour) */
    // TODO: { "Advm", &a_unknown },  /* Devour Magic */
    // TODO: { "Aegr", &a_unknown },  /* Elune's Grace */
    // TODO: { "Aenr", &a_unknown },  /* Entangling Roots (creep) */
    // TODO: { "Aens", &a_unknown },  /* Ensnare */
    // TODO: { "Aenw", &a_unknown },  /* Entangling Seaweed */
    // TODO: { "Aesn", &a_unknown },  /* Sentinel */
    // TODO: { "Aesr", &a_unknown },  /* Sentinel (no research) */
    // TODO: { "Aetf", &a_unknown },  /* Ethereal Form */
    // TODO: { "Aeth", &a_unknown },  /* Ghost (Visible) */
    // TODO: { "Aetl", &a_unknown },  /* Ethereal */
    // TODO: { "Aexh", &a_unknown },  /* Exhume */
    // TODO: { "Aeye", &a_unknown },  /* Sentry Ward */
    // TODO: { "Afa2", &a_unknown },  /* Faerie Fire */
    // TODO: { "Afae", &a_unknown },  /* Faerie Fire */
    // TODO: { "Afak", &a_unknown },  /* Orb of Annihilation */
    // TODO: { "Afbt", &a_unknown },  /* Feedback(Arcane Tower) */
    // TODO: { "Afod", &a_unknown },  /* Finger of Death */
    // TODO: { "Afr2", &a_unknown },  /* Frost Attack (1,2) */
    // TODO: { "Afra", &a_unknown },  /* Frost Attack */
    // TODO: { "Afrb", &a_unknown },  /* Frost Breath */
    // TODO: { "Afrz", &a_unknown },  /* Freezing Breath */
    // TODO: { "Afzy", &a_unknown },  /* Frenzy */
    // TODO: { "Agho", &a_unknown },  /* Ghost */
    // TODO: { "Agra", &a_unknown },  /* Grab Tree */
    // TODO: { "Agyd", &a_unknown },  /* Graveyard */
    // TODO: { "Ahid", &a_unknown },  /* Shadow Meld (Akama) */
    // TODO: { "Ahr2", &a_unknown },  /* Harvest Lumber (Arch ghouls) */
    // TODO: { "Ahr3", &a_unknown },  /* Harvest Lumber (shredder) */
    // TODO: { "Ahrp", &a_unknown },  /* Repair (Human) */
    // TODO: { "Ahwd", &a_unknown },  /* Healing Ward */
    // TODO: { "Aien", &a_unknown },  /* Inventory(2 slot unit) Night Elf */
    // TODO: { "Aihn", &a_unknown },  /* Inventory(2 slot unit) Human */
    // TODO: { "Aimp", &a_unknown },  /* Impaling Bolt */
    // TODO: { "Aion", &a_unknown },  /* Inventory(2 slot unit) Orc */
    // TODO: { "Aiun", &a_unknown },  /* Inventory(2 slot unit) Undead */
    // TODO: { "Alam", &a_unknown },  /* Sacrifice (Acolyte) */
    // TODO: { "Aliq", &a_unknown },  /* Liquid Fire */
    // TODO: { "Alit", &a_unknown },  /* Lightning Attack */
    // TODO: { "Alsh", &a_unknown },  /* Lightning Shield */
    // TODO: { "Amb2", &a_unknown },  /* Mana Battery (Obsidian Statue) */
    // TODO: { "Ambb", &a_unknown },  /* Mana Burn (Hotkey B) */
    // TODO: { "Ambd", &a_unknown },  /* Mana Burn (demon) */
    // TODO: { "Amec", &a_unknown },  /* MechanicalCritter */
    // TODO: { "Amed", &a_unknown },  /* Meat Drop */
    // TODO: { "Amel", &a_unknown },  /* Meat Load */
    // TODO: { "Amfl", &a_unknown },  /* Mana Flare */
    // TODO: { "Amgl", &a_unknown },  /* Moon Glaive */
    // TODO: { "Amgr", &a_unknown },  /* Moon Glaive (No research) */
    // TODO: { "Amim", &a_unknown },  /* Magic Immunity */
    // TODO: { "Amin", &a_unknown },  /* Mine */
    // TODO: { "Amnb", &a_unknown },  /* Mana Burn (demon) */
    // TODO: { "Amnx", &a_unknown },  /* Death Damage (mine) */
    // TODO: { "Amnz", &a_unknown },  /* Death Damage (mine BIG) */
    // TODO: { "Andt", &a_unknown },  /* Neutral Detection (Reveal ability) */
    // TODO: { "Ane2", &a_unknown },  /* Neutral Building (any unit) */
    // TODO: { "Anh1", &a_unknown },  /* Heal (Creep Normal) */
    // TODO: { "Anh2", &a_unknown },  /* Heal (Creep High) */
    // TODO: { "Anhe", &a_unknown },  /* Heal (Creep Normal) */
    // TODO: { "Ansk", &a_unknown },  /* Hardened Skin(Naga Turtle) */
    // TODO: { "Ansp", &a_unknown },  /* Neutral Spies */
    // TODO: { "Aoar", &a_unknown },  /* Aura - Regeneration (Ward) */
    // TODO: { "Apak", &a_unknown },  /* Inventory (Pack Mule) */
    // TODO: { "Apg2", &a_unknown },  /* Purge */
    // TODO: { "Apig", &a_unknown },  /* Permanent Immolation (graphic) */
    // TODO: { "Apiv", &a_unknown },  /* Permanent Invisibility */
    // TODO: { "Apmf", &a_unknown },  /* Permanent Immolation (flying) */
    // TODO: { "Apo2", &a_unknown },  /* Orb of Venom (Poison Attack) */
    // TODO: { "Apoi", &a_unknown },  /* Poison Attack */
    // TODO: { "Apos", &a_unknown },  /* Possession */
    // TODO: { "Aprg", &a_unknown },  /* Purge */
    // TODO: { "Aps2", &a_unknown },  /* Possession (Channeling) */
    // TODO: { "Apsh", &a_unknown },  /* Phase Shift */
    // TODO: { "Apts", &a_unknown },  /* Plague Toss */
    // TODO: { "Ara2", &a_unknown },  /* Roar */
    // TODO: { "Arai", &a_unknown },  /* Raise Dead */
    // TODO: { "Arbr", &a_unknown },  /* Reinforced Burrows */
    // TODO: { "Arej", &a_unknown },  /* Rejuvination */
    // TODO: { "Arel", &a_unknown },  /* Regen Life */
    // TODO: { "Aret", &a_unknown },  /* Retrain */
    // TODO: { "Argd", &a_unknown },  /* Return (Gold) */
    // TODO: { "Argl", &a_unknown },  /* Return (Gold & Lumber) */
    // TODO: { "Arll", &a_unknown },  /* Regen Life */
    // TODO: { "Arlm", &a_unknown },  /* Return (Lumber) */
    // TODO: { "Arng", &a_unknown },  /* Revenge */
    // TODO: { "Aro1", &a_unknown },  /* Root (Ancients) */
    // TODO: { "Aro2", &a_unknown },  /* Root (Ancient Protector) */
    // TODO: { "Aroa", &a_unknown },  /* Roar */
    // TODO: { "Arpb", &a_unknown },  /* Replenish (Life & Mana) */
    // TODO: { "Arpl", &a_unknown },  /* Replenish (Life) */
    // TODO: { "Arpm", &a_unknown },  /* Replenish (Mana) */
    // TODO: { "Arsk", &a_unknown },  /* Resistant Skin */
    // TODO: { "Asac", &a_unknown },  /* Sacrifice (Sacrificial Pit) */
    // TODO: { "Asal", &a_unknown },  /* Pillage */
    // TODO: { "Asd2", &a_unknown },  /* Self Destruct 2 (Clockwerk Goblins) */
    // TODO: { "Asd3", &a_unknown },  /* Self Destruct 3 (Clockwerk Goblins) */
    // TODO: { "Asdg", &a_unknown },  /* Self Destruct (Clockwerk Goblins) */
    // TODO: { "Asds", &a_unknown },  /* Self Destruct */
    // TODO: { "Ashm", &a_unknown },  /* Shadow Meld */
    // TODO: { "Ashs", &a_unknown },  /* ShadowSight */
    // TODO: { "Asid", &a_unknown },  /* Sell Item */
    // TODO: { "Asla", &a_unknown },  /* Sleep Always */
    // TODO: { "Aslp", &a_unknown },  /* Summon Lobstrok Prawns */
    // TODO: { "Asod", &a_unknown },  /* Spawn Skeleton */
    // TODO: { "Asou", &a_unknown },  /* SoulPossession */
    // TODO: { "Asp1", &a_unknown },  /* Sphere (SoV Level 1) */
    // TODO: { "Asp2", &a_unknown },  /* Sphere (SoV Level 2) */
    // TODO: { "Asp3", &a_unknown },  /* Sphere (SoV Level 3) */
    // TODO: { "Asp4", &a_unknown },  /* Sphere (SoV Level 4) */
    // TODO: { "Asp5", &a_unknown },  /* Sphere (SoV Level 5) */
    // TODO: { "Asp6", &a_unknown },  /* Sphere (SoV Level 6) */
    // TODO: { "Aspa", &a_unknown },  /* Spider Attack */
    // TODO: { "Aspb", &a_unknown },  /* Spell Book */
    // TODO: { "Aspd", &a_unknown },  /* Spawn Spider */
    // TODO: { "Aspi", &a_unknown },  /* Spiked Barricades */
    // TODO: { "Aspl", &a_unknown },  /* Spirit Link */
    // TODO: { "Aspo", &a_unknown },  /* Slow Poison */
    // TODO: { "Aspp", &a_unknown },  /* Rune of Spirit Link */
    // TODO: { "Aspt", &a_unknown },  /* Spawn Hydra Hatchling */
    // TODO: { "Aspy", &a_unknown },  /* Spawn Hydra */
    // TODO: { "Assk", &a_unknown },  /* Hardened Skin */
    // TODO: { "Assp", &a_unknown },  /* Spawn Spiderling */
    // TODO: { "Asta", &a_unknown },  /* Stasis Trap */
    // TODO: { "Aste", &a_unknown },  /* ManaSteal */
    // TODO: { "Astn", &a_unknown },  /* Stone Form */
    // TODO: { "Asud", &a_unknown },  /* Sell Unit */
    // TODO: { "Atau", &a_unknown },  /* Taunt */
    // TODO: { "Atdg", &a_unknown },  /* TornadoDamage */
    // TODO: { "Atol", &a_unknown },  /* Tree of life (for attaching art) */
    // TODO: { "Atru", &a_unknown },  /* Detect (Shade) */
    // TODO: { "Atsp", &a_unknown },  /* TornadoSpin */
    // TODO: { "Atwa", &a_unknown },  /* TornadoWander */
    // TODO: { "Auco", &a_unknown },  /* Unstable Concoction */
    // TODO: { "Auhf", &a_unknown },  /* Unholy Frenzy */
    // TODO: { "Ault", &a_unknown },  /* Ultravision */
    // TODO: { "Auns", &a_unknown },  /* Unsummon */
    // TODO: { "Aven", &a_unknown },  /* Venom Spears */
    // TODO: { "Avng", &a_unknown },  /* Vengeance */
    // TODO: { "Awan", &a_unknown },  /* Wander */
    // TODO: { "Awar", &a_unknown },  /* Pulverize */
    // TODO: { "Aweb", &a_unknown },  /* Web */
    // TODO: { "Awfb", &a_unknown },  /* Fire Bolt (warlock) */
    // TODO: { "Awh2", &a_unknown },  /* Wisp Harvest (Invulnerable) */
    // TODO: { "Awrg", &a_unknown },  /* War Stomp (sea giant) */
    // TODO: { "Awrh", &a_unknown },  /* War Stomp (hydra) */
    // TODO: { "Awrp", &a_unknown },  /* Warp */
    // TODO: { "Awrs", &a_unknown },  /* War Stomp (creep) */
    // TODO: { "SCae", &a_unknown },  /* Aura - Endurance (Creep) */
    // TODO: { "SCc1", &a_unknown },  /* Cyclone (Cenarius) */
    // TODO: { "SCva", &a_unknown },  /* Vampiric attack */
    // TODO: { "SNdc", &a_unknown },  /* Dark Conversion (Fast) */
    // TODO: { "SNdd", &a_unknown },  /* Super Death and Decay */
    // TODO: { "SNeq", &a_unknown },  /* Super Earthquake */
    // TODO: { "SNin", &a_unknown },  /* Tichondrius - Inferno */
    // TODO: { "Sbsk", &a_unknown },  /* Berserker Upgrade */
    // TODO: { "Sbtl", &a_unknown },  /* Battlestations (Chaos) */
    // TODO: { "Sca1", &a_unknown },  /* Chaos (Grunt) */
    // TODO: { "Sca2", &a_unknown },  /* Chaos (Raider) */
    // TODO: { "Sca3", &a_unknown },  /* Chaos (Shaman) */
    // TODO: { "Sca4", &a_unknown },  /* Chaos (Kodo) */
    // TODO: { "Sca5", &a_unknown },  /* Chaos (Peon) */
    // TODO: { "Sca6", &a_unknown },  /* Chaos (Grom) */
    // TODO: { "Sch2", &a_unknown },  /* Cargo Hold (Meat Wagon) */
    // TODO: { "Sch3", &a_unknown },  /* Cargo Hold (Transport) */
    // TODO: { "Sch4", &a_unknown },  /* Cargo Hold (Tank) */
    // TODO: { "Sch5", &a_unknown },  /* Cargo Hold (Ship) */
    // TODO: { "Scri", &a_unknown },  /* Cripple (Warlock) */
    // TODO: { "Sdro", &a_unknown },  /* Drop */
    // TODO: { "Slo2", &a_unknown },  /* Load (Entangled Gold Mine) */
    // TODO: { "Slo3", &a_unknown },  /* Load (Navies) */
    // TODO: { "Sloa", &a_unknown },  /* Load (Burrow) */
    // TODO: { "Srtt", &a_unknown },  /* Tank Upgrade */
    // TODO: { "Sshm", &a_unknown },  /* Shadow Meld (Instant) */
    // TODO: { "Stpm", &a_unknown },  /* Pilot Tank (Mortar Team) */
    // TODO: { "Stpr", &a_unknown },  /* PIlot Tank (Rifleman) */
    // TODO: { "Suhf", &a_unknown },  /* Unholy Frenzy (Warlock) */

    /* END GENERATED TODO ABILITIES */
};

/* Build a compact unique callback list once, rather than scan the whole registry per unit tick. */
static ability_t const *ability_updates[sizeof(abilitylist) / sizeof(abilitylist[0])];
static DWORD num_updates;

/* ROC/TFT physical data columns are normalized by the AbilityData DDX schema. */
FLOAT AB_Data(LPCSTR classname, DWORD level, DWORD index) {
    abilityLevel_t const *row = G_AbilityLevel(FS_SLKKey(classname), level);
    index = MAX(1, MIN(index, 9));
    return row->data[index - 1].number;
}

DWORD AB_DataId(LPCSTR classname, DWORD level, DWORD index) {
    abilityLevel_t const *row = G_AbilityLevel(FS_SLKKey(classname), level);
    index = MAX(1, MIN(index, 9));
    return row->data[index - 1].id;
}

/* Order names belong to their ability, including orders for preplaced alternate forms. */
ability_t const *FindAbilityByOrder(LPCSTR order) {
    if (!order) return NULL;
    FOR_LOOP(i, game.num_abilities) {
        ability_t const *ability = abilitylist[i].ability;
        if (!ability->orders || !ability->order) continue;
        for (LPCSTR const *name = ability->orders; *name; name++)
            if (!strcmp(*name, order)) return ability;
    }
    return NULL;
}

/* Persistent effects can outlive their active order; the callback owns its per-unit state checks. */
void S_RunAbilityUpdates(LPEDICT ent) {
    FOR_LOOP(i, num_updates)
        ability_updates[i]->update(ent);
}

ability_t const *FindAbilityByClassname(LPCSTR classname) {
    FOR_LOOP(i, game.num_abilities) {
        if (!abilitylist[i].classname)
            continue;
        if (!strcmp(abilitylist[i].classname, classname))
            return abilitylist[i].ability;
    }
    return NULL;
}

/* Command-card names use two namespaces. Engine commands (CmdBuild, CmdMove,
 * etc.) are full strings registered directly in abilitylist. WC3 abilities are
 * four-character rawcodes whose AbilityData alias may point at a base handler.
 * Only rawcodes belong in the SLK resolver: passing CmdBuild through FS_SLKKey
 * truncates it to CmdB and loses the registered build command. */
ability_t const *FindAbilityForCommand(LPCSTR classname) {
    if (!classname || !*classname) {
        return NULL;
    }
    if (strlen(classname) != 4) {
        return FindAbilityByClassname(classname);
    }
    return FindAbilityByClassname(GetClassName(G_AbilityCodeName(classname)));
}

static ability_t const *ability_for_code(DWORD code) {
    char name[5] = {0};
    memcpy(name, &code, 4);
    return FindAbilityForCommand(name);
}

void S_EnableAbility(LPEDICT ent, DWORD code) {
    ability_t const *ability = ability_for_code(code);
    if (ability && ability->enabled) ability->enabled(ent);
}

void S_DisableAbility(LPEDICT ent, DWORD code) {
    ability_t const *ability = ability_for_code(code);
    if (ability && ability->disabled) ability->disabled(ent);
}

void S_RefreshAbilityLevel(LPEDICT ent, ability_t const *ability) {
    if (ability && ability->level && ability->level_changed) ability->level_changed(ent, ability->level(ent));
}

static BOOL unit_has_ability_handler(LPEDICT ent, ability_t const *wanted) {
    LPCSTR abilities;

    if (!ent || !wanted || !ent->data.UnitAbilities) return false;
    abilities = ent->data.UnitAbilities->abilList;
    if (!abilities) return false;

    PARSE_LIST(abilities, ability_name, parse_segment) {
        if (FindAbilityForCommand(ability_name) == wanted) return true;
    }
    return false;
}

BOOL G_UnitAutocastIsOn(LPEDICT ent, ability_t const *ability) {
    return ent && ability && ability->autocast_is_on && unit_has_ability_handler(ent, ability) &&
           ability->autocast_is_on(ent);
}

BOOL G_SetUnitAutocast(LPEDICT ent, ability_t const *ability, BOOL enabled) {
    LPCSTR abilities;

    if (!ent || !ability || !ability->autocast_set || !unit_has_ability_handler(ent, ability)) {
#ifdef WC3_DEBUG_AUTOCAST
        if (G_AutocastDebugLevel() >= 1) {
            fprintf(stderr, "WC3_AUTOCAST toggle rejected unit=%ld ability=%p enabled=%d flags=0x%x\n",
                    ent && g_edicts ? (long)(ent - g_edicts) : -1L, (void *)ability,
                    enabled ? 1 : 0, ent ? ent->aiflags : 0);
        }
#endif
        return false;
    }

    /* Warsmash keeps one selected autocast ability per unit. Turning a new one
     * on first disables every other autocast-capable ability currently present
     * on this unit; abilities without autocast hooks remain untouched. */
    if (enabled && ent->data.UnitAbilities && (abilities = ent->data.UnitAbilities->abilList)) {
        PARSE_LIST(abilities, ability_name, parse_segment) {
            ability_t const *other = FindAbilityForCommand(ability_name);
            if (other && other != ability && other->autocast_set) other->autocast_set(ent, false);
        }
    }
    ability->autocast_set(ent, enabled);
    if (enabled) {
        ent->aiflags |= AI_AUTOCAST_ACTIVE;
    } else {
        BOOL any_enabled = false;
        if (ent->data.UnitAbilities && (abilities = ent->data.UnitAbilities->abilList)) {
            PARSE_LIST(abilities, ability_name, parse_segment) {
                ability_t const *other = FindAbilityForCommand(ability_name);
                if (other && other->autocast_is_on && other->autocast_is_on(ent)) {
                    any_enabled = true;
                    break;
                }
            }
        }
        if (!any_enabled) ent->aiflags &= ~AI_AUTOCAST_ACTIVE;
    }
#ifdef WC3_DEBUG_AUTOCAST
    if (G_AutocastDebugLevel() >= 1) {
        fprintf(stderr, "WC3_AUTOCAST toggle unit=%ld class=%.4s enabled=%d flags=0x%x idle_worker=%d abilities=%s\n",
                g_edicts ? (long)(ent - g_edicts) : -1L, (LPCSTR)&ent->class_id,
                enabled ? 1 : 0, ent->aiflags, G_UnitIsIdleWorker(ent) ? 1 : 0,
                ent->data.UnitAbilities && ent->data.UnitAbilities->abilList ? ent->data.UnitAbilities->abilList : "<none>");
    }
#endif
    return true;
}

BOOL G_TryUnitAutocast(LPEDICT ent) {
    LPCSTR abilities;

    if (!ent || !(ent->aiflags & AI_AUTOCAST_ACTIVE) || !ent->data.UnitAbilities ||
        !(abilities = ent->data.UnitAbilities->abilList)) {
#ifdef WC3_DEBUG_AUTOCAST
        if (G_AutocastDebugLevel() >= 2 && ent) {
            fprintf(stderr, "WC3_AUTOCAST skip unit=%ld class=%.4s flags=0x%x abilities=%s\n",
                    g_edicts ? (long)(ent - g_edicts) : -1L, (LPCSTR)&ent->class_id,
                    ent->aiflags,
                    ent->data.UnitAbilities && ent->data.UnitAbilities->abilList ? ent->data.UnitAbilities->abilList : "<none>");
        }
#endif
        return false;
    }
#ifdef WC3_DEBUG_AUTOCAST
    if (G_AutocastDebugLevel() >= 2) {
        fprintf(stderr, "WC3_AUTOCAST try unit=%ld class=%.4s flags=0x%x abilities=%s\n",
                g_edicts ? (long)(ent - g_edicts) : -1L, (LPCSTR)&ent->class_id,
                ent->aiflags, abilities);
    }
#endif
    PARSE_LIST(abilities, ability_name, parse_segment) {
        ability_t const *ability = FindAbilityForCommand(ability_name);
        BOOL is_on;
        if (!ability || !ability->autocast_is_on || !ability->autocast_acquire) continue;
        is_on = ability->autocast_is_on(ent);
#ifdef WC3_DEBUG_AUTOCAST
        if (G_AutocastDebugLevel() >= 2) {
            fprintf(stderr, "WC3_AUTOCAST ability unit=%ld code=%s on=%d\n",
                    g_edicts ? (long)(ent - g_edicts) : -1L, ability_name, is_on ? 1 : 0);
        }
#endif
        if (is_on && ability->autocast_acquire(ent)) {
#ifdef WC3_DEBUG_AUTOCAST
            if (G_AutocastDebugLevel() >= 1) {
                fprintf(stderr, "WC3_AUTOCAST acquired unit=%ld code=%s\n",
                        g_edicts ? (long)(ent - g_edicts) : -1L, ability_name);
            }
#endif
            return true;
        }
    }
#ifdef WC3_DEBUG_AUTOCAST
    if (G_AutocastDebugLevel() >= 2) {
        fprintf(stderr, "WC3_AUTOCAST no_target unit=%ld\n",
                g_edicts ? (long)(ent - g_edicts) : -1L);
    }
#endif
    return false;
}

DWORD FindAbilityIndex(LPCSTR classname) {
    FOR_LOOP(i, game.num_abilities) {
        if (!abilitylist[i].classname)
            continue;
        if (!strcmp(abilitylist[i].classname, classname))
            return i;
    }
    return 255;
}

void InitAbilities(void) {
    game.num_abilities = sizeof(abilitylist)/sizeof(abilitylist[0]);
    S_WireAbilityParents();
    S_InitHumanAbilities();
    num_updates = 0;
    FOR_LOOP(i, game.num_abilities) {
        abilityitem_t *abil = &abilitylist[i];
        if (abil->ability->init) {
            abil->ability->init(abil->classname, abil->ability);
        }
        if (abil->ability->update) {
            DWORD n;
            for (n = 0; n < num_updates && ability_updates[n] != abil->ability; n++) {}
            if (n == num_updates) ability_updates[num_updates++] = abil->ability;
        }
    }
}

void SetAbilityNames(void) {
//    FOR_LOOP(i, game.num_abilities) {
//        if (!abilitylist[i].classname)
//            continue;
//        abilityitem_t *abil = &abilitylist[i];
//    }
}

ability_t const *GetAbilityByIndex(DWORD index) {
    if (index >= game.num_abilities)
        return NULL;
    return abilitylist[index].ability;
}

DWORD GetAbilityIndex(ability_t const *ability) {
    FOR_LOOP(i, game.num_abilities) {
        if (abilitylist[i].ability == ability) {
            return i;
        }
    }
    return 255;
}
