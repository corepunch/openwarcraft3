#ifndef g_unitdata_h
#define g_unitdata_h

#include "g_local.h"

#define UNIT_DEFENSE_TYPE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "udty")
#define UNIT_ARMOR_TYPE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uarm")
#define UNIT_LOOPING_FADE_IN_RATE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulfi")
#define UNIT_LOOPING_FADE_OUT_RATE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulfo")
#define UNIT_AGILITY(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uagc")
#define UNIT_INTELLIGENCE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uinc")
#define UNIT_STRENGTH(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ustc")
#define UNIT_AGILITY_PERMANENT(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uagm")
#define UNIT_INTELLIGENCE_PERMANENT(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uinm")
#define UNIT_STRENGTH_PERMANENT(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ustm")
#define UNIT_AGILITY_WITH_BONUS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uagb")
#define UNIT_INTELLIGENCE_WITH_BONUS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uinb")
#define UNIT_STRENGTH_WITH_BONUS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ustb")
#define UNIT_GOLD_BOUNTY_AWARDED_NUMBER_OF_DICE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ubdi")
#define UNIT_GOLD_BOUNTY_AWARDED_BASE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ubba")
#define UNIT_GOLD_BOUNTY_AWARDED_SIDES_PER_DIE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ubsi")
#define UNIT_LUMBER_BOUNTY_AWARDED_NUMBER_OF_DICE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulbd")
#define UNIT_LUMBER_BOUNTY_AWARDED_BASE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulba")
#define UNIT_LUMBER_BOUNTY_AWARDED_SIDES_PER_DIE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulbs")
#define UNIT_LEVEL(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulev")
#define UNIT_COLLISION(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ucol")
#define UNIT_FORMATION_RANK(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ufor")
#define UNIT_ORIENTATION_INTERPOLATION(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uori")
#define UNIT_ELEVATION_SAMPLE_POINTS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uept")
#define UNIT_TINTING_COLOR_RED(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uclr")
#define UNIT_TINTING_COLOR_GREEN(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uclg")
#define UNIT_TINTING_COLOR_BLUE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uclb")
#define UNIT_TINTING_COLOR_ALPHA(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ucal")
#define UNIT_MOVE_TYPE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "umvt")
#define UNIT_TARGETED_AS(UNIT) UnitStringField(UnitsMetaData, UNIT, "utar")
#define UNIT_BUILD_TIME(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ubld")
#define UNIT_GOLD_COST(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ugol")
#define UNIT_LUMBER_COST(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ulum")
#define UNIT_FOOD_USED(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ufoo")
#define UNIT_FOOD_MADE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ufma")
#define UNIT_UNIT_CLASSIFICATION(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "utyp")
#define UNIT_HIT_POINTS_REGENERATION_TYPE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "uhrt")
#define UNIT_PLACEMENT_PREVENTED_BY(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "upar")
#define UNIT_PRIMARY_ATTRIBUTE(UNIT) UnitStringField(UnitsMetaData, UNIT, "upra")
#define UNIT_STRENGTH_PER_LEVEL(UNIT) UnitRealField(UnitsMetaData, UNIT, "ustp")
#define UNIT_AGILITY_PER_LEVEL(UNIT) UnitRealField(UnitsMetaData, UNIT, "uagp")
#define UNIT_INTELLIGENCE_PER_LEVEL(UNIT) UnitRealField(UnitsMetaData, UNIT, "uinp")
#define UNIT_HIT_POINTS_REGENERATION_RATE(UNIT) UnitRealField(UnitsMetaData, UNIT, "uhpr")
#define UNIT_MANA_REGENERATION(UNIT) UnitRealField(UnitsMetaData, UNIT, "umpr")
#define UNIT_DEATH_TIME(UNIT) UnitRealField(UnitsMetaData, UNIT, "udtm")
#define UNIT_FLY_HEIGHT(UNIT) UnitRealField(UnitsMetaData, UNIT, "ufyh")
#define UNIT_TURN_RATE(UNIT) UnitRealField(UnitsMetaData, UNIT, "umvr")
#define UNIT_ELEVATION_SAMPLE_RADIUS(UNIT) UnitRealField(UnitsMetaData, UNIT, "uerd")
#define UNIT_FOG_OF_WAR_SAMPLE_RADIUS(UNIT) UnitRealField(UnitsMetaData, UNIT, "ufrd")
#define UNIT_MAXIMUM_PITCH_ANGLE_DEGREES(UNIT) UnitRealField(UnitsMetaData, UNIT, "umxp")
#define UNIT_MAXIMUM_ROLL_ANGLE_DEGREES(UNIT) UnitRealField(UnitsMetaData, UNIT, "umxr")
#define UNIT_SCALING_VALUE(UNIT) UnitRealField(UnitsMetaData, UNIT, "usca")
#define UNIT_ANIMATION_RUN_SPEED(UNIT) UnitRealField(UnitsMetaData, UNIT, "urun")
#define UNIT_SELECTION_SCALE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ussc")
#define UNIT_SELECTION_CIRCLE_HEIGHT(UNIT) UnitRealField(UnitsMetaData, UNIT, "uslz")
#define UNIT_SHADOW_IMAGE_HEIGHT(UNIT) UnitRealField(UnitsMetaData, UNIT, "ushh")
#define UNIT_SHADOW_IMAGE_WIDTH(UNIT) UnitRealField(UnitsMetaData, UNIT, "ushw")
#define UNIT_SHADOW_IMAGE_CENTER_X(UNIT) UnitRealField(UnitsMetaData, UNIT, "ushx")
#define UNIT_SHADOW_IMAGE_CENTER_Y(UNIT) UnitRealField(UnitsMetaData, UNIT, "ushy")
#define UNIT_ANIMATION_WALK_SPEED(UNIT) UnitRealField(UnitsMetaData, UNIT, "uwal")
#define UNIT_DEFENSE(UNIT) UnitRealField(UnitsMetaData, UNIT, "udfc")
#define UNIT_SIGHT_RADIUS(UNIT) UnitRealField(UnitsMetaData, UNIT, "usir")
#define UNIT_PRIORITY(UNIT) UnitRealField(UnitsMetaData, UNIT, "upri")
#define UNIT_SPEED(UNIT) UnitRealField(UnitsMetaData, UNIT, "umvs")
#define UNIT_OCCLUDER_HEIGHT(UNIT) UnitRealField(UnitsMetaData, UNIT, "uocc")
#define UNIT_HP(UNIT) UnitRealField(UnitsMetaData, UNIT, "uhpm")
#define UNIT_MANA(UNIT) UnitRealField(UnitsMetaData, UNIT, "umpc")
#define UNIT_ACQUISITION_RANGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "uacq")
#define UNIT_CAST_BACK_SWING(UNIT) UnitRealField(UnitsMetaData, UNIT, "ucbs")
#define UNIT_CAST_POINT(UNIT) UnitRealField(UnitsMetaData, UNIT, "ucpt")
#define UNIT_MINIMUM_ATTACK_RANGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "uamn")
#define UNIT_RAISABLE(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "urai")
#define UNIT_DECAYABLE(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "udec")
#define UNIT_IS_A_BUILDING(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "ubdg")
#define UNIT_USE_EXTENDED_LINE_OF_SIGHT(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "ulos")
#define UNIT_NEUTRAL_BUILDING_SHOWS_MINIMAP_ICON(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "unbm")
#define UNIT_HERO_HIDE_HERO_INTERFACE_ICON(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uhhb")
#define UNIT_HERO_HIDE_HERO_MINIMAP_DISPLAY(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uhhm")
#define UNIT_HERO_HIDE_HERO_DEATH_MESSAGE(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uhhd")
#define UNIT_HIDE_MINIMAP_DISPLAY(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uhom")
#define UNIT_SCALE_PROJECTILES(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uscb")
#define UNIT_SELECTION_CIRCLE_ON_WATER(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "usew")
#define UNIT_HAS_WATER_SHADOW(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "ushr")
#define UNIT_NAME(UNIT) UnitStringField(UnitsMetaData, UNIT, "unam")
#define UNIT_PROPER_NAMES(UNIT) UnitStringField(UnitsMetaData, UNIT, "upro")
#define UNIT_GROUND_TEXTURE(UNIT) UnitStringField(UnitsMetaData, UNIT, "uubs")
#define UNIT_SHADOW_IMAGE_UNIT(UNIT) UnitStringField(UnitsMetaData, UNIT, "ushu")
#define UNIT_ABILITIES_NORMAL(UNIT) UnitStringField(UnitsMetaData, UNIT, "uabi")
#define UNIT_ABILITIES_HERO(UNIT) UnitStringField(UnitsMetaData, UNIT, "uhab")
#define UNIT_BUILDS(UNIT) UnitStringField(UnitsMetaData, UNIT, "ubui")
#define UNIT_TRAINS(UNIT) UnitStringField(UnitsMetaData, UNIT, "utra")
#define UNIT_MODEL(UNIT) UnitStringField(UnitsMetaData, UNIT, "umdl")
#define UNIT_UBER_SPLAT(UNIT) UnitStringField(UnitsMetaData, UNIT, "uubs")
#define UNIT_PATH_TEX(UNIT) UnitStringField(UnitsMetaData, UNIT, "upat")


