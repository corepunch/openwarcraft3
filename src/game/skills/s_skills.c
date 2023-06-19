#include "g_local.h"

void SP_ability_harvest(ability_t *self);
void SP_ability_move(ability_t *self);
void SP_ability_attack(ability_t *self);
void SP_ability_build(ability_t *self);

ABILITY(Move) {
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

void CMD_CancelCommand(edict_t *ent);

static ability_t abilitylist[] = {
    { NULL },    // leave index 0 alone

    { STR_CmdMove, SP_ability_move },
    { STR_CmdAttack, SP_ability_attack },
    { STR_CmdBuildHuman, SP_ability_build },
    { STR_CmdBuildOrc, SP_ability_build },
    { STR_CmdHoldPos, NULL, M_HoldPosition, NULL },
    { STR_CmdPatrol, NULL, M_Patrol, NULL },
    { STR_CmdStop, NULL, M_Stop, NULL },
    { STR_CmdCancel, NULL, NULL, CMD_CancelCommand },
    
    { "Ahar", SP_ability_harvest, NULL, NULL },
    { "Amil", NULL, M_Militia, NULL },
    { "Arep", NULL, M_Repair, NULL },

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
    FOR_LOOP(i, game.num_abilities) {
        ability_t *abil = &abilitylist[i];
        if (abil->init) {
            abil->init(abil);
        }
    }
}

void SetAbilityNames(void) {
    FOR_LOOP(i, game.num_abilities) {
        if (!abilitylist[i].classname)
            continue;
        ability_t *abil = &abilitylist[i];
        LPCSTR art = FindConfigValue(abil->classname, STR_ART);
        LPCSTR buttonpos = FindConfigValue(abil->classname, STR_BUTTONPOS);
        abil->buttonimage = gi.ImageIndex(art);
        sscanf(buttonpos, "%d,%d", &abil->buttonpos.x, &abil->buttonpos.y);
        gi.configstring(CS_ITEMS + i, abil->classname);
    }
}

ability_t *GetAbilityByIndex(DWORD index) {
    if (index == 0 || index >= game.num_abilities)
        return NULL;
    return &abilitylist[index];
}
