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

static abilityitem_t abilitylist[] = {
    { STR_CmdStop, &a_stop },  // Stop — engine command
    { STR_CmdMove, &a_move },  // Move — engine command
    { STR_CmdAttack, &a_attack },  // Attack — engine command
    { STR_CmdBuild, &a_build },  // Build — engine command
    { STR_CmdHoldPos, &a_holdpos },  // Hold Position — engine command
    { STR_CmdPatrol, &a_patrol },  // Patrol — engine command
    { STR_CmdRally, &a_rally },  // Rally — engine command
    { STR_CmdCancel, &a_cancel },  // Cancel — engine command
    { STR_CmdCancelBuild, &a_cancel },  // Cancel Build — engine command
    { STR_CmdSelectSkill, &a_selectskill },  // Select Skill — engine command

    { "Ahar", &a_harvest },  // Harvest — CAbilityHarvest other
    { "Amic", &a_call_to_arms },  // Call To Arms — CAbilityBaseBuild human
    { "Amil", &a_militia },  // Call to Arms — CAbilityMorph human
    { "Arep", &a_repair },  // Repair — CAbilityRepair orc
    { "Agld", &a_goldmine },  // Gold Mine ability — CAbilityGoldMine other
    { "AHad", &a_devotionaura },  // Devotion Aura — CAbilityAuraBrilliance [HERO] human
    { "AHhb", &a_holylight },  // Holy Light — CAbilityHolyLight [HERO] human
    { "AHwe", &a_water_elemental },  // Summon Water Elemental — CAbilityWaterElemental [HERO] human
    { "AEfn", &a_force_of_nature },  // Force of Nature — CAbilityForceOfNature [HERO] nightelf
    { "AHbz", &a_blizzard },  // Blizzard — CAbilityBlizzard [HERO] human
    { "AEsf", &a_starfall },  // Starfall — CAbilityWhirlwind [HERO] nightelf
    { "AOsh", &a_shockwave },  // Shockwave — CAbilityShockwave [HERO] orc
    { "ANrf", &a_rain_of_fire },  // Rain of Fire — CAbilityRainOfFire [HERO] creeps
    { "AEtq", &a_tranquility },  // Tranquility — CAbilityAuraRegenLife [HERO] nightelf
    { "AHtb", &a_thunderbolt },  // Storm Bolt — CAbilityThunderBolt [HERO] human
    { "ANfb", &a_firebolt },  // Firebolt — CAbilityCreepThunderBolt [HERO] creeps
    { "Apxf", &a_phoenix_fire },  // Phoenix Fire — CAbilityPermImmolation human
    { "AOsf", &a_feral_spirit },  // Feral Spirit — CAbilitySpiritWolf [HERO] orc
    { "AOmi", &a_mirror_image },  // Mirror Image — CAbilityMirrorImage [HERO] orc
    { "Abun", &a_cargo_hold_burrow },  // Cargo Hold (Orc Burrow) — CAbilityBunker orc
    { "Astd", &a_stand_down },  // Stand Down — CAbilityStandDown orc
    { "AEim", &a_immolation },  // Immolation — CAbilityImmolation [HERO] nightelf
    { "Aenc", &a_cargo_hold_entangled_mine },  // Load — CAbilityCargoHold nightelf
    { "Aent", &a_entangle_goldmine },  // Entangle Gold Mine — CAbilityGoldMine nightelf
    { "Aegm", &a_entangled_mine },  // Entangled Gold Mine Ability — CAbilityGoldMine nightelf
    { "Aeat", &a_eat_tree },  // Eat Tree — CAbilityHarvest nightelf
    { "Ambt", &a_moon_well },  // Replenish Mana and Life — CAbilityRegenMana nightelf
    { "ANch", &a_charm },  // Charm — CAbilitySpell [HERO] creeps
    { "AIco", &a_charm },  // Item Command — CAbilityButton [ITEM] other
    { "AHca", &a_cold_arrows },  // Cold Arrows — CAbilityPoisonAttack [HERO] creeps
    { "Agl2", &a_goldmine_overlayed },  // Gold Mine ability — CAbilityGoldMine other
    { "Abgm", &a_blighted_goldmine },  // Blighted Gold Mine Ability — CAbilityGoldMineBase undead
    { "Abli", &a_blight },  // Blight — CAbilityButton undead
    { "Aaha", &a_acolyte_harvest },  // Gather — CAbilityHarvest undead
    { "Artn", &a_return_resources },  // Return — CAbilityReturn other
    { "Awha", &a_wisp_harvest },  // Gather — CAbilityHarvest nightelf
    { "Ahrl", &a_harvest_lumber },  // Harvest — CAbilityHarvestLumber undead
    { "ANcl", &a_channel_test },  // Channel — CAbilitySpell [HERO] creeps
    { "AUcs", &a_carrion_swarm },  // Carrion Swarm — CAbilityCarrionSwarm [HERO] undead
    { "AInv", &a_inventory },  // Inventory — CAbilityInventory other
    { "Aren", &a_repair_generic },  // Renew — CAbilityRepair nightelf
    { "Arst", &a_repair_generic },  // Restore — CAbilityRepair undead
    { "Avul", &a_invulnerable },  // Invulnerable — CAbilityInvulnerable other
    { "Apit", &a_shop_purchase_item },  // Shop Purchase Item — CAbilityBaseSell other
    { "Aneu", &a_neutral_building },  // Select Hero — CAbilityNeutralBuild other
    { "Aall", &a_shop_sharing },  // Shop Sharing, Allied Bldg. — CAbilityButton other
    { "Acoi", &a_couple_instant },  // Couple Instant — CAbilityMorph other
    { "AIhe", &a_item_heal },  // Item Healing — CAbilityItemHeal [ITEM] other
    { "AIma", &a_item_mana_regain },  // Item Mana Regain — CAbilityItemManaRestore [ITEM] other
    { "AIat", &a_item_attack_bonus },  // Item Damage Bonus — CAbilityAttackBonus [ITEM] other
    { "AIab", &a_item_stat_bonus },  // Item Hero Stat Bonus — CAbilityAttributeBonus [ITEM] other
    { "AIim", &a_item_permanent_stat_gain },  // Item Intelligence Gain — CAbilityTome [ITEM] other
    { "AIsm", &a_item_permanent_stat_gain },  // Item Strength Gain — CAbilityTome [ITEM] other
    { "AIam", &a_item_permanent_stat_gain },  // Item Agility Gain — CAbilityTome [ITEM] other
    { "AIxm", &a_item_permanent_stat_gain },  // Item Int/Agi/Str gain — CAbilityTome [ITEM] other
    { "AIde", &a_item_defense_bonus },  // Item Armor Bonus — CAbilityDefenseBonus [ITEM] other
    { "AIml", &a_item_life_bonus },  // Item Life Bonus — CAbilityMaxLifeBonus [ITEM] other
    { "AImm", &a_item_mana_bonus },  // Item Mana Bonus — CAbilityMaxManaBonus [ITEM] other
    { "AIfs", &a_item_figurine_summon },  // Item Skeleton Summon — CAbilityFigurine [ITEM] other
    { "AImi", &a_item_permanent_life_gain },  // Item Permanent Life Gain — CAbilityTome [ITEM] other
    { "AIem", &a_item_experience_gain },  // Item Experience Gain — CAbilityExperienceMod [ITEM] other
    { "AIlm", &a_item_level_gain },  // Item Level Gain — CAbilityLevelMod [ITEM] other
    { "AIda", &a_item_defense_aoe },  // Item Temporary Area Armor Bonus — CAbilityItemDefenseAoe [ITEM] other
    { "AIct", &a_item_change_time },  // Change Time of Day — CAbilityButton [ITEM] other
    { "Acar", &a_cargo_hold },  // Cargo Hold — CAbilityCargoHold other
    { "Aloa", &a_load },  // Load — CAbilityCargoLoad other
    { "Adro", &a_drop },  // Unload — CAbilityCargoDrop other
    { "Adri", &a_drop_instant },  // Unload Instant — CAbilityCargoDropInstant other
    { "Aroo", &a_root },  // Root — CAbilityMorph nightelf

    /* Night Elf Warden (Maiev) hero abilities. */
    { "AEbl", &a_blink },  // Blink — CAbilityMove [HERO] nightelf
    { "AEfk", &a_fan_of_knives },  // Fan of Knives — CAbilityBounce [HERO] nightelf
    { "AEsh", &a_shadow_strike },  // Shadow Strike — CAbilityThunderBolt [HERO] nightelf

    /* Additional hero spells. */
    { "ANfs", &a_flame_strike },  // Flame Strike — CAbilityRainOfFire [HERO] creeps
    { "ANdr", &a_siphon_mana },  // Life Drain — CAbilitySpell [HERO] creeps
    { "AEmb", &a_mana_burn },  // Mana Burn — CAbilitySpell [HERO] nightelf
    { "AOws", &a_war_stomp },  // War Stomp — CAbilityWarStomp [HERO] orc
    { "AOae", &a_aura_endurance },  // Endurance Aura — CAbilityAuraEndurance [HERO] orc
    { "AOwk", &a_wind_walk },  // Wind Walk — CAbilityWindWalk [HERO] orc
    { "AHbh", &a_bash },  // Bash — CAbilityBash [HERO] human


/* Missing WC3 abilities with Game.dll C++ class names */
/* Generated by ability_map from Game.dll RTTI + AbilityData.slk */

    { "AHab", &a_brilliance_aura },  // Brilliance Aura — CAbilityAuraBrilliance [HERO] human
    { "AHmt", &a_mass_teleport },  // Mass Teleport — CAbilityMassTeleport [HERO] human
    { "ANst", &a_stomp },  // Stampede — CAbilityStomp [HERO] creeps
    { "ANsg", &a_summon_bear },  // Summon Bear — CAbilityForceOfNature [HERO] creeps
    { "ANsq", &a_summon_quilbeast },  // Summon Quilbeast — CAbilityForceOfNature [HERO] creeps
    { "ANsw", &a_summon_hawk },  // Summon Hawk — CAbilityForceOfNature [HERO] creeps
    { "AOww", &a_whirlwind },  // Bladestorm — CAbilityWhirlwind [HERO] orc
    { "AOcr", &a_critical_strike },  // Critical Strike — CAbilityCriticalStrike [HERO] orc
    { "AHbn", &a_banish },  // Banish — CAbilitySpell [HERO] human
    { "AHfs", &a_flame_strike_human },  // Flame Strike — CAbilityRainOfFire [HERO] human
    { "AHdr", &a_siphon_mana_human },  // Siphon Mana — CAbilitySpell [HERO] human
    { "AHpx", &a_phoenix },  // Phoenix — CAbilityMorph [HERO] human
    { "AUcb", &a_carrion_beetles },  // Carrion Beetles — CAbilityMorph [HERO] undead
    { "AUim", &a_impale },  // Impale — CAbilityStomp [HERO] undead
    { "AUls", &a_locust_swarm },  // Locust Swarm — CAbilityCreepSleep [HERO] undead
    { "AUts", &a_spiked_carapace },  // Spiked Carapace — CAbilityAura [HERO] undead
    { "ANba", &a_black_arrow },  // Black Arrow — CAbilityPoisonAttack [HERO] creeps
    { "ANsi", &a_silence },  // Silence — CAbilitySpell [HERO] creeps
    { "AUan", &a_animate_dead },  // Animate Dead — CAbilityRevenge [HERO] undead
    { "AUdc", &a_death_coil },  // Death Coil — CAbilitySpell [HERO] undead
    { "AUdp", &a_death_pact },  // Death Pact — CAbilitySpell [HERO] undead
    { "AUau", &a_unholy_aura },  // Unholy Aura — CAbilityAuraRegenLife [HERO] undead
    { "AEev", &a_evasion },  // Evasion — CAbilityEvasion [HERO] nightelf
    { "AEme", &a_metamorphosis },  // Metamorphosis — CAbilityMorph [HERO] nightelf
    { "AUsl", &a_sleep },  // Sleep — CAbilityCreepSleep [HERO] undead
    { "AUav", &a_vampiric_aura },  // Vampiric Aura — CAbilityAura [HERO] undead
    { "AUin", &a_inferno },  // Inferno — CAbilityStomp [HERO] undead
    { "AOcl", &a_chain_lightning },  // Chain Lightning — CAbilityChainLightning [HERO] orc
    { "ANfl", &a_forked_lightning },  // Forked Lightning — CAbilityBounce [HERO] creeps
    { "AOeq", &a_earthquake },  // Earthquake — CAbilityEarthquake [HERO] orc
    { "AOfs", &a_far_sight },  // Far Sight — CAbilityFarSight [HERO] orc
    { "AEer", &a_entangling_roots },  // Entangling Roots — CAbilityTarget [HERO] nightelf
    { "AEah", &a_aura_spell },  // Thorns Aura — CAbilityAuraSpell [HERO] nightelf
    { "AUdr", &a_dark_ritual },  // Dark Ritual — CAbilitySpell [HERO] undead
    { "AUdd", &a_death_and_decay },  // Death And Decay — CAbilityRainOfFire [HERO] undead
    { "AUfa", &a_frost_armor },  // Frost Armor — CAbilitySpell [HERO] undead
    { "AUfu", &a_frost_armor_variant },  // Frost Armor — CAbilitySpell [HERO] undead
    { "AUfn", &a_frost_nova },  // Frost Nova — CAbilityFrostNova [HERO] undead
    // TODO: { "AHav", &a_attribute_mod },  // Avatar — CAbilityAttributeMod [HERO] human
    { "AHtc", &a_thunder_clap },  // Thunder Clap — CAbilityThunderClap [HERO] human
    { "AHds", &a_divine_shield },  // Divine Shield — CAbilityInvulnerable [HERO] human
    // TODO: { "ANto", &a_whirlwind },  // Tornado — CAbilityWhirlwind [HERO] creeps
    // TODO: { "ANms", &a_aura },  // Mana Shield — CAbilityAura [HERO] creeps
    // TODO: { "AHre", &a_revive },  // Resurrection — CAbilityRevive [HERO] human
    // TODO: { "ANbf", &a_immolation },  // Breath of Fire — CAbilityImmolation [HERO] creeps
    // TODO: { "ANdb", &a_revenge },  // Drunken Brawler — CAbilityRevenge [HERO] creeps
    // TODO: { "ANdh", &a_spell },  // Drunken Haze — CAbilitySpell [HERO] creeps
    // TODO: { "ANef", &a_button },  // "Storm, Earth, And Fire" — CAbilityButton [HERO] creeps
    // TODO: { "ANdo", &a_spell },  // Doom — CAbilitySpell [HERO] creeps
    // TODO: { "ANht", &a_healing_ward },  // Howl of Terror — CAbilityHealingWard [HERO] creeps
    // TODO: { "ANca", &a_chain_lightning },  // Cleaving Attack — CAbilityChainLightning [HERO] creeps
    // TODO: { "AHfa", &a_poison_attack },  // Searing Arrows — CAbilityPoisonAttack [HERO] nightelf
    // TODO: { "AEst", &a_button },  // Scout — CAbilityButton [HERO] nightelf
    // TODO: { "AEar", &a_aura },  // Trueshot Aura — CAbilityAura [HERO] nightelf
    // TODO: { "AOre", &a_reincarnation },  // Reincarnation — CAbilityReincarnation [HERO] orc
    // TODO: { "AOhw", &a_spell },  // Healing Wave — CAbilitySpell [HERO] orc
    // TODO: { "AOhx", &a_spell },  // Hex — CAbilitySpell [HERO] orc
    // TODO: { "AOwd", &a_unknown },  // Shadow Hunter - Serpent Ward — CAbility [HERO] orc
    // TODO: { "AOvd", &a_spell },  // Big Bad Voodoo — CAbilitySpell [HERO] orc
    // TODO: { "AEsv", &a_morph },  // Vengeance — CAbilityMorph [HERO] nightelf
    // TODO: { "ANab", &a_button },  // Acid Bomb — CAbilityButton [HERO] creeps
    // TODO: { "ANcr", &a_critical_strike },  // Chemical Rage — CAbilityCriticalStrike [HERO] creeps
    // TODO: { "ANhs", &a_heal },  // Healing Spray — CAbilityHeal [HERO] creeps
    // TODO: { "ANtm", &a_aura_regen_life },  // Transmute — CAbilityAuraRegenLife [HERO] creeps
    // TODO: { "ANeg", &a_evasion },  // Engineering Upgrade — CAbilityEvasion [HERO] creeps
    // TODO: { "ANcs", &a_spell },  // Cluster Rockets — CAbilitySpell [HERO] creeps
    // TODO: { "ANrg", &a_regen_base },  // Robo-Goblin — CAbilityRegenBase [HERO] creeps
    // TODO: { "ANsy", &a_button },  // Pocket Factory — CAbilityButton [HERO] creeps
    // TODO: { "ANde", &a_button },  // Demolish — CAbilityButton [HERO] creeps
    // TODO: { "ANic", &a_inner_fire },  // Incinerate — CAbilityInnerFire [HERO] creeps
    // TODO: { "ANia", &a_inner_fire },  // Incinerate — CAbilityInnerFire [HERO] creeps
    // TODO: { "ANso", &a_spell },  // Soul Burn — CAbilitySpell [HERO] creeps
    // TODO: { "ANlm", &a_lightning_shield },  // Summon Lava Spawn — CAbilityLightningShield [HERO] creeps
    // TODO: { "ANvc", &a_spell },  // Volcano — CAbilitySpell [HERO] creeps
    // TODO: { "ANin", &a_creep_thunder_bolt },  // Inferno — CAbilityCreepThunderBolt [HERO] creeps
    // TODO: { "ANfd", &a_spell },  // Finger of Death — CAbilitySpell [HERO] creeps
    // TODO: { "ANdp", &a_mass_teleport },  // Dark Portal — CAbilityMassTeleport [HERO] creeps
    // TODO: { "ANrc", &a_rain_of_fire },  // Rain of Chaos — CAbilityRainOfFire [HERO] creeps
    // TODO: { "ANdc", &a_spell },  // Dark Conversion — CAbilitySpell [HERO] creeps
    // TODO: { "ANsl", &a_spell },  // Soul Preservation — CAbilitySpell [HERO] creeps
    // TODO: { "ANmo", &a_bounce },  // Monsoon — CAbilityBounce [HERO] creeps
    // TODO: { "AEpa", &a_poison_attack },  // Poison Arrows — CAbilityPoisonAttack [HERO] creeps
    // TODO: { "ANwm", &a_war_stomp },  // Watery Minion — CAbilityWarStomp [HERO] creeps
    // TODO: { "ANbr", &a_bash },  // Battle Roar — CAbilityBash [HERO] creeps
    // TODO: { "Aamk", &a_attack_mod },  // Attribute Modifier Skill — CAbilityAttackMod [HERO] creeps
    // TODO: { "Aadm", &a_auto_dispel_magic },  // Abolish Magic — CAbilityAutoDispelMagic nightelf
    // TODO: { "Aabs", &a_aura },  // Absorb Mana — CAbilityAura undead
    // TODO: { "Aalr", &a_alarm },  // Alarm — CAbilityAlarm other
    // TODO: { "Aast", &a_aura },  // Ancestral Spirit — CAbilityAura orc
    // TODO: { "ACad", &a_revenge },  // Animate Dead — CAbilityRevenge creeps
    // TODO: { "Aams", &a_magic_immunity },  // Anti-magic Shell — CAbilityMagicImmunity undead
    // TODO: { "Aatk", &a_attack },  // Attack — CAbilityAttack other
    // TODO: { "AOac", &a_unknown },  // Command Aura — CAbility creeps
    // TODO: { "Aapl", &a_unknown },  // Disease Cloud — CAbility undead
    // TODO: { "Aoar", &a_aura_regen_life },  // Healing Ward Aura — CAbilityAuraRegenLife orc
    // TODO: { "Aabr", &a_aura_regen_life },  // Aura of Blight — CAbilityAuraRegenLife undead
    // TODO: { "Aasl", &a_slow },  // Slow Aura — CAbilitySlow naga
    // TODO: { "Aakb", &a_aura_command },  // War Drums — CAbilityAuraCommand orc
    // TODO: { "Aave", &a_stomp },  // Destroyer Form  — CAbilityStomp undead
    // TODO: { "Aawa", &a_war_stomp },  // Revive Hero Instantly — CAbilityWarStomp other
    // TODO: { "Abof", &a_aura_command },  // Burning Oil — CAbilityAuraCommand orc
    // TODO: { "Abtl", &a_battlestations },  // Battle Stations — CAbilityBattlestations orc
    // TODO: { "Abrf", &a_morph },  // Bear Form — CAbilityMorph nightelf
    // TODO: { "Absk", &a_button },  // Berserk — CAbilityButton orc
    // TODO: { "Acha", &a_unknown },  // Chaos — CAbility orc
    // TODO: { "Ablo", &a_bloodlust },  // Bloodlust — CAbilityBloodlust orc
    // TODO: { "ACbf", &a_on_fire },  // Breath of Frost — CAbilityOnFire creeps
    // TODO: { "ANbu", &a_neutral_build },  // Build (Neutral) — CAbilityNeutralBuild other
    // TODO: { "AHbu", &a_human_build },  // Build (Human) — CAbilityHumanBuild human
    // TODO: { "AObu", &a_orc_build },  // Build (Orc) — CAbilityOrcBuild orc
    // TODO: { "AEbu", &a_night_elf_build },  // Build (Night Elf) — CAbilityNightElfBuild nightelf
    // TODO: { "AUbu", &a_undead_build },  // Build (Undead) — CAbilityUndeadBuild undead
    // TODO: { "AGbu", &a_neutral_build },  // Build (Naga) — CAbilityNeutralBuild other
    // TODO: { "Abur", &a_creep_sleep },  // Burrow — CAbilityCreepSleep undead
    // TODO: { "Abdt", &a_creep_sleep },  // Burrow Detection — CAbilityCreepSleep other
    // TODO: { "Acan", &a_cannibalize },  // Cannibalize — CAbilityCannibalize undead
    // TODO: { "Advc", &a_devour_cargo },  // Devour Cargo — CAbilityDevourCargo orc
    // TODO: { "Amtc", &a_unknown },  // Cargo Hold — CAbility undead
    // TODO: { "Achd", &a_cargo_hold },  // Cargo Hold Death — CAbilityCargoHold other
    // TODO: { "AIdc", &a_item_defense_aoe },  // Item Chain Dispel — CAbilityItemDefenseAoe creeps
    // TODO: { "Achl", &a_cargo_load },  // Chaos Cargo Load — CAbilityCargoLoad orc
    // TODO: { "Aclf", &a_bounce },  // Cloud — CAbilityBounce human
    // TODO: { "Acmg", &a_spell },  // Control Magic — CAbilitySpell human
    // TODO: { "Acpf", &a_purge },  // Corporeal Form — CAbilityPurge orc
    // TODO: { "Acor", &a_bounce },  // Corrosive Breath — CAbilityBounce nightelf
    // TODO: { "Acoa", &a_morph },  // Mount Hippogryph — CAbilityMorph nightelf
    // TODO: { "Acoh", &a_morph },  // Pick up Archer — CAbilityMorph nightelf
    // TODO: { "ACsp", &a_creep_sleep },  // Sleep — CAbilityCreepSleep creeps
    // TODO: { "Acri", &a_cripple },  // Cripple — CAbilityCripple undead
    // TODO: { "Acrs", &a_auto_target_spell },  // Curse — CAbilityAutoTargetSpell undead
    // TODO: { "Acyc", &a_morph },  // Cyclone — CAbilityMorph nightelf
    // TODO: { "Adda", &a_on_fire },  // AOE damage upon death — CAbilityOnFire other
    // TODO: { "Adec", &a_defend },  // Dismount — CAbilityDefend nightelf
    // TODO: { "Adef", &a_defend },  // Defend — CAbilityDefend human
    // TODO: { "Adet", &a_unknown },  // Detector — CAbility orc
    // TODO: { "Atru", &a_true_sight },  // True Sight — CAbilityTrueSight undead
    // TODO: { "Agyv", &a_true_sight },  // True Sight — CAbilityTrueSight human
    // TODO: { "Adts", &a_magic_sentry },  // Magic Sentry — CAbilityMagicSentry human
    // TODO: { "Adtn", &a_button },  // Detonate — CAbilityButton nightelf
    // TODO: { "Adev", &a_devour },  // Devour — CAbilityDevour orc
    // TODO: { "ACdv", &a_creep_devour },  // Devour — CAbilityCreepDevour creeps
    // TODO: { "Advm", &a_dispel_magic },  // Devour Magic — CAbilityDispelMagic undead
    // TODO: { "Adch", &a_spell },  // Disenchant — CAbilitySpell orc
    // TODO: { "Adis", &a_dispel_magic },  // Dispel Magic — CAbilityDispelMagic orc
    // TODO: { "Atdp", &a_cargo_drop },  // Drop Pilot — CAbilityCargoDrop human
    // TODO: { "AIdd", &a_item_heal },  // Defend — CAbilityItemHeal nightelf
    // TODO: { "Aens", &a_ensnare },  // Ensnare — CAbilityEnsnare naga
    // TODO: { "Aetl", &a_button },  // Ethereal — CAbilityButton nightelf
    // TODO: { "Aetf", &a_morph },  // Ethereal Form — CAbilityMorph orc
    // TODO: { "Aexh", &a_button },  // Exhume Corpses — CAbilityButton undead
    // TODO: { "ANfy", &a_on_fire },  // Factory — CAbilityOnFire creeps
    // TODO: { "Afae", &a_auto_target_spell },  // Faerie Fire — CAbilityAutoTargetSpell nightelf
    // TODO: { "Afbk", &a_aura },  // Feedback — CAbilityAura human
    // TODO: { "Aflk", &a_button },  // Flak Cannons — CAbilityButton human
    // TODO: { "Afla", &a_button },  // Flare — CAbilityButton human
    // TODO: { "Afsh", &a_button },  // Fragmentation Shards — CAbilityButton human
    // TODO: { "Afrz", &a_frost_nova },  // Freezing Breath — CAbilityFrostNova undead
    // TODO: { "Afzy", &a_spell },  // Frenzy — CAbilitySpell creeps
    // TODO: { "Afra", &a_button },  // Frost Attack — CAbilityButton undead
    // TODO: { "Afrb", &a_button },  // Frost Breath — CAbilityButton undead
    // TODO: { "Agho", &a_ghost },  // Ghost — CAbilityGhost undead
    // TODO: { "Aeth", &a_ghost },  // Ghost — CAbilityGhost undead
    // TODO: { "Agra", &a_poison_attack },  // War Club — CAbilityPoisonAttack nightelf
    // TODO: { "Agyd", &a_simple_spell },  // Create Corpse — CAbilitySimpleSpell undead
    // TODO: { "Agyb", &a_bounce },  // Flying Machine Bombs — CAbilityBounce human
    // TODO: { "Assk", &a_button },  // Hardened Skin — CAbilityButton nightelf
    // TODO: { "Ahea", &a_heal },  // Heal — CAbilityHeal human
    // TODO: { "Anhe", &a_heal },  // Heal — CAbilityHeal creeps
    // TODO: { "Ahwd", &a_healing_ward },  // Healing Ward — CAbilityHealingWard orc
    // TODO: { "Aroa", &a_roar },  // Roar — CAbilityRoar naga
    // TODO: { "AHer", &a_hero },  // Hero — CAbilityHero other
    // TODO: { "Aimp", &a_bounce },  // Impaling Bolt — CAbilityBounce nightelf
    // TODO: { "Ainf", &a_inner_fire },  // Inner Fire — CAbilityInnerFire human
    // TODO: { "Aivs", &a_perm_invis },  // Invisibility — CAbilityPermInvis human
    // TODO: { "Alit", &a_lightning_attack },  // Lightning Attack — CAbilityLightningAttack nightelf
    // TODO: { "Alsh", &a_lightning_shield },  // Lightning Shield — CAbilityLightningShield orc
    // TODO: { "Aliq", &a_on_fire },  // Liquid Fire — CAbilityOnFire orc
    // TODO: { "Atlp", &a_cargo_load },  // Load Pilot — CAbilityCargoLoad human
    // TODO: { "Aloc", &a_button },  // Locust — CAbilityButton undead
    // TODO: { "Amdf", &a_aura },  // Magic Defense — CAbilityAura human
    // TODO: { "Amim", &a_magic_immunity },  // Spell Immunity — CAbilityMagicImmunity nightelf
    // TODO: { "Amls", &a_aura },  // Aerial Shackles — CAbilityAura human
    // TODO: { "Amfl", &a_frost_nova },  // Mana Flare — CAbilityFrostNova nightelf
    // TODO: { "Amed", &a_cargo_drop },  // Drop Corpse — CAbilityCargoDrop undead
    // TODO: { "Amel", &a_cargo_load },  // Get Corpse — CAbilityCargoLoad undead
    // TODO: { "ANmr", &a_aura },  // Mind Rot — CAbilityAura creeps
    // TODO: { "Amin", &a_button },  // Mine - exploding — CAbilityButton other
    // TODO: { "Amgl", &a_bounce },  // Moon Glaive — CAbilityBounce nightelf
    // TODO: { "Amov", &a_move },  // Move — CAbilityMove other
    // TODO: { "Andt", &a_evil_eye },  // Reveal — CAbilityEvilEye creeps
    // TODO: { "Aarm", &a_unknown },  // Mana Regeneration Aura — CAbility creeps
    // TODO: { "AAns", &a_neutral_spell },  // Charge Gold and Lumber — CAbilityNeutralSpell creeps
    // TODO: { "Ansp", &a_neutral_spell },  // Neutral Spies — CAbilityNeutralSpell creeps
    // TODO: { "Afak", &a_spell },  // Orb of Annihilation — CAbilitySpell undead
    // TODO: { "Afir", &a_on_fire },  // On Fire — CAbilityOnFire other
    // TODO: { "Afih", &a_on_fire_human },  // On Fire (Human) — CAbilityOnFireHuman human
    // TODO: { "Afio", &a_on_fire_orc },  // On Fire (Orc) — CAbilityOnFireOrc orc
    // TODO: { "Afin", &a_on_fire_night_elf },  // On Fire (Night Elf) — CAbilityOnFireNightElf nightelf
    // TODO: { "Afiu", &a_on_fire_undead },  // On Fire (Undead) — CAbilityOnFireUndead undead
    // TODO: { "ANpa", &a_poison_attack },  // Parasite — CAbilityPoisonAttack naga
    // TODO: { "ANpi", &a_perm_immolation },  // Permanent Immolation — CAbilityPermImmolation other
    // TODO: { "Apig", &a_harvest_return },  // Permanent Immolation — CAbilityHarvestReturn other
    // TODO: { "Apiv", &a_perm_invis },  // Permanent Invisibility — CAbilityPermInvis other
    // TODO: { "Apsh", &a_stomp },  // Phase Shift — CAbilityStomp nightelf
    // TODO: { "Aphx", &a_morph },  // Phoenix Morphing (Egg Related) — CAbilityMorph human
    // TODO: { "Apts", &a_button },  // Disease Cloud — CAbilityButton undead
    // TODO: { "Apoi", &a_poison_attack },  // Poison Sting — CAbilityPoisonAttack other
    // TODO: { "Aply", &a_simple_spell },  // Polymorph — CAbilitySimpleSpell human
    // TODO: { "Apos", &a_creep_sleep },  // Possession — CAbilityCreepSleep undead
    // TODO: { "Aps2", &a_creep_sleep },  // Possession — CAbilityCreepSleep undead
    // TODO: { "Awar", &a_bounce },  // Pulverize — CAbilityBounce orc
    // TODO: { "Aprg", &a_lightning_purge },  // Purge — CAbilityLightningPurge orc
    // TODO: { "Arai", &a_simple_spell },  // Raise Dead — CAbilitySimpleSpell undead
    // TODO: { "ARal", &a_rally },  // Rally — CAbilityRally other
    // TODO: { "Arav", &a_raven_form },  // Storm Crow Form — CAbilityRavenForm nightelf
    // TODO: { "ACrn", &a_creep_reincarnation },  // Reincarnation — CAbilityCreepReincarnation creeps
    // TODO: { "Arbr", &a_button },  // Reinforced Burrows Upgrade — CAbilityButton orc
    // TODO: { "Arej", &a_regen_life },  // Rejuvenation — CAbilityRegenLife nightelf
    // TODO: { "Arpb", &a_spell },  // Replenish — CAbilitySpell undead
    // TODO: { "Arpl", &a_spell },  // Essence of Blight — CAbilitySpell undead
    // TODO: { "Arpm", &a_spell },  // Spirit Touch — CAbilitySpell undead
    // TODO: { "Arsk", &a_morph },  // Resistant Skin — CAbilityMorph nightelf
    // TODO: { "AIta", &a_item_town_portal },  // Item Area Detection — CAbilityItemTownPortal human
    // TODO: { "Arng", &a_revenge },  // Revenge — CAbilityRevenge other
    // TODO: { "Arev", &a_revive },  // Revive Hero — CAbilityRevive other
    // TODO: { "Aroc", &a_rain_of_fire },  // Barrage — CAbilityRainOfFire human
    // TODO: { "Asac", &a_spell },  // Sacrifice — CAbilitySpell undead
    // TODO: { "Asal", &a_button },  // Pillage — CAbilityButton orc
    // TODO: { "Alam", &a_spell },  // Sacrifice — CAbilitySpell undead
    // TODO: { "Asds", &a_button },  // Kaboom! — CAbilityButton creeps
    // TODO: { "Asid", &a_button },  // Sell Items — CAbilityButton other
    // TODO: { "Asud", &a_spell },  // Sell Units — CAbilitySpell other
    // TODO: { "Aesn", &a_button },  // Sentinel — CAbilityButton nightelf
    // TODO: { "Aeye", &a_button },  // Sentry Ward — CAbilityButton orc
    // TODO: { "Ashm", &a_shadow_meld },  // Shadow Meld — CAbilityShadowMeld nightelf
    // TODO: { "Ahid", &a_shadow_meld },  // Shadow Meld — CAbilityShadowMeld creeps
    // TODO: { "Asla", &a_sleep_always },  // Sleep Always — CAbilitySleepAlways creeps
    // TODO: { "Aslo", &a_slow },  // Slow — CAbilitySlow human
    // TODO: { "Aspo", &a_poison_attack },  // Slow Poison — CAbilityPoisonAttack nightelf
    // TODO: { "Asod", &a_simple_spell },  // Spawn Skeleton — CAbilitySimpleSpell creeps
    // TODO: { "Assp", &a_simple_spell },  // Spawn Spiderlings — CAbilitySimpleSpell creeps
    // TODO: { "Aspd", &a_simple_spell },  // Spawn Spiders — CAbilitySimpleSpell creeps
    // TODO: { "Asps", &a_creep_sleep },  // Spell Steal — CAbilityCreepSleep human
    // TODO: { "Asph", &a_poison_attack },  // Sphere — CAbilityPoisonAttack human
    // TODO: { "Aspa", &a_attack },  // Spider Attack — CAbilityAttack undead
    // TODO: { "Aspi", &a_spiked },  // Spiked Barricades — CAbilitySpiked orc
    // TODO: { "Aspl", &a_creep_sleep },  // Spirit Link — CAbilityCreepSleep orc
    // TODO: { "Asta", &a_stasis_trap },  // Stasis Trap — CAbilityStasisTrap orc
    // TODO: { "Astn", &a_morph },  // Stone Form — CAbilityMorph undead
    // TODO: { "Asth", &a_bounce },  // Storm Hammers — CAbilityBounce human
    // TODO: { "ANsu", &a_unknown },  // Submerge (Myrmidon) — CAbility creeps
    // TODO: { "Attu", &a_button },  // Turret — CAbilityButton human
    // TODO: { "Atau", &a_spell },  // Taunt — CAbilitySpell nightelf
    // TODO: { "ACtb", &a_creep_thunder_bolt },  // Hurl Boulder — CAbilityCreepThunderBolt creeps
    // TODO: { "ACtc", &a_creep_thunder_clap },  // Slam — CAbilityCreepThunderClap creeps
    // TODO: { "Atdg", &a_stomp },  // Building Damage Aura — CAbilityStomp naga
    // TODO: { "Atsp", &a_aura },  // Tornado Spin — CAbilityAura naga
    // TODO: { "Atwa", &a_war_stomp },  // Tornado Wander — CAbilityWarStomp naga
    // TODO: { "Atol", &a_upgrade },  // Tree of Life upgrade ability — CAbilityUpgrade nightelf
    // TODO: { "Ault", &a_night_vision },  // Ultravision — CAbilityNightVision nightelf
    // TODO: { "Auhf", &a_creep_sleep },  // Unholy Frenzy — CAbilityCreepSleep undead
    // TODO: { "Auco", &a_spell },  // Unstable Concoction — CAbilitySpell orc
    // TODO: { "Auns", &a_button },  // Unsummon Building — CAbilityButton undead
    // TODO: { "AIva", &a_attack_mod },  // Item Life Steal — CAbilityAttackMod creeps
    // TODO: { "Avng", &a_revenge },  // Spirit of Vengeance — CAbilityRevenge nightelf
    // TODO: { "Awan", &a_wander },  // Wander — CAbilityWander other
    // TODO: { "Aven", &a_venom_spear },  // Envenomed Spears — CAbilityVenomSpear orc
    // TODO: { "Awrp", &a_warp },  // Waygate ability — CAbilityWarp other
    // TODO: { "Aweb", &a_auto_target_spell },  // Web — CAbilityAutoTargetSpell undead
    // TODO: { "AIaa", &a_damage_bonus_base },  // Item Permanent Damage Gain — CAbilityDamageBonusBase [ITEM] other
    // TODO: { "AIbl", &a_on_fire },  // Build Tiny Castle — CAbilityOnFire [ITEM] other
    // TODO: { "AIgl", &a_unknown },  // FortificationGlyph — CAbility [ITEM] other
    // TODO: { "AIfl", &a_button },  // Item Capture The Flag — CAbilityButton [ITEM] other
    // TODO: { "AIfm", &a_button },  // Item Capture The Flag — CAbilityButton [ITEM] other
    // TODO: { "AIfo", &a_button },  // Item Capture The Flag — CAbilityButton [ITEM] other
    // TODO: { "AIfn", &a_button },  // Item Capture The Flag — CAbilityButton [ITEM] other
    // TODO: { "AIfe", &a_button },  // Item Capture The Flag — CAbilityButton [ITEM] other
    // TODO: { "AIfa", &a_agility_mod },  // Flare Gun — CAbilityAgilityMod [ITEM] other
    // TODO: { "AIlp", &a_lightning_purge },  // Item Purge — CAbilityLightningPurge [ITEM] other
    // TODO: { "AIms", &a_move_speed_bonus },  // Item Move Speed Bonus — CAbilityMoveSpeedBonus [ITEM] other
    // TODO: { "ANbs", &a_spell },  // Orb of Darkness — CAbilitySpell [ITEM] other
    // TODO: { "AIsb", &a_item_heal },  // Orb of Slow  — CAbilityItemHeal [ITEM] other
    // TODO: { "AIcb", &a_button },  // Item Attack Corruption Bonus — CAbilityButton [ITEM] other
    // TODO: { "AIfb", &a_on_fire },  // Item Attack Fire Bonus — CAbilityOnFire [ITEM] other
    // TODO: { "AIzb", &a_frost_nova },  // Item Freeze Damage Bonus — CAbilityFrostNova [ITEM] other
    // TODO: { "AIob", &a_frost_nova },  // Item Attack Frost Bonus — CAbilityFrostNova [ITEM] other
    // TODO: { "AIlb", &a_bounce },  // Item Attack Lightning Bonus — CAbilityBounce [ITEM] other
    // TODO: { "AIpb", &a_item_mana_restore },  // Item Attack Poison Bonus — CAbilityItemManaRestore [ITEM] other
    // TODO: { "Apo2", &a_item_invul },  // Poison Sting — CAbilityItemInvul [ITEM] other
    // TODO: { "Arel", &a_aura_regen_life },  // Item Life Regeneration — CAbilityAuraRegenLife [ITEM] other
    // TODO: { "AIsi", &a_sight_bonus },  // Item Sight Range Bonus — CAbilitySightBonus [ITEM] other
    // TODO: { "AIso", &a_simple_spell },  // Item Soul Theft — CAbilitySimpleSpell [ITEM] other
    // TODO: { "Asou", &a_simple_spell },  // Item Soul Possession — CAbilitySimpleSpell [ITEM] other
    // TODO: { "AIcf", &a_immolation },  // Item Immolation — CAbilityImmolation [ITEM] other
    // TODO: { "AIdm", &a_bounce },  // Item Area tree/wall damage — CAbilityBounce [ITEM] other
    // TODO: { "AIdi", &a_item_dispel_aoe },  // Item Dispel — CAbilityItemDispelAoe [ITEM] other
    // TODO: { "AIha", &a_item_heal_aoe },  // Item Area Healing — CAbilityItemHealAoe [ITEM] other
    // TODO: { "AIil", &a_item_illusion },  // Item Illusions — CAbilityItemIllusion [ITEM] other
    // TODO: { "AIvi", &a_unknown },  // Item Temporary Invisibility — CAbility [ITEM] other
    // TODO: { "AIvu", &a_item_invul },  // Item Temporary Invulnerability — CAbilityItemInvul [ITEM] other
    // TODO: { "AImr", &a_item_mana_restore_aoe },  // Item Area Mana Regain — CAbilityItemManaRestoreAoe [ITEM] other
    // TODO: { "AIpm", &a_button },  // Item Place Goblin Land Mine — CAbilityButton [ITEM] other
    // TODO: { "AIrt", &a_item_recall },  // Item Recall — CAbilityItemRecall [ITEM] other
    // TODO: { "AIrm", &a_item_regen_mana },  // Item Mana Regeneration — CAbilityItemRegenMana [ITEM] other
    // TODO: { "AIrc", &a_item_reincarnation },  // Item Reincarnation — CAbilityItemReincarnation [ITEM] other
    // TODO: { "AIre", &a_item_restore },  // Item Heal/Mana Regain — CAbilityItemRestore [ITEM] other
    // TODO: { "AIra", &a_item_restore_aoe },  // Item Area Heal/Mana Regain — CAbilityItemRestoreAoe [ITEM] other
    // TODO: { "AIsp", &a_item_speed },  // Item Temporary Speed Bonus — CAbilityItemSpeed [ITEM] other
    // TODO: { "AIsa", &a_item_speed },  // Scroll of Haste — CAbilityItemSpeed [ITEM] other
    // TODO: { "AItp", &a_item_town_portal },  // Item Town Portal — CAbilityItemTownPortal [ITEM] other
    // TODO: { "Aami", &a_unknown },  // Item Anti-Magic Shell — CAbility [ITEM] other
    // TODO: { "AIan", &a_simple_spell },  // Item Animate Dead — CAbilitySimpleSpell [ITEM] other
    // TODO: { "AIrs", &a_item_reincarnation },  // Item Resurrection — CAbilityItemReincarnation [ITEM] other
    // TODO: { "AIas", &a_unknown },  // Item Attack Speed Bonus — CAbility [ITEM] other
    // TODO: { "AIrg", &a_unknown },  // Potion of Life Regen — CAbility [ITEM] other
    // TODO: { "AIgo", &a_attack_mod },  // Chest of Gold — CAbilityAttackMod [ITEM] other
    // TODO: { "AIlu", &a_item_heal_aoe },  // Bundle of Lumber — CAbilityItemHealAoe [ITEM] other
    // TODO: { "AIrv", &a_item_heal_aoe },  // Item Reveal Entire Map — CAbilityItemHealAoe [ITEM] other
    // TODO: { "AIwb", &a_button },  // Item Web — CAbilityButton [ITEM] other
    // TODO: { "AImo", &a_item_mana_restore_aoe },  // Monster Lure — CAbilityItemManaRestoreAoe [ITEM] other
    // TODO: { "AIri", &a_item_speed },  // Random Item — CAbilityItemSpeed [ITEM] other
    // TODO: { "AIsr", &a_item_speed },  // Spell Damage Reduction — CAbilityItemSpeed [ITEM] other
    // TODO: { "Ablp", &a_item_heal },  // Blight Placement — CAbilityItemHeal [ITEM] other
    // TODO: { "AIpv", &a_item_mana_restore_aoe },  // Vampiric Potion — CAbilityItemManaRestoreAoe [ITEM] other
    // TODO: { "Aste", &a_figurine_rock_golem },  // Steal — CAbilityFigurineRockGolem [ITEM] other
    // TODO: { "Amec", &a_button },  // Mechanical Critter — CAbilityButton [ITEM] other
    // TODO: { "Ashs", &a_spell },  // Wand of Shadowsight — CAbilitySpell [ITEM] other
    // TODO: { "ANpr", &a_button },  // Staff of Preservation — CAbilityButton [ITEM] other
    // TODO: { "ANsa", &a_bounce },  // Staff of Sanctuary — CAbilityBounce [ITEM] other
    // TODO: { "ANss", &a_bounce },  // Spell Shield — CAbilityBounce [ITEM] other
    // TODO: { "ANse", &a_spell },  // Spell Shield — CAbilitySpell [ITEM] other
    // TODO: { "Aret", &a_revive },  // Tome of Retraining — CAbilityRevive [ITEM] other
    // TODO: { "Aspb", &a_bounce },  // Spell Book — CAbilityBounce [ITEM] other
    // TODO: { "AIrd", &a_item_dispel_aoe },  // Raise Dead (Item) — CAbilityItemDispelAoe [ITEM] other
    // TODO: { "AItb", &a_button },  // Dust of Appearance — CAbilityButton [ITEM] other
    // TODO: { "AIrb", &a_item_heal_aoe },  // Rebirth — CAbilityItemHealAoe [ITEM] other
    // TODO: { "AUds", &a_mass_teleport },  // Dark Summoning — CAbilityMassTeleport [ITEM] other
    // TODO: { "AIsh", &a_item_town_portal },  // Summon Headhunter — CAbilityItemTownPortal [ITEM] other
};

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
    FOR_LOOP(i, game.num_abilities) {
        abilityitem_t *abil = &abilitylist[i];
        if (abil->ability->init) {
            abil->ability->init(abil->classname, abil->ability);
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
