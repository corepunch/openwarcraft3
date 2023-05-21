#include "g_local.h"

#include "Units/UnitUI.h"
#include "Units/UnitBalance.h"
#include "Units/UnitWeapons.h"

#include <stdlib.h>

#define MAX_WAYPOINTS 256
static EDICT waypoints[MAX_WAYPOINTS];
DWORD current_waypoint = 0;

LPEDICT Waypoint_add(VECTOR2 spot) {
    LPEDICT lpWaypoint = &waypoints[current_waypoint++ % MAX_WAYPOINTS];
    lpWaypoint->s.origin.x = spot.x;
    lpWaypoint->s.origin.y = spot.y;
    M_CheckGround(lpWaypoint);
    return lpWaypoint;
}

void M_MoveFrame(LPEDICT self) {
    if (self->monsterinfo.aiflags & AI_HOLD_FRAME)
        return;
    mmove_t const *move = self->monsterinfo.currentmove;
    ANIMATION anim = gi.GetAnimation(self->s.model, move->animation);
    if (self->s.frame < anim.firstframe || self->s.frame >= anim.lastframe) {
        self->s.frame = anim.firstframe;
    } else {
        if ((self->s.frame + FRAMETIME) >= anim.lastframe) {
            SAFE_CALL(move->endfunc, self);
            if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME)) {
                self->s.frame = anim.firstframe;
            }
        } else {
            self->s.frame += FRAMETIME;
        }
    }
}

void monster_think(LPEDICT self) {
    M_MoveFrame(self);
    if (self->monsterinfo.currentmove->think) {
        self->monsterinfo.currentmove->think(self);
    }
}

void monster_start(LPEDICT self) {
    if (self->monsterinfo.currentmove) {
        ANIMATION anim = gi.GetAnimation(self->s.model, self->monsterinfo.currentmove->animation);
        self->s.frame = anim.firstframe + (rand() % (anim.lastframe - anim.firstframe + 1));
    }
    if (!self->monsterinfo.checkattack) {
        self->monsterinfo.checkattack = M_CheckAttack;
    }
}

void SP_monster_peon(LPEDICT self);

void SP_SpawnUnit(LPEDICT self, struct UnitUI const *lpUnit) {
    struct UnitBalance const *balance = FindUnitBalance(self->class_id);
    struct UnitWeapons const *weapons = FindUnitWeapons(self->class_id);
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", lpUnit->file);
    self->s.model = gi.ModelIndex(buffer);
    self->think = monster_think;
    if (balance) {
        self->monsterinfo.health = balance->HP;
    }
    if (weapons) {
        
    }
}

void M_CheckGround(LPEDICT self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(LPEDICT self) {
    return false;
}
