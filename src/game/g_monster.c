#include "g_local.h"

#include "Units/UnitUI.h"
#include "Units/UnitBalance.h"
#include "Units/UnitWeapons.h"
#include "Units/UnitAbilities.h"
#include "Units/UnitData.h"
#include "Units/AbilityData.h"

#include <stdlib.h>

#define MAX_WAYPOINTS 256
static edict_t waypoints[MAX_WAYPOINTS];
DWORD current_waypoint = 0;

edict_t *Waypoint_add(VECTOR2 spot) {
    edict_t *waypoint = &waypoints[current_waypoint++ % MAX_WAYPOINTS];
    waypoint->s.origin.x = spot.x;
    waypoint->s.origin.y = spot.y;
    M_CheckGround(waypoint);
    return waypoint;
}

void M_MoveFrame(edict_t *self) {
    if (self->unitinfo.aiflags & AI_HOLD_FRAME)
        return;
    umove_t const *move = self->unitinfo.currentmove;
    animationInfo_t const *anim = self->animation;
    if (!anim)
        return;
    if (self->s.frame < anim->firstframe ||
        self->s.frame >= anim->lastframe)
    {
        self->s.frame = anim->firstframe;
    } else if ((self->s.frame + FRAMETIME) >= anim->lastframe) {
        SAFE_CALL(move->endfunc, self);
        if (!(self->unitinfo.aiflags & AI_HOLD_FRAME)) {
            self->s.frame = anim->firstframe;
        }
    } else {
        self->s.frame += FRAMETIME;
    }
}

void monster_think(edict_t *self) {
    if (!self->unitinfo.currentmove)
        return;
    M_MoveFrame(self);
    if (self->unitinfo.currentmove->think) {
        self->unitinfo.currentmove->think(self);
    }
}

void monster_start(edict_t *self) {
    animationInfo_t const *anim = self->animation;
    if (anim) {
        DWORD len = MAX(1, anim->lastframe - anim->firstframe - 1);
        self->s.frame = (anim->firstframe + (rand() % len));
    }
    if (!self->unitinfo.checkattack) {
        self->unitinfo.checkattack = M_CheckAttack;
    }
}

void M_AddAbility(edict_t *self, LPCSTR classname) {
    ability_t *ability = FindAbilityByClassname(classname);
    if (!ability)
        return;
    LPCSTR str = NULL;
    if ((str = FindConfigValue(classname, "Art"))) {
        ability->imageindex = gi.ImageIndex(str);
    }
    if ((str = FindConfigValue(classname, "buttonpos"))) {
        sscanf(str, "%d,%d", &ability->buttonPos[0], &ability->buttonPos[1]);
    }
    self->abilities[self->num_abilities++] = ability;
    
    FOR_LOOP(i, MAX_ABILITIES) {
        if (self->s.commands[i] != 0)
            continue;
        self->s.commands[i] = FindAbilityIndex(classname);
        break;
    }
}

void M_AddDefaultAbilityMove(edict_t *self) {
    if (self->unitinfo.ui->run > 0) {
        M_AddAbility(self, CMD_MOVE);
    }
}

void M_AddDefaultAbilityBuild(edict_t *self) {
    LPCSTR builds = FindConfigValue(GetClassName(self->class_id), "builds");
    if (builds && *builds) {
        if (!strcmp(self->unitinfo.data->race, RACE_HUMAN)) {
            M_AddAbility(self, CMD_BUILD_HUMAN);
        }
        if (!strcmp(self->unitinfo.data->race, RACE_ORC)) {
            M_AddAbility(self, CMD_BUILD_ORC);
        }
    }
}

void M_AddDefaultAbilityAttack(edict_t *self) {
    M_AddAbility(self, CMD_ATTACK);
}

void M_AddDefaultAbilityPatrol(edict_t *self) {
    M_AddAbility(self, CMD_PATROL);
}

void M_AddDefaultAbilityHoldPosition(edict_t *self) {
    M_AddAbility(self, CMD_HOLDPOS);
}

void M_AddDefaultAbilityStop(edict_t *self) {
    M_AddAbility(self, CMD_STOP);
}

void M_AddDefaultAbilities(edict_t *self) {
    M_AddDefaultAbilityMove(self);
    M_AddDefaultAbilityStop(self);
    M_AddDefaultAbilityBuild(self);
    M_AddDefaultAbilityAttack(self);
    M_AddDefaultAbilityPatrol(self);
    M_AddDefaultAbilityHoldPosition(self);
}

void M_ParseData(edict_t *self, struct UnitData const* data) {
    self->unitinfo.data = data;
}

void M_ParseBalance(edict_t *self, struct UnitBalance const* data) {
    if (!data) return;
    self->unitinfo.health = data->HP;
    self->unitinfo.balance = data;
}

void M_ParseAbilities(edict_t *self, struct UnitAbilities const* data) {
    if (!data) return;
    DWORD slot = 0;
//    printf("%.4s\n", &self->class_id);
    for (LPCSTR abil = data->abilList; abil; abil = strstr(abil+1, ",")) {
        DWORD abil_id = *(DWORD *)(*abil == ',' ? abil + 1 : abil);
        struct AbilityData const *ability = FindAbilityData(abil_id);
        if (ability) {
            M_AddAbility(self, GetClassName(ability->code));
            self->unitinfo.abil[slot++] = ability;
        }
    }
    self->unitinfo.abilities = data;
}

void M_ParseWeapon(edict_t *self, struct UnitWeapons const* data) {
    if (!data) return;
    self->unitinfo.weapon = data;
}

void M_ParseUnitUI(edict_t *self, struct UnitUI const *data) {
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", data->file);
    self->s.model = gi.ModelIndex(buffer);
    self->s.scale = data->modelScale;
    self->flags |= data->isbldg ? 0 : IS_UNIT;
    self->unitinfo.ui = data;
}

void SP_SpawnUnit(edict_t *self, struct UnitUI const *unit) {
    M_ParseUnitUI(self, unit);
    M_ParseAbilities(self, FindUnitAbilities(self->class_id));
    M_ParseBalance(self, FindUnitBalance(self->class_id));
    M_ParseWeapon(self, FindUnitWeapons(self->class_id));
    M_ParseData(self, FindUnitData(self->class_id));
    M_AddDefaultAbilities(self);

    self->think = monster_think;
}

void M_CheckGround(edict_t *self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(edict_t *self) {
    return false;
}
