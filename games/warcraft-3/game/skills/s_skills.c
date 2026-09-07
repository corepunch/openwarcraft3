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

    /* BEGIN GENERATED ABILITY STRINGS */

    /* CampaignAbilityStrings.txt */
    { "Aamk", &a_attribute_bonus },  /* Attribute Bonus */
    { "ACtn", &a_spawn_tentacle },  /* Spawn Tentacle */
    { "ACs7", &a_feral_spirit_campaign },  /* Feral Spirit */
    { "ANav", &a_avatar_campaign },  /* Avatar */
    { "ANsh", &a_shockwave_campaign },  /* Shockwave */
    { "ACs8", &a_spirit_beast },  /* Spirit Beast */
    { "ANr2", &a_reincarnation_campaign },  /* Reincarnation */
    { "Afbb", &a_feedback },  /* Feedback */
    { "Andm", &a_abolish_magic },  /* Abolish Magic */
    { "Asb1", &a_submerge_myrmidon },  /* Submerge */
    { "Asb2", &a_submerge_royal_guard },  /* Submerge */
    { "Asb3", &a_submerge_snap_dragon },  /* Submerge */
    { "ANha", &a_harvest },  /* Harvest */
    { "ANen", &a_ensnare },  /* Ensnare */
    { "ACfu", &a_frost_armor_campaign },  /* Frost Armor */
    { "ANpa", &a_parasite },  /* Parasite */
    { "Acny", &a_cyclone_campaign },  /* Cyclone */
    { "Ahnl", &a_summoning_ritual },  /* Summoning Ritual */
    { "ANcl", &a_channel_test },  /* Channel */
    { "Arsq", &a_summon_quilbeast_campaign },  /* Summon Quilbeast */
    { "Arsg", &a_summon_misha },  /* Summon Misha */
    { "Arsp", &a_stampede_campaign },  /* Stampede */
    { "ANbr", &a_battle_roar },  /* Battle Roar */
    { "ANsb", &a_storm_bolt_campaign },  /* Storm Bolt */
    { "ANcf", &a_breath_of_fire_campaign },  /* Breath of Fire */
    { "Acdh", &a_drunken_haze_campaign },  /* Drunken Haze */
    { "Acef", &a_storm_earth_fire },  /* "Storm, Earth, And Fire" */
    { "ANhw", &a_healing_wave_campaign },  /* Healing Wave */
    { "ANhx", &a_hex_campaign },  /* Hex */
    { "Arsw", &a_serpent_ward },  /* Serpent Ward */
    { "AOr2", &a_endurance_aura_campaign },  /* Endurance Aura */
    { "AOs2", &a_shockwave_cairne },  /* Shockwave */
    { "AOr3", &a_reincarnation_cairne },  /* Reincarnation */
    { "AOw2", &a_war_stomp_campaign },  /* War Stomp */
    { "AOls", &a_voodoo_spirits },  /* Voodoo Spirits */

    /* CommonAbilityStrings.txt */
    { "Aall", &a_shop_sharing },  /* Shop Sharing, Allied Bldg. */
    { "Abdt", &a_burrow_detection },  /* Burrow Detection */
    { "Apit", &a_shop_purchase_item },  /* Shop Purchase Item */
    { "Ahar", &a_harvest },  /* Harvest */
    { "Ahrl", &a_harvest_lumber },  /* Harvest */
    { "Arev", &a_revive_hero },  /* Revive Hero */
    { "Aawa", &a_revive_hero_instant },  /* Revive Hero Instantly */
    { "Adet", &a_detector },  /* Detector */
    { "Arep", &a_repair },  /* Repair */
    { "AEpa", &a_poison_arrows },  /* Poison Arrows */
    { "AEbu", &a_build },  /* Build (Night Elf) */
    { "AGbu", &a_build },  /* Build (Naga) */
    { "AHbu", &a_build },  /* Build (Human) */
    { "AHer", &a_hero },  /* Hero */
    { "ANbu", &a_build },  /* Build (Neutral) */
    { "AObu", &a_build },  /* Build (Orc) */
    { "ARal", &a_rally },  /* Rally */
    { "AUbu", &a_build },  /* Build (Undead) */
    { "Aalr", &a_alarm },  /* Alarm */
    { "Aatk", &a_attack },  /* Attack */
    { "Afih", &a_on_fire },  /* On Fire (Human) */
    { "Afin", &a_on_fire },  /* On Fire (Night Elf) */
    { "Afio", &a_on_fire },  /* On Fire (Orc) */
    { "Afir", &a_on_fire },  /* On Fire */
    { "Afiu", &a_on_fire },  /* On Fire (Undead) */
    { "Aloc", &a_locust },  /* Locust */
    { "Amov", &a_move },  /* Move */
    { "Atdp", &a_drop },  /* Drop Pilot */
    { "Atlp", &a_load },  /* Load Pilot */
    { "Attu", &a_turret },  /* Turret */

    /* HumanAbilityStrings.txt */
    { "AHdr", &a_siphon_mana_human },  /* Siphon Mana */
    { "AHfs", &a_flame_strike_human },  /* Flame Strike */
    { "AHbn", &a_banish },  /* Banish */
    { "AHpx", &a_phoenix },  /* Phoenix */
    { "Apxf", &a_phoenix_fire },  /* Phoenix Fire */
    { "AHbz", &a_blizzard },  /* Blizzard */
    { "AHwe", &a_water_elemental },  /* Summon Water Elemental */
    { "AHab", &a_brilliance_aura },  /* Brilliance Aura */
    { "AHmt", &a_mass_teleport },  /* Mass Teleport */
    { "AHtb", &a_thunderbolt },  /* Storm Bolt */
    { "AHtc", &a_thunder_clap },  /* Thunder Clap */
    { "AHbh", &a_bash },  /* Bash */
    { "AHhb", &a_holylight },  /* Holy Light */
    { "AHds", &a_divine_shield },  /* Divine Shield */
    { "AHad", &a_devotionaura },  /* Devotion Aura */
    { "AHre", &a_revive },  /* Resurrection */
    { "Amil", &a_militia },  /* Call to Arms */
    { "Amic", &a_call_to_arms },  /* Call To Arms */

    /* ItemAbilityStrings.txt */
    { "AIsm", &a_item_permanent_stat_gain },  /* Item Strength Gain */
    { "AIam", &a_item_permanent_stat_gain },  /* Item Agility Gain */
    { "AIat", &a_item_attack_bonus },  /* Item Damage Bonus */
    { "AIde", &a_item_defense_bonus },  /* Item Armor Bonus */
    { "AIem", &a_item_experience_gain },  /* Item Experience Gain */
    { "AIlm", &a_item_level_gain },  /* Item Level Gain */
    { "AIim", &a_item_permanent_stat_gain },  /* Item Intelligence Gain */
    { "AIxm", &a_item_permanent_stat_gain },  /* Item Int/Agi/Str gain */
    { "AIhe", &a_item_heal },  /* Item Healing */
    { "AIma", &a_item_mana_regain },  /* Item Mana Regain */
    { "AIda", &a_item_defense_aoe },  /* Item Temporary Area Armor Bonus */
    { "AIco", &a_charm },  /* Item Command */
    { "AIfs", &a_item_figurine_summon },  /* Item Skeleton Summon */
    { "AImi", &a_item_permanent_life_gain },  /* Item Permanent Life Gain */
    { "AIab", &a_item_stat_bonus },  /* Item Hero Stat Bonus */
    { "AIml", &a_item_life_bonus },  /* Item Life Bonus */
    { "AImm", &a_item_mana_bonus },  /* Item Mana Bonus */
    { "AIct", &a_item_change_time },  /* Change Time of Day */

    /* NeutralAbilityStrings.txt */
    { "ANab", &a_acid_bomb },  /* Acid Bomb */
    { "ANms", &a_mana_shield },  /* Mana Shield */
    { "ANrf", &a_rain_of_fire },  /* Rain of Fire */
    { "AHca", &a_cold_arrows },  /* Cold Arrows */
    { "ANht", &a_howl_of_terror },  /* Howl of Terror */
    { "ANca", &a_cleaving_attack },  /* Cleaving Attack */
    { "ANdo", &a_doom },  /* Doom */
    { "ANdr", &a_siphon_mana },  /* Life Drain */
    { "ANbf", &a_breath_of_fire },  /* Breath of Fire */
    { "ANsg", &a_summon_bear },  /* Summon Bear */
    { "ANsq", &a_summon_quilbeast },  /* Summon Quilbeast */
    { "ANsw", &a_summon_hawk },  /* Summon Hawk */
    { "ANst", &a_stomp },  /* Stampede */
    { "ANfs", &a_flame_strike },  /* Flame Strike */
    { "AInv", &a_inventory },  /* Inventory */
    { "ANdb", &a_drunken_brawler },  /* Drunken Brawler */
    { "ANdh", &a_drunken_haze },  /* Drunken Haze */
    { "ANsi", &a_silence },  /* Silence */
    { "ANba", &a_black_arrow },  /* Black Arrow */
    { "ANch", &a_charm },  /* Charm */
    { "ANto", &a_tornado },  /* Tornado */
    { "Abgm", &a_blighted_goldmine },  /* Blighted Gold Mine Ability */
    { "Aegm", &a_entangled_mine },  /* Entangled Gold Mine Ability */
    { "Aloa", &a_load },  /* Load */
    { "Adro", &a_drop },  /* Unload */
    { "Adri", &a_drop_instant },  /* Unload Instant */
    { "Abun", &a_cargo_hold_burrow },  /* Cargo Hold (Orc Burrow) */
    { "Acar", &a_cargo_hold },  /* Cargo Hold */
    { "Aneu", &a_neutral_building },  /* Select Hero */
    { "ANfb", &a_firebolt },  /* Firebolt */
    { "Agld", &a_goldmine },  /* Gold Mine ability */
    { "Artn", &a_return_resources },  /* Return */
    { "Avul", &a_invulnerable },  /* Invulnerable */
    { "Abli", &a_blight },  /* Blight */
    { "ANfl", &a_forked_lightning },  /* Forked Lightning */

    /* NightElfAbilityStrings.txt */
    { "AEbl", &a_blink },  /* Blink */
    { "AEfk", &a_fan_of_knives },  /* Fan of Knives */
    { "AEsh", &a_shadow_strike },  /* Shadow Strike */
    { "AEsv", &a_vengeance },  /* Vengeance */
    { "Aeat", &a_eat_tree },  /* Eat Tree */
    { "Ambt", &a_moon_well },  /* Replenish Mana and Life */
    { "Awha", &a_wisp_harvest },  /* Gather */
    { "Aent", &a_entangle_goldmine },  /* Entangle Gold Mine */
    { "Aenc", &a_cargo_hold_entangled_mine },  /* Load */
    { "Aroo", &a_root },  /* Root */
    { "AEmb", &a_mana_burn },  /* Mana Burn */
    { "AEim", &a_immolation },  /* Immolation */
    { "AEev", &a_evasion },  /* Evasion */
    { "AEme", &a_metamorphosis },  /* Metamorphosis */
    { "AEer", &a_entangling_roots },  /* Entangling Roots */
    { "AEfn", &a_force_of_nature },  /* Force of Nature */
    { "AEah", &a_aura_spell },  /* Thorns Aura */
    { "AEtq", &a_tranquility },  /* Tranquility */
    { "AHfa", &a_searing_arrows },  /* Searing Arrows */
    { "AEar", &a_trueshot_aura },  /* Trueshot Aura */
    { "AEsf", &a_starfall },  /* Starfall */
    { "Aren", &a_repair_generic },  /* Renew */

    /* OrcAbilityStrings.txt */
    { "AOhw", &a_healing_wave },  /* Healing Wave */
    { "AOhx", &a_hex },  /* Hex */
    { "AOvd", &a_big_bad_voodoo },  /* Big Bad Voodoo */
    { "Astd", &a_stand_down },  /* Stand Down */
    { "AOwk", &a_wind_walk },  /* Wind Walk */
    { "AOmi", &a_mirror_image },  /* Mirror Image */
    { "AOcr", &a_critical_strike },  /* Critical Strike */
    { "AOww", &a_whirlwind },  /* Bladestorm */
    { "AOcl", &a_chain_lightning },  /* Chain Lightning */
    { "AOfs", &a_far_sight },  /* Far Sight */
    { "AOsf", &a_feral_spirit },  /* Feral Spirit */
    { "AOeq", &a_earthquake },  /* Earthquake */
    { "AOsh", &a_shockwave },  /* Shockwave */
    { "AOae", &a_aura_endurance },  /* Endurance Aura */
    { "AOre", &a_reincarnation },  /* Reincarnation */
    { "AOws", &a_war_stomp },  /* War Stomp */

    /* UndeadAbilityStrings.txt */
    { "AUim", &a_impale },  /* Impale */
    { "AUts", &a_spiked_carapace },  /* Spiked Carapace */
    { "AUcb", &a_carrion_beetles },  /* Carrion Beetles */
    { "AUls", &a_locust_swarm },  /* Locust Swarm */
    { "Aaha", &a_acolyte_harvest },  /* Gather */
    { "AUdc", &a_death_coil },  /* Death Coil */
    { "AUau", &a_unholy_aura },  /* Unholy Aura */
    { "AUdp", &a_death_pact },  /* Death Pact */
    { "AUan", &a_animate_dead },  /* Animate Dead */
    { "AUcs", &a_carrion_swarm },  /* Carrion Swarm */
    { "AUsl", &a_sleep },  /* Sleep */
    { "AUav", &a_vampiric_aura },  /* Vampiric Aura */
    { "AUfn", &a_frost_nova },  /* Frost Nova */
    { "AUfa", &a_frost_armor },  /* Frost Armor */
    { "AUfu", &a_frost_armor_variant },  /* Frost Armor */
    { "AUdr", &a_dark_ritual },  /* Dark Ritual */
    { "AUdd", &a_death_and_decay },  /* Death And Decay */
    { "Arst", &a_repair_generic },  /* Restore */
    { "AUin", &a_inferno },  /* Inferno */

    /* No AbilityStrings source file */
    { "Acoi", &a_couple_instant },  /* Couple Instant */
    { "Agl2", &a_goldmine_overlayed },  /* Gold Mine ability */

    /* END GENERATED ABILITY STRINGS */
    /* BEGIN GENERATED TODO ABILITIES */

    /* CampaignAbilityStrings.txt */
    // TODO: { "Aamk", &a_attack_mod },  /* Attribute Bonus */
    // TODO: { "ANpa", &a_poison_attack },  /* Parasite */
    // TODO: { "ANbr", &a_bash },  /* Battle Roar */

    /* HumanAbilityStrings.txt */
    // TODO: { "Amls", &a_aura },  /* Aerial Shackles */
    // TODO: { "Afbk", &a_aura },  /* Feedback */
    // TODO: { "Acmg", &a_spell },  /* Control Magic */
    // TODO: { "Aflk", &a_button },  /* Flak Cannons */
    // TODO: { "Afsh", &a_button },  /* Fragmentation Shards */
    // TODO: { "Aroc", &a_rain_of_fire },  /* Barrage */
    // TODO: { "Amdf", &a_aura },  /* Magic Defense */
    // TODO: { "Asph", &a_poison_attack },  /* Sphere */
    // TODO: { "Asps", &a_creep_sleep },  /* Spell Steal */
    // TODO: { "Aclf", &a_bounce },  /* Cloud */
    // TODO: { "Aphx", &a_morph },  /* Phoenix Morphing (Egg Related) */
    // TODO: { "Agyb", &a_bounce },  /* Flying Machine Bombs */
    // TODO: { "Asth", &a_bounce },  /* Storm Hammers */
    // TODO: { "Agyv", &a_true_sight },  /* True Sight */
    // TODO: { "Adef", &a_defend },  /* Defend */
    // TODO: { "Afla", &a_button },  /* Flare */
    // TODO: { "Adts", &a_magic_sentry },  /* Magic Sentry */
    // TODO: { "Ainf", &a_inner_fire },  /* Inner Fire */
    // TODO: { "Adis", &a_dispel_magic },  /* Dispel Magic */
    // TODO: { "Ahea", &a_heal },  /* Heal */
    // TODO: { "Aslo", &a_slow },  /* Slow */
    // TODO: { "Aivs", &a_perm_invis },  /* Invisibility */
    // TODO: { "Aply", &a_simple_spell },  /* Polymorph */
    // TODO: { "AHav", &a_attribute_mod },  /* Avatar */

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
    // TODO: { "AIfb", &a_on_fire },  /* Item Attack Fire Bonus */
    // TODO: { "AIlb", &a_bounce },  /* Item Attack Lightning Bonus */
    // TODO: { "AIlp", &a_lightning_purge },  /* Item Purge */
    // TODO: { "AIob", &a_frost_nova },  /* Item Attack Frost Bonus */
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
    // TODO: { "AIcf", &a_immolation },  /* Item Immolation */
    // TODO: { "AIzb", &a_frost_nova },  /* Item Freeze Damage Bonus */
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
    // TODO: { "AIdc", &a_item_defense_aoe },  /* Item Chain Dispel */
    // TODO: { "AIwb", &a_button },  /* Item Web */
    // TODO: { "AImo", &a_item_mana_restore_aoe },  /* Monster Lure */
    // TODO: { "AIri", &a_item_speed },  /* Random Item */
    // TODO: { "Ablp", &a_item_heal },  /* Blight Placement */
    // TODO: { "Aste", &a_figurine_rock_golem },  /* Steal */
    // TODO: { "AIpv", &a_item_mana_restore_aoe },  /* Vampiric Potion */
    // TODO: { "AIsr", &a_item_speed },  /* Spell Damage Reduction */
    // TODO: { "AIbl", &a_on_fire },  /* Build Tiny Castle */
    // TODO: { "Ashs", &a_spell },  /* Wand of Shadowsight */
    // TODO: { "Aret", &a_revive },  /* Tome of Retraining */
    // TODO: { "ANpr", &a_button },  /* Staff of Preservation */
    // TODO: { "Amec", &a_button },  /* Mechanical Critter */
    // TODO: { "ANss", &a_bounce },  /* Spell Shield */
    // TODO: { "ANse", &a_spell },  /* Spell Shield */
    // TODO: { "Aspb", &a_bounce },  /* Spell Book */
    // TODO: { "AIrd", &a_item_dispel_aoe },  /* Raise Dead (Item) */
    // TODO: { "ANsa", &a_bounce },  /* Staff of Sanctuary */
    // TODO: { "AIsa", &a_item_speed },  /* Scroll of Haste */
    // TODO: { "AItb", &a_button },  /* Dust of Appearance */
    // TODO: { "AIsb", &a_item_heal },  /* Orb of Slow */
    // TODO: { "ANbs", &a_spell },  /* Orb of Darkness */
    // TODO: { "AIrb", &a_item_heal_aoe },  /* Rebirth */
    // TODO: { "AUds", &a_mass_teleport },  /* Dark Summoning */
    // TODO: { "AIdd", &a_item_heal },  /* Defend */
    // TODO: { "AIsh", &a_item_town_portal },  /* Summon Headhunter */

    /* NeutralAbilityStrings.txt */
    // TODO: { "ANic", &a_inner_fire },  /* Incinerate */
    // TODO: { "ANia", &a_inner_fire },  /* Incinerate */
    // TODO: { "ANso", &a_spell },  /* Soul Burn */
    // TODO: { "ANlm", &a_lightning_shield },  /* Summon Lava Spawn */
    // TODO: { "ANvc", &a_spell },  /* Volcano */
    // TODO: { "ANsy", &a_button },  /* Pocket Factory */
    // TODO: { "ANcs", &a_spell },  /* Cluster Rockets */
    // TODO: { "ANeg", &a_evasion },  /* Engineering Upgrade */
    // TODO: { "ANrg", &a_regen_base },  /* Robo-Goblin */
    // TODO: { "ANde", &a_button },  /* Demolish */
    // TODO: { "ANfy", &a_on_fire },  /* Factory */
    // TODO: { "ANhs", &a_heal },  /* Healing Spray */
    // TODO: { "ANcr", &a_critical_strike },  /* Chemical Rage */
    // TODO: { "ANtm", &a_aura_regen_life },  /* Transmute */
    // TODO: { "Aasl", &a_slow },  /* Slow Aura */
    // TODO: { "Atdg", &a_stomp },  /* Building Damage Aura */
    // TODO: { "Atsp", &a_aura },  /* Tornado Spin */
    // TODO: { "Atwa", &a_war_stomp },  /* Tornado Wander */
    // TODO: { "ANef", &a_button },  /* "Storm, Earth, And Fire" */
    // TODO: { "ACbf", &a_on_fire },  /* Breath of Frost */
    // TODO: { "ANmr", &a_aura },  /* Mind Rot */
    // TODO: { "ANmo", &a_bounce },  /* Monsoon */
    // TODO: { "ANwm", &a_war_stomp },  /* Watery Minion */
    // TODO: { "Arng", &a_revenge },  /* Revenge */
    // TODO: { "Atol", &a_upgrade },  /* Tree of Life upgrade ability */
    // TODO: { "Awrp", &a_warp },  /* Waygate ability */
    // TODO: { "ANdc", &a_spell },  /* Dark Conversion */
    // TODO: { "ANsl", &a_spell },  /* Soul Preservation */
    // TODO: { "ANfd", &a_spell },  /* Finger of Death */
    // TODO: { "ANdp", &a_mass_teleport },  /* Dark Portal */
    // TODO: { "ANrc", &a_rain_of_fire },  /* Rain of Chaos */
    // TODO: { "Achd", &a_cargo_hold },  /* Cargo Hold Death */
    // TODO: { "Asla", &a_sleep_always },  /* Sleep Always */
    // TODO: { "Advc", &a_devour_cargo },  /* Devour Cargo */
    // TODO: { "ANpi", &a_perm_immolation },  /* Permanent Immolation */
    // TODO: { "Apig", &a_harvest_return },  /* Permanent Immolation */
    // TODO: { "AAns", &a_neutral_spell },  /* Charge Gold and Lumber */
    // TODO: { "Andt", &a_evil_eye },  /* Reveal */
    // TODO: { "ANin", &a_creep_thunder_bolt },  /* Inferno */
    // TODO: { "Asds", &a_button },  /* Kaboom! */
    // TODO: { "Anhe", &a_heal },  /* Heal */
    // TODO: { "ACtc", &a_creep_thunder_clap },  /* Slam */
    // TODO: { "ACtb", &a_creep_thunder_bolt },  /* Hurl Boulder */
    // TODO: { "Afzy", &a_spell },  /* Frenzy */
    // TODO: { "ACdv", &a_creep_devour },  /* Devour */
    // TODO: { "ACsp", &a_creep_sleep },  /* Sleep */
    // TODO: { "Asod", &a_simple_spell },  /* Spawn Skeleton */
    // TODO: { "Assp", &a_simple_spell },  /* Spawn Spiderlings */
    // TODO: { "Aspd", &a_simple_spell },  /* Spawn Spiders */
    // TODO: { "AOac", &a_unknown },  /* Command Aura */
    // TODO: { "ACad", &a_revenge },  /* Animate Dead */
    // TODO: { "ACrn", &a_creep_reincarnation },  /* Reincarnation */
    // TODO: { "Adda", &a_on_fire },  /* AOE damage upon death */
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
    // TODO: { "Amfl", &a_frost_nova },  /* Mana Flare */
    // TODO: { "Apsh", &a_stomp },  /* Phase Shift */
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
    // TODO: { "Arav", &a_raven_form },  /* Storm Crow Form */
    // TODO: { "Aadm", &a_auto_dispel_magic },  /* Abolish Magic */
    // TODO: { "Amim", &a_magic_immunity },  /* Spell Immunity */
    // TODO: { "Ault", &a_night_vision },  /* Ultravision */
    // TODO: { "Acoa", &a_morph },  /* Mount Hippogryph */
    // TODO: { "Acoh", &a_morph },  /* Pick up Archer */
    // TODO: { "Adec", &a_defend },  /* Dismount */
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
    // TODO: { "Aliq", &a_on_fire },  /* Liquid Fire */
    // TODO: { "Auco", &a_spell },  /* Unstable Concoction */
    // TODO: { "Acha", &a_unknown },  /* Chaos */
    // TODO: { "Achl", &a_cargo_load },  /* Chaos Cargo Load */
    // TODO: { "Awar", &a_bounce },  /* Pulverize */
    // TODO: { "Abtl", &a_battlestations },  /* Battle Stations */
    // TODO: { "Aens", &a_ensnare },  /* Ensnare */
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
    // TODO: { "Aave", &a_stomp },  /* Destroyer Form */
    // TODO: { "Afak", &a_spell },  /* Orb of Annihilation */
    // TODO: { "Advm", &a_dispel_magic },  /* Devour Magic */
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
    // TODO: { "Aspa", &a_attack },  /* Spider Attack */
    // TODO: { "Aweb", &a_auto_target_spell },  /* Web */
    // TODO: { "Astn", &a_morph },  /* Stone Form */
    // TODO: { "Amel", &a_cargo_load },  /* Get Corpse */
    // TODO: { "Amed", &a_cargo_drop },  /* Drop Corpse */
    // TODO: { "Aapl", &a_unknown },  /* Disease Cloud */
    // TODO: { "Apts", &a_button },  /* Disease Cloud */
    // TODO: { "Afrb", &a_button },  /* Frost Breath */
    // TODO: { "Afra", &a_button },  /* Frost Attack */
    // TODO: { "Afrz", &a_frost_nova },  /* Freezing Breath */
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

    /* END GENERATED TODO ABILITIES */
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
