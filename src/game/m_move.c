#include "g_local.h"
#include "Units/UnitWeapons.h"

#include <stdlib.h>

#define ATTACK_DISTANCE 200

void M_ChangeAngle(LPEDICT self) {
    VECTOR2 const dir = Vector2_sub((LPVECTOR2)&self->s.origin,
                                    (LPVECTOR2)&self->path->point);
    self->s.angle = atan2(-dir.y, -dir.x);
}


bool SV_CloseEnough(LPEDICT self, LPCEDICT goal, float distance) {
    if (self->enemy) {
        float between = Vector2_distance((LPVECTOR2)&self->s.origin, (LPVECTOR2)&self->goalentity->s.origin);
        if (between < self->monsterinfo.weapon->rangeN1) {
            self->goalentity = NULL;
            self->monsterinfo.melee(self);
            return true;
        } else {
            return false;
        }
    } else {
        float between = Vector2_distance((LPVECTOR2)&self->s.origin, (LPVECTOR2)&self->path->point);
        if (between < distance) {
            if (self->path->next) {
                self->path = self->path->next;
                return false;
            } else {
                self->path = NULL;
                self->monsterinfo.stand(self);
                return true;
            }
        } else {
            return false;
        }
    }
}

void SV_StepDirection(LPEDICT self, float yaw, float distance) {
    self->s.origin.x += cos(yaw) * distance;
    self->s.origin.y += sin(yaw) * distance;
}

void M_MoveToGoal(LPEDICT self) {
    if (SV_CloseEnough(self, self->goalentity, self->monsterinfo.movespeed))
        return;
    SV_StepDirection(self, self->s.angle, self->monsterinfo.movespeed);
}