#define UNIT_ATTACK1_LAUNCH_X(UNIT) UnitRealField(UnitsMetaData, UNIT, "ulpx")
#define UNIT_ATTACK1_LAUNCH_Y(UNIT) UnitRealField(UnitsMetaData, UNIT, "ulpy")
#define UNIT_ATTACK1_LAUNCH_Z(UNIT) UnitRealField(UnitsMetaData, UNIT, "ulpz")

#define UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua1d")
#define UNIT_ATTACK1_DAMAGE_BASE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua1b")
#define UNIT_ATTACK1_DAMAGE_SIDES_PER_DIE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua1s")
#define UNIT_ATTACK1_MAXIMUM_NUMBER_OF_TARGETS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "utc1")
#define UNIT_ATTACK1_ATTACK_TYPE(UNIT) UnitStringField(UnitsMetaData, UNIT, "ua1t")
#define UNIT_ATTACK1_WEAPON_TYPE(UNIT) UnitStringField(UnitsMetaData, UNIT, "ua1w")
#define UNIT_ATTACK1_WEAPON_SOUND(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ucs1")
#define UNIT_ATTACK1_AREA_OF_EFFECT_TARGETS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua1p")
#define UNIT_ATTACK1_TARGETS_ALLOWED(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua1g")
#define UNIT_ATTACK1_BACKSWING_POINT(UNIT) UnitRealField(UnitsMetaData, UNIT, "ubs1")
#define UNIT_ATTACK1_DAMAGE_POINT(UNIT) UnitRealField(UnitsMetaData, UNIT, "udp1")
#define UNIT_ATTACK1_BASE_COOLDOWN(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua1c")
#define UNIT_ATTACK1_DAMAGE_LOSS_FACTOR(UNIT) UnitRealField(UnitsMetaData, UNIT, "udl1")
#define UNIT_ATTACK1_DAMAGE_FACTOR_MEDIUM(UNIT) UnitRealField(UnitsMetaData, UNIT, "uhd1")
#define UNIT_ATTACK1_DAMAGE_FACTOR_SMALL(UNIT) UnitRealField(UnitsMetaData, UNIT, "uqd1")
#define UNIT_ATTACK1_DAMAGE_SPILL_DISTANCE(UNIT) UnitRealField(UnitsMetaData, UNIT, "usd1")
#define UNIT_ATTACK1_DAMAGE_SPILL_RADIUS(UNIT) UnitRealField(UnitsMetaData, UNIT, "usr1")
#define UNIT_ATTACK1_PROJECTILE_SPEED(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua1z")
#define UNIT_ATTACK1_PROJECTILE_ARC(UNIT) UnitRealField(UnitsMetaData, UNIT, "uma1")
#define UNIT_ATTACK1_AREA_OF_EFFECT_FULL_DAMAGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua1f")
#define UNIT_ATTACK1_AREA_OF_EFFECT_MEDIUM_DAMAGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua1h")
#define UNIT_ATTACK1_AREA_OF_EFFECT_SMALL_DAMAGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua1q")
#define UNIT_ATTACK1_RANGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua1r")
#define UNIT_ATTACK1_SHOW_UI(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uwu1")
#define UNIT_ATTACK1_PROJECTILE_HOMING_ENABLED(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "umh1")
#define UNIT_ATTACK1_PROJECTILE_ART(UNIT) UnitStringField(UnitsMetaData, UNIT, "ua1m")

