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

unitRace_t M_GetRace(LPCSTR string) {
    if (!strcmp(string, STR_HUMAN)) return RACE_HUMAN;
    if (!strcmp(string, STR_ORC)) return RACE_ORC;
    if (!strcmp(string, STR_UNDEAD)) return RACE_UNDEAD;
    if (!strcmp(string, STR_NIGHTELF)) return RACE_NIGHTELF;
    if (!strcmp(string, STR_DEMON)) return RACE_DEMON;
    if (!strcmp(string, STR_CREEPS)) return RACE_CREEPS;
    if (!strcmp(string, STR_CRITTERS)) return RACE_CRITTERS;
    if (!strcmp(string, STR_OTHER)) return RACE_OTHER;
    if (!strcmp(string, STR_COMMONER)) return RACE_COMMONER;
    return RACE_UNKNOWN;
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
    self->s.flags |= data->isbldg ? 0 : EF_IS_UNIT;
    self->unitinfo.ui = data;
}

void M_SetFlags(edict_t *self) {
    if (self->unitinfo.ui->run > 0) self->s.flags |= EF_CAN_MOVE;
    if (self->unitinfo.weapon) self->s.flags |= EF_CAN_ATTACK;
    self->s.flags |= M_GetRace(self->unitinfo.data->race) << RACE_BIT;
}

void SP_SpawnUnit(edict_t *self, struct UnitUI const *unit) {
    M_ParseUnitUI(self, unit);
    M_ParseAbilities(self, FindUnitAbilities(self->s.class_id));
    M_ParseBalance(self, FindUnitBalance(self->s.class_id));
    M_ParseWeapon(self, FindUnitWeapons(self->s.class_id));
    M_ParseData(self, FindUnitData(self->s.class_id));
    M_SetFlags(self);

    self->think = monster_think;
}

void M_CheckGround(edict_t *self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(edict_t *self) {
    return false;
}
