#include "g_local.h"

#include <stdlib.h>

#define MAX_WAYPOINTS 256
static edict_t waypoints[MAX_WAYPOINTS];
DWORD current_waypoint = 0;

edict_t *Waypoint_add(LPCVECTOR2 spot) {
    edict_t *waypoint = &waypoints[current_waypoint++ % MAX_WAYPOINTS];
    waypoint->s.origin.x = spot->x;
    waypoint->s.origin.y = spot->y;
    M_CheckGround(waypoint);
    return waypoint;
}

void M_MoveFrame(edict_t *self) {
    if (self->unitinfo.aiflags & AI_HOLD_FRAME)
        return;
    umove_t const *move = self->unitinfo.currentmove;
    animation_t const *anim = self->animation;
    if (!anim)
        return;
    if (self->s.frame < anim->interval[0] ||
        self->s.frame >= anim->interval[1])
    {
        self->s.frame = anim->interval[0] ;
    } else if ((self->s.frame + FRAMETIME) >= anim->interval[1]) {
        SAFE_CALL(move->endfunc, self);
        if (!(self->unitinfo.aiflags & AI_HOLD_FRAME)) {
            self->s.frame = anim->interval[0] ;
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
    animation_t const *anim = self->animation;
    if (anim) {
        DWORD len = MAX(1, anim->interval[1] - anim->interval[0] - 1);
        self->s.frame = (anim->interval[0] + (rand() % len));
    }
}

//unitRace_t M_GetRace(LPCSTR string) {
//    if (!strcmp(string, STR_HUMAN)) return RACE_HUMAN;
//    if (!strcmp(string, STR_ORC)) return RACE_ORC;
//    if (!strcmp(string, STR_UNDEAD)) return RACE_UNDEAD;
//    if (!strcmp(string, STR_NIGHTELF)) return RACE_NIGHTELF;
//    if (!strcmp(string, STR_DEMON)) return RACE_DEMON;
//    if (!strcmp(string, STR_CREEPS)) return RACE_CREEPS;
//    if (!strcmp(string, STR_CRITTERS)) return RACE_CRITTERS;
//    if (!strcmp(string, STR_OTHER)) return RACE_OTHER;
//    if (!strcmp(string, STR_COMMONER)) return RACE_COMMONER;
//    return RACE_UNKNOWN;
//}

void SP_SpawnUnit(edict_t *self) {
    PATHSTR model_filename;
    sprintf(model_filename, "%s.mdx", UNIT_MODEL(self->class_id));
    self->s.model = gi.ModelIndex(model_filename);
    self->s.scale = UNIT_SCALING_VALUE(self->class_id);
    self->s.radius = UNIT_SELECTION_SCALE(self->class_id) * SEL_SCALE / 2;
    self->s.flags |= UNIT_SPEED(self->class_id) > 0 ? EF_MOVABLE : 0;
    self->health = UNIT_HP(self->class_id);
    self->think = monster_think;
}

void M_CheckGround(edict_t *self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(edict_t *self) {
    return false;
}

float M_DistanceToGoal(edict_t *ent) {
    if (ent->goalentity) {
        return Vector2_distance(&ent->goalentity->s.origin2, &ent->s.origin2);
    } else {
        return 0;
    }
}