#define UNIT_ATTACK2_DAMAGE_NUMBER_OF_DICE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua2d")
#define UNIT_ATTACK2_DAMAGE_BASE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua2b")
#define UNIT_ATTACK2_DAMAGE_SIDES_PER_DIE(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua2s")
#define UNIT_ATTACK2_MAXIMUM_NUMBER_OF_TARGETS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "utc2")
#define UNIT_ATTACK2_ATTACK_TYPE(UNIT) UnitStringField(UnitsMetaData, UNIT, "ua2t")
#define UNIT_ATTACK2_WEAPON_TYPE(UNIT) UnitStringField(UnitsMetaData, UNIT, "ua2w")
#define UNIT_ATTACK2_WEAPON_SOUND(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ucs2")
#define UNIT_ATTACK2_AREA_OF_EFFECT_TARGETS(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua2p")
#define UNIT_ATTACK2_TARGETS_ALLOWED(UNIT) UnitIntegerField(UnitsMetaData, UNIT, "ua2g")
#define UNIT_ATTACK2_BACKSWING_POINT(UNIT) UnitRealField(UnitsMetaData, UNIT, "ubs2")
#define UNIT_ATTACK2_DAMAGE_POINT(UNIT) UnitRealField(UnitsMetaData, UNIT, "udp2")
#define UNIT_ATTACK2_BASE_COOLDOWN(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua2c")
#define UNIT_ATTACK2_DAMAGE_LOSS_FACTOR(UNIT) UnitRealField(UnitsMetaData, UNIT, "udl2")
#define UNIT_ATTACK2_DAMAGE_FACTOR_MEDIUM(UNIT) UnitRealField(UnitsMetaData, UNIT, "uhd2")
#define UNIT_ATTACK2_DAMAGE_FACTOR_SMALL(UNIT) UnitRealField(UnitsMetaData, UNIT, "uqd2")
#define UNIT_ATTACK2_DAMAGE_SPILL_DISTANCE(UNIT) UnitRealField(UnitsMetaData, UNIT, "usd2")
#define UNIT_ATTACK2_DAMAGE_SPILL_RADIUS(UNIT) UnitRealField(UnitsMetaData, UNIT, "usr2")
#define UNIT_ATTACK2_PROJECTILE_SPEED(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua2z")
#define UNIT_ATTACK2_PROJECTILE_ARC(UNIT) UnitRealField(UnitsMetaData, UNIT, "uma2")
#define UNIT_ATTACK2_AREA_OF_EFFECT_FULL_DAMAGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua2f")
#define UNIT_ATTACK2_AREA_OF_EFFECT_MEDIUM_DAMAGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua2h")
#define UNIT_ATTACK2_AREA_OF_EFFECT_SMALL_DAMAGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua2q")
#define UNIT_ATTACK2_RANGE(UNIT) UnitRealField(UnitsMetaData, UNIT, "ua2r")
#define UNIT_ATTACK2_SHOW_UI(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uwu2")
#define UNIT_ATTACK2_PROJECTILE_HOMING_ENABLED(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "umh2")
#define UNIT_ATTACK2_PROJECTILE_ART(UNIT) UnitStringField(UnitsMetaData, UNIT, "ua2m")

