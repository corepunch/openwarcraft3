#include "g_local.h"

#include "Units/UnitUI.h"
#include "Units/UnitBalance.h"
#include "Units/UnitWeapons.h"

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

void SP_SpawnUnit(LPEDICT self, struct UnitUI const *unit) {
    self->unitinfo.balance = FindUnitBalance(self->class_id);
    self->unitinfo.weapon = FindUnitWeapons(self->class_id);
    self->unitinfo.ui = unit;
//    if (/*self->s.number == 66 || */self->s.number == 107 ) {
//        printf("%d %.4s\n", self->s.number, &self->class_id);
//    }
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", unit->file);
    self->s.model = gi.ModelIndex(buffer);
    self->s.scale = unit->modelScale;
    self->think = monster_think;
    self->flags |= unit->isbldg ? 0 : IS_UNIT;
    if (self->unitinfo.balance) {
        self->unitinfo.health = self->unitinfo.balance->HP;
    }
}

void M_CheckGround(LPEDICT self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(LPEDICT self) {
    return false;
}
