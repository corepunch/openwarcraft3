#include "g_local.h"

sheetMetaData_t DestructableMetaData[] = {
    { "bnam", "Name", "DestructableData" },
    { "bsuf", "EditorSuffix", "DestructableData" },
    { "bcat", "category", "DestructableData" },
    { "btil", "tilesets", "DestructableData" },
    { "btsp", "tilesetSpecific", "DestructableData" },
    { "bfil", "file", "DestructableData" },
    { "bdir", "dir", "DestructableData" },
    { "blit", "lightweight", "DestructableData" },
    { "bflo", "fatLOS", "DestructableData" },
    { "btxi", "texID", "DestructableData" },
    { "btxf", "texFile", "DestructableData" },
    { "buch", "useClickHelper", "DestructableData" },
    { "bonc", "onCliffs", "DestructableData" },
    { "bonw", "onWater", "DestructableData" },
    { "bcpd", "canPlaceDead", "DestructableData" },
    { "bwal", "walkable", "DestructableData" },
    { "bclh", "cliffHeight", "DestructableData" },
    { "btar", "targType", "DestructableData" },
    { "barm", "armor", "DestructableData" },
    { "bvar", "numVar", "DestructableData" },
    { "bhps", "HP", "DestructableData" },
    { "boch", "occH", "DestructableData" },
    { "bflh", "flyH", "DestructableData" },
    { "bfxr", "fixedRot", "DestructableData" },
    { "bsel", "selSize", "DestructableData" },
    { "bmis", "minScale", "DestructableData" },
    { "bmas", "maxScale", "DestructableData" },
    { "bcpr", "canPlaceRandScale", "DestructableData" },
    { "bmap", "maxPitch", "DestructableData" },
    { "bmar", "maxRoll", "DestructableData" },
    { "brad", "radius", "DestructableData" },
    { "bfra", "fogRadius", "DestructableData" },
    { "bfvi", "fogVis", "DestructableData" },
    { "bptx", "pathTex", "DestructableData" },
    { "bptd", "pathTexDeath", "DestructableData" },
    { "bdsn", "deathSnd", "DestructableData" },
    { "bshd", "shadow", "DestructableData" },
    { "bsmm", "showInMM", "DestructableData" },
    { "bmmr", "MMRed", "DestructableData" },
    { "bmmg", "MMGreen", "DestructableData" },
    { "bmmb", "MMBlue", "DestructableData" },
    { "bumm", "useMMColor", "DestructableData" },
    { "bbut", "buildTime", "DestructableData" },
    { "bret", "repairTime", "DestructableData" },
    { "breg", "goldRep", "DestructableData" },
    { "brel", "lumberRep", "DestructableData" },
    { "busr", "UserList", "DestructableData" },
    { "bvcr", "colorR", "DestructableData" },
    { "bvcg", "colorG", "DestructableData" },
    { "bvcb", "colorB", "DestructableData" },
    { "bgse", "selectable", "DestructableData" },
    { "bgsc", "selcircsize", "DestructableData" },
    { "bgpm", "portraitmodel", "DestructableData" },
    { NULL }
};