#define UNIT_ATTACKS_ENABLED(UNIT) UnitBooleanField(UnitsMetaData, UNIT, "uaen")

#define DESTRUCTABLE_NAME(UNIT) UnitStringField(DestructableMetaData, UNIT, "bnam")
#define DESTRUCTABLE_HIT_POINT_MAXIMUM(UNIT) UnitRealField(DestructableMetaData, UNIT, "bhps")
#define DESTRUCTABLE_TARGETED_AS(UNIT) UnitStringField(DestructableMetaData, UNIT, "btar")
#define DESTRUCTABLE_ARMOR_TYPE(UNIT) UnitIntegerField(DestructableMetaData, UNIT, "barm")
#define DESTRUCTABLE_BUILD_TIME(UNIT) UnitIntegerField(DestructableMetaData, UNIT, "bbut")
#define DESTRUCTABLE_REPAIR_TIME(UNIT) UnitIntegerField(DestructableMetaData, UNIT, "bret")
#define DESTRUCTABLE_GOLD_REPAIR(UNIT) UnitIntegerField(DestructableMetaData, UNIT, "breg")
#define DESTRUCTABLE_LUMBER_REPAIR(UNIT) UnitIntegerField(DestructableMetaData, UNIT, "brel")
#define DESTRUCTABLE_TEXTURE(UNIT) UnitStringField(DestructableMetaData, UNIT, "btxf")
#define DESTRUCTABLE_FILE(UNIT) UnitStringField(DestructableMetaData, UNIT, "bfil")
#define DESTRUCTABLE_DIRECTORY(UNIT) UnitStringField(DestructableMetaData, UNIT, "bdir")

#endif
