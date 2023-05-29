#include "g_local.h"

#include "Units/UnitUI.h"
#include "Units/UnitBalance.h"
#include "Units/UnitWeapons.h"
#include "Units/UnitAbilities.h"
#include "Units/AbilityData.h"

#include <stdlib.h>

#define MAX_WAYPOINTS 256
static EDICT waypoints[MAX_WAYPOINTS];
DWORD current_waypoint = 0;

LPEDICT Waypoint_add(VECTOR2 spot) {
    LPEDICT waypoint = &waypoints[current_waypoint++ % MAX_WAYPOINTS];
    waypoint->s.origin.x = spot.x;
    waypoint->s.origin.y = spot.y;
    M_CheckGround(waypoint);
    return waypoint;
}

void M_MoveFrame(LPEDICT self) {
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

void monster_think(LPEDICT self) {
    if (!self->unitinfo.currentmove)
        return;
    M_MoveFrame(self);
    if (self->unitinfo.currentmove->think) {
        self->unitinfo.currentmove->think(self);
    }
}

void monster_start(LPEDICT self) {
    animationInfo_t const *anim = self->animation;
    if (anim) {
        DWORD len = MAX(1, anim->lastframe - anim->firstframe - 1);
        self->s.frame = (anim->firstframe + (rand() % len));
    }
    if (!self->unitinfo.checkattack) {
        self->unitinfo.checkattack = M_CheckAttack;
    }
}

void M_ParseBalance(LPEDICT self, struct UnitBalance const* data) {
    if (!data) return;
    self->unitinfo.health = data->HP;
    self->unitinfo.balance = data;
}

void M_ParseAbilities(LPEDICT self, struct UnitAbilities const* data) {
    if (!data) return;
    DWORD slot = 0;
    for (LPCSTR abil = data->abilList; abil; abil = strstr(abil+1, ",")) {
        DWORD abil_id = *(DWORD *)(*abil == ',' ? abil + 1 : abil);
        struct AbilityData const *ability = FindAbilityData(abil_id);
        if (ability) {
            self->unitinfo.abil[slot++] = ability;
        }
    }
    self->unitinfo.abilities = data;
}

void M_ParseWeapon(LPEDICT self, struct UnitWeapons const* data) {
    if (!data) return;
    self->unitinfo.weapon = data;
}

void M_ParseUnitUI(LPEDICT self, struct UnitUI const *data) {
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", data->file);
    self->s.model = gi.ModelIndex(buffer);
    self->s.scale = data->modelScale;
    self->flags |= data->isbldg ? 0 : IS_UNIT;
    self->unitinfo.ui = data;
}

void SP_SpawnUnit(LPEDICT self, struct UnitUI const *unit) {
    M_ParseUnitUI(self, unit);
    M_ParseAbilities(self, FindUnitAbilities(self->class_id));
    M_ParseBalance(self, FindUnitBalance(self->class_id));
    M_ParseWeapon(self, FindUnitWeapons(self->class_id));

    self->think = monster_think;
}

void M_CheckGround(LPEDICT self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(LPEDICT self) {
    return false;
}