sheetMetaData_t UnitsMetaData[] = {
    { "iabi", "abilList", "ItemData" },
    { "iarm", "armor", "ItemData" },
    { "icla", "class", "ItemData" },
    { "iclb", "colorB", "ItemData" },
    { "iclg", "colorG", "ItemData" },
    { "iclr", "colorR", "ItemData" },
    { "icid", "cooldownID", "ItemData" },
    { "idrp", "drop", "ItemData" },
    { "idro", "droppable", "ItemData" },
    { "ifil", "file", "ItemData" },
    { "igol", "goldcost", "ItemData" },
    { "ihtp", "HP", "ItemData" },
    { "iicd", "ignoreCD", "ItemData" },
    { "ilev", "Level", "ItemData" },
    { "ilum", "lumbercost", "ItemData" },
    { "imor", "morph", "ItemData" },
    { "ilvo", "oldLevel", "ItemData" },
    { "iper", "perishable", "ItemData" },
    { "iprn", "pickRandom", "ItemData" },
    { "ipow", "powerup", "ItemData" },
    { "ipri", "prio", "ItemData" },
    { "isca", "scale", "ItemData" },
    { "issc", "selSize", "ItemData" },
    { "isel", "sellable", "ItemData" },
    { "ipaw", "pawnable", "ItemData" },
    { "isto", "stockMax", "ItemData" },
    { "istr", "stockRegen", "ItemData" },
    { "isst", "stockStart", "ItemData" },
    { "iusa", "usable", "ItemData" },
    { "iuse", "uses", "ItemData" },
    { "uani", "animProps", "Profile" },
    { "uico", "Art", "Profile" },
    { "iico", "Art", "Profile" },
    { "uaap", "Attachmentanimprops", "Profile" },
    { "ualp", "Attachmentlinkprops", "Profile" },
    { "uawt", "Awakentip", "Profile" },
    { "ubpr", "Boneprops", "Profile" },
    { "ubsl", "BuildingSoundLabel", "Profile" },
    { "ubui", "Builds", "Profile" },
    { "ubpx", "Buttonpos", "Profile" },
    { "ubpy", "Buttonpos", "Profile" },
    { "ucua", "Casterupgradeart", "Profile" },
    { "ucun", "Casterupgradename", "Profile" },
    { "ucut", "Casterupgradetip", "Profile" },
    { "udep", "DependencyOr", "Profile" },
    { "ides", "Description", "Profile" },
    { "unsf", "EditorSuffix", "Profile" },
    { "uhot", "Hotkey", "Profile" },
    { "ulfi", "LoopingSoundFadeIn", "Profile" },
    { "ulfo", "LoopingSoundFadeOut", "Profile" },
    { "umki", "Makeitems", "Profile" },
    { "uma1", "Missilearc", "Profile" },
    { "uma2", "Missilearc", "Profile" },
    { "ua1m", "Missileart", "Profile" },
    { "ua2m", "Missileart", "Profile" },
    { "umh1", "MissileHoming", "Profile" },
    { "umh2", "MissileHoming", "Profile" },
    { "ua1z", "Missilespeed", "Profile" },
    { "ua2z", "Missilespeed", "Profile" },
    { "umsl", "MovementSoundLabel", "Profile" },
    { "unam", "Name", "Profile" },
    { "upro", "Propernames", "Profile" },
    { "ursl", "RandomSoundLabel", "Profile" },
    { "urqc", "Requirescount", "Profile" },
    { "ureq", "Requires", "Profile" },
    { "urq1", "Requires1", "Profile" },
    { "urq2", "Requires2", "Profile" },
    { "urq3", "Requires3", "Profile" },
    { "urq4", "Requires4", "Profile" },
    { "urq5", "Requires5", "Profile" },
    { "urq6", "Requires6", "Profile" },
    { "urq7", "Requires7", "Profile" },
    { "urq8", "Requires8", "Profile" },
    { "urqa", "Requiresamount", "Profile" },
    { "ures", "Researches", "Profile" },
    { "urev", "Revive", "Profile" },
    { "utpr", "Revivetip", "Profile" },
    { "ussi", "ScoreScreenIcon", "Profile" },
    { "usei", "Sellitems", "Profile" },
    { "useu", "Sellunits", "Profile" },
    { "uspa", "Specialart", "Profile" },
    { "utaa", "Targetart", "Profile" },
    { "utip", "Tip", "Profile" },
    { "utra", "Trains", "Profile" },
    { "urva", "Reviveat", "Profile" },
    { "utub", "Ubertip", "Profile" },
    { "uupt", "Upgrade", "Profile" },
    { "uabi", "abilList", "UnitAbilities" },
    { "udaa", "auto", "UnitAbilities" },
    { "uhab", "heroAbilList", "UnitAbilities" },
    { "uagi", "AGI", "UnitBalance" },
    { "uagp", "AGIplus", "UnitBalance" },
    { "ubld", "bldtm", "UnitBalance" },
    { "ubdi", "bountydice", "UnitBalance" },
    { "ubba", "bountyplus", "UnitBalance" },
    { "ubsi", "bountysides", "UnitBalance" },
    { "ulbd", "lumberbountydice", "UnitBalance" },
    { "ulba", "lumberbountyplus", "UnitBalance" },
    { "ulbs", "lumberbountysides", "UnitBalance" },
    { "ucol", "collision", "UnitData" },
    { "udef", "def", "UnitBalance" },
    { "udty", "defType", "UnitBalance" },
    { "udup", "defUp", "UnitBalance" },
    { "ufma", "fmade", "UnitBalance" },
    { "ufoo", "fused", "UnitBalance" },
    { "ugol", "goldcost", "UnitBalance" },
    { "ugor", "goldRep", "UnitBalance" },
    { "uhpm", "HP", "UnitBalance" },
    { "uint", "INT", "UnitBalance" },
    { "uinp", "INTplus", "UnitBalance" },
    { "ubdg", "isbldg", "UnitUI" }, // "UnitBalance" },
    { "ulev", "level", "UnitBalance" },
    { "ulum", "lumbercost", "UnitBalance" },
    { "ulur", "lumberRep", "UnitBalance" },
    { "umpi", "mana0", "UnitBalance" },
    { "umpm", "manaN", "UnitBalance" },
    { "umas", "maxSpd", "UnitBalance" },
    { "umis", "minSpd", "UnitBalance" },
    { "unbr", "nbrandom", "UnitBalance" },
    { "usin", "nsight", "UnitBalance" },
    { "upap", "preventPlace", "UnitBalance" },
    { "upra", "Primary", "UnitBalance" },
    { "uhpr", "regenHP", "UnitBalance" },
    { "umpr", "regenMana", "UnitBalance" },
    { "uhrt", "regenType", "UnitBalance" },
    { "urtm", "reptm", "UnitBalance" },
    { "urpo", "repulse", "UnitBalance" },
    { "urpg", "repulseGroup", "UnitBalance" },
    { "urpp", "repulseParam", "UnitBalance" },
    { "urpr", "repulsePrio", "UnitBalance" },
    { "upar", "requirePlace", "UnitBalance" },
    { "usid", "sight", "UnitBalance" },
    { "umvs", "spd", "UnitBalance" },
    { "usma", "stockMax", "UnitBalance" },
    { "usrg", "stockRegen", "UnitBalance" },
    { "usst", "stockStart", "UnitBalance" },
    { "ustr", "STR", "UnitBalance" },
    { "ustp", "STRplus", "UnitBalance" },
    { "util", "tilesets", "UnitBalance" },
    { "utyp", "type", "UnitBalance" },
    { "upgr", "upgrades", "UnitBalance" },
    { "uabr", "buffRadius", "UnitData" },
    { "uabt", "buffType", "UnitData" },
    { "ucbo", "canBuildOn", "UnitData" },
    { "ufle", "canFlee", "UnitData" },
    { "usle", "canSleep", "UnitData" },
    { "ucar", "cargoSize", "UnitData" },
    { "udtm", "death", "UnitData" },
    { "udea", "deathType", "UnitData" },
    { "ulos", "fatLOS", "UnitData" },
    { "ufor", "formation", "UnitData" },
    { "uibo", "isBuildOn", "UnitData" },
    { "umvf", "moveFloor", "UnitData" },
    { "umvh", "moveHeight", "UnitData" },
    { "umvt", "movetp", "UnitData" },
    { "upru", "nameCount", "UnitData" },
    { "uori", "orientInterp", "UnitData" },
    { "upat", "pathTex", "UnitData" },
    { "upoi", "points", "UnitData" },
    { "upri", "prio", "UnitData" },
    { "uprw", "propWin", "UnitData" },
    { "urac", "race", "UnitData" },
    { "upaw", "requireWaterRadius", "UnitData" },
    { "utar", "targType", "UnitData" },
    { "umvr", "turnRate", "UnitData" },
    { "uarm", "armor", "UnitUI" },
    { "uble", "blend", "UnitUI" },
    { "uclb", "blue", "UnitUI" },
    { "ushb", "buildingShadow", "UnitUI" },
    { "ucam", "campaign", "UnitUI" },
    { "utcc", "customTeamColor", "UnitUI" },
    { "udro", "dropItems", "UnitUI" },
    { "uept", "elevPts", "UnitUI" },
    { "uerd", "elevRad", "UnitUI" },
    { "umdl", "file", "UnitUI" },
    { "uver", "fileVerFlags", "UnitUI" },
    { "ufrd", "fogRad", "UnitUI" },
    { "uclg", "green", "UnitUI" },
    { "uhos", "hostilePal", "UnitUI" },
    { "uine", "inEditor", "UnitUI" },
    { "umxp", "maxPitch", "UnitUI" },
    { "umxr", "maxRoll", "UnitUI" },
    { "usca", "modelScale", "UnitUI" },
    { "unbm", "nbmmIcon", "UnitUI" },
    { "uhhb", "hideHeroBar", "UnitUI" },
    { "uhhm", "hideHeroMinimap", "UnitUI" },
    { "uhhd", "hideHeroDeathMsg", "UnitUI" },
    { "uhom", "hideOnMinimap", "UnitUI" },
    { "uocc", "occH", "UnitUI" },
    { "uclr", "red", "UnitUI" },
    { "urun", "run", "UnitUI" },
    { "ussc", "scale", "UnitUI" },
    { "uscb", "scaleBull", "UnitUI" },
    { "usew", "selCircOnWater", "UnitUI" },
    { "uslz", "selZ", "UnitUI" },
    { "ushh", "shadowH", "UnitUI" },
    { "ushr", "shadowOnWater", "UnitUI" },
    { "ushw", "shadowW", "UnitUI" },
    { "ushx", "shadowX", "UnitUI" },
    { "ushy", "shadowY", "UnitUI" },
    { "uspe", "special", "UnitUI" },
    { "utco", "teamColor", "UnitUI" },
    { "utss", "tilesetSpecific", "UnitUI" },
    { "uubs", "uberSplat", "UnitUI" },
    { "ushu", "unitShadow", "UnitUI" },
    { "usnd", "unitSound", "UnitUI" },
    { "uuch", "useClickHelper", "UnitUI" },
    { "uwal", "walk", "UnitUI" },
    { "uacq", "acquire", "UnitWeapons" },
    { "ua1t", "atkType1", "UnitWeapons" },
    { "ua2t", "atkType2", "UnitWeapons" },
    { "ubs1", "backSw1", "UnitWeapons" },
    { "ubs2", "backSw2", "UnitWeapons" },
    { "ucbs", "castbsw", "UnitWeapons" },
    { "ucpt", "castpt", "UnitWeapons" },
    { "ua1c", "cool1", "UnitWeapons" },
    { "ua2c", "cool2", "UnitWeapons" },
    { "udl1", "damageLoss1", "UnitWeapons" },
    { "udl2", "damageLoss2", "UnitWeapons" },
    { "ua1d", "dice1", "UnitWeapons" },
    { "ua2d", "dice2", "UnitWeapons" },
    { "ua1b", "dmgplus1", "UnitWeapons" },
    { "ua2b", "dmgplus2", "UnitWeapons" },
    { "udp1", "dmgpt1", "UnitWeapons" },
    { "udp2", "dmgpt2", "UnitWeapons" },
    { "udu1", "dmgUp1", "UnitWeapons" },
    { "udu2", "dmgUp2", "UnitWeapons" },
    { "ua1f", "Farea1", "UnitWeapons" },
    { "ua2f", "Farea2", "UnitWeapons" },
    { "ua1h", "Harea1", "UnitWeapons" },
    { "ua2h", "Harea2", "UnitWeapons" },
    { "uhd1", "Hfact1", "UnitWeapons" },
    { "uhd2", "Hfact2", "UnitWeapons" },
    { "uisz", "impactSwimZ", "UnitWeapons" },
    { "uimz", "impactZ", "UnitWeapons" },
    { "ulsz", "launchSwimZ", "UnitWeapons" },
    { "ulpx", "launchX", "UnitData" },
    { "ulpy", "launchY", "UnitData" },
    { "ulpz", "launchZ", "UnitData" },
    { "uamn", "minRange", "UnitWeapons" },
    { "ua1q", "Qarea1", "UnitWeapons" },
    { "ua2q", "Qarea2", "UnitWeapons" },
    { "uqd1", "Qfact1", "UnitWeapons" },
    { "uqd2", "Qfact2", "UnitWeapons" },
    { "ua1r", "rangeN1", "UnitWeapons" },
    { "ua2r", "rangeN2", "UnitWeapons" },
    { "urb1", "RngBuff1", "UnitWeapons" },
    { "urb2", "RngBuff2", "UnitWeapons" },
    { "uwu1", "showUI1", "UnitWeapons" },
    { "uwu2", "showUI2", "UnitWeapons" },
    { "ua1s", "sides1", "UnitWeapons" },
    { "ua2s", "sides2", "UnitWeapons" },
    { "usd1", "spillDist1", "UnitWeapons" },
    { "usd2", "spillDist2", "UnitWeapons" },
    { "usr1", "spillRadius1", "UnitWeapons" },
    { "usr2", "spillRadius2", "UnitWeapons" },
    { "ua1p", "splashTargs1", "UnitWeapons" },
    { "ua2p", "splashTargs2", "UnitWeapons" },
    { "utc1", "targCount1", "UnitWeapons" },
    { "utc2", "targCount2", "UnitWeapons" },
    { "ua1g", "targs1", "UnitWeapons" },
    { "ua2g", "targs2", "UnitWeapons" },
    { "uaen", "weapsOn", "UnitWeapons" },
    { "ua1w", "weapTp1", "UnitWeapons" },
    { "ua2w", "weapTp2", "UnitWeapons" },
    { "ucs1", "weapType1", "UnitWeapons" },
    { "ucs2", "weapType2", "UnitWeapons" },
    { NULL }
};

sheetMetaData_t ItemsMetaData[] = {
    { "inam", "comment", "ItemData" },
    { "icla", "itemClass", "ItemData" },
    { "ifil", "file", "ItemData" },
    { NULL }
};
