#include "g_local.h"

#define ABILITY(NAME) static void M_##NAME(edict_t *ent, ability_t const *ability)

ABILITY(Move) {
}

ABILITY(Harvest) {
}

ABILITY(Militia) {
}

ABILITY(Repair) {
}

ABILITY(Build) {
}

ABILITY(Attack) {
}

ABILITY(HoldPosition) {
}

ABILITY(Patrol) {
}

ABILITY(Stop) {
}

static ability_t abilitylist[] = {
    { NULL },    // leave index 0 alone

    { CMD_MOVE, M_Move, },
    { CMD_ATTACK, M_Attack, },
    { CMD_BUILD_HUMAN, M_Build, },
    { CMD_BUILD_ORC, M_Build, },
    { CMD_HOLDPOS, M_HoldPosition, },
    { CMD_PATROL, M_Patrol, },
    { CMD_STOP, M_Stop, },

    { "Ahar", M_Harvest, },
    { "Amil", M_Militia, },
    { "Arep", M_Repair, },
    { NULL }
};

ability_t *FindAbilityByClassname(LPCSTR classname) {
    FOR_LOOP(i, game.num_abilities) {
        if (!abilitylist[i].classname)
            continue;
        if (!strcmp(abilitylist[i].classname, classname))
            return &abilitylist[i];
    }
    return NULL;
}

DWORD FindAbilityIndex(LPCSTR classname) {
    ability_t const *ability = FindAbilityByClassname(classname);
    return ability ? (DWORD)(ability - abilitylist) : 0;
}

void InitAbilities(void) {
    game.num_abilities = sizeof(abilitylist)/sizeof(abilitylist[0]) - 1;
}

void SetAbilityNames(void) {
    FOR_LOOP(i, game.num_abilities) {
        gi.configstring(CS_ITEMS + i, abilitylist[i].classname);
    }
}

ability_t *GetAbilityByIndex(DWORD index) {
    if (index == 0 || index >= game.num_abilities)
        return NULL;
    return &abilitylist[index];
}
