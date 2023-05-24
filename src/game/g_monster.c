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

//void G_FindPath(LPEDICT edict, LPCVECTOR2 location) {
//    pathPoint_t *path = gi.FindPath((LPCVECTOR2)&edict->s.origin, location);
//    int a =0;
//}

void M_MoveFrame(LPEDICT self) {
    if (self->monsterinfo.aiflags & AI_HOLD_FRAME)
        return;
    mmove_t const *move = self->monsterinfo.currentmove;
    LPCANIMATION anim = gi.GetAnimation(self->s.model, move->animation);
    if (!anim)
        return;
    if (self->s.frame < anim->firstframe || self->s.frame >= anim->lastframe) {
        self->s.frame = anim->firstframe;
    } else if ((self->s.frame + FRAMETIME) >= anim->lastframe) {
        SAFE_CALL(move->endfunc, self);
        if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME)) {
            self->s.frame = anim->firstframe;
        }
    } else {
        self->s.frame += FRAMETIME;
    }
}

void monster_think(LPEDICT self) {
    if (!self->monsterinfo.currentmove)
        return;
    M_MoveFrame(self);
    if (self->monsterinfo.currentmove->think) {
        self->monsterinfo.currentmove->think(self);
    }
}

void monster_start(LPEDICT self) {
    if (self->monsterinfo.currentmove) {
        LPCANIMATION anim = gi.GetAnimation(self->s.model, self->monsterinfo.currentmove->animation);
        if (anim) {
            self->s.frame = anim->firstframe + (rand() % (anim->lastframe - anim->firstframe + 1));
        }
    }
    if (!self->monsterinfo.checkattack) {
        self->monsterinfo.checkattack = M_CheckAttack;
    }
}

void SP_SpawnUnit(LPEDICT self, struct UnitUI const *unit) {
    self->monsterinfo.balance = FindUnitBalance(self->class_id);
    self->monsterinfo.weapon = FindUnitWeapons(self->class_id);
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", unit->file);
    self->s.model = gi.ModelIndex(buffer);
    self->s.scale = unit->modelScale;
    self->think = monster_think;
    if (self->monsterinfo.balance) {
        self->monsterinfo.health = self->monsterinfo.balance->HP;
        self->monsterinfo.movespeed = 10 * unit->run / FRAMETIME;
    }
}

void M_CheckGround(LPEDICT self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(LPEDICT self) {
    return false;
}
