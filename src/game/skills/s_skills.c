#include "s_skills.h"

typedef struct {
    LPCSTR classname;
    ability_t *ability;
} abilityitem_t;

void CMD_CancelCommand(LPEDICT ent);

FLOAT AB_Number(LPCSTR classname, LPCSTR field) {
    LPCSTR str = gi.FindSheetCell(game.config.abilities, classname, field);
    return str ? atof(str) : 0;
}

static abilityitem_t abilitylist[] = {
    { STR_CmdStop, &a_stop },
    { STR_CmdMove, &a_move },
    { STR_CmdAttack, &a_attack },
    { STR_CmdBuild, &a_build },
    { STR_CmdHoldPos, &a_holdpos },
    { STR_CmdPatrol, &a_patrol },
    { STR_CmdCancel, &a_cancel },
    { STR_CmdSelectSkill, &a_selectskill },

    { "Ahar", &a_harvest },
    { "Amil", &a_militia },
    { "Arep", &a_repair },
    { "Agld", &a_goldmine },
    { "AHad", &a_devotionaura},
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
    return 0;
}
