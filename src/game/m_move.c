#include "g_local.h"
#include "Units/UnitWeapons.h"
#include "Units/UnitUI.h"

#include <stdlib.h>

#define NAVI_THRESHOLD 50

void M_ChangeAngle(LPEDICT self) {
    VECTOR2 dir;
    if (!self->goalentity) {
        dir = Vector2_sub(&self->enemy->s.origin2, &self->s.origin2);
    } else if (Vector2_len(&dir) > NAVI_THRESHOLD) {
        dir = gi.GetFlowDirection(self->heatmap, self->s.origin.x, self->s.origin.y);
    } else {
        dir = Vector2_sub(&self->goalentity->s.origin2, &self->s.origin2);
    }
    self->s.angle = atan2(dir.y, dir.x);
}


bool SV_CloseEnough(LPEDICT self, LPCEDICT goal, float distance) {
    if (self->enemy) {
        float between = Vector2_distance(&self->s.origin2, &self->enemy->s.origin2);
        if (between < self->unitinfo.weapon->rangeN1) {
            self->goalentity = NULL;
            self->unitinfo.melee(self);
            return true;
        } else {
            return false;
        }
    } else {
        float between = Vector2_distance(&self->s.origin2, &self->goalentity->s.origin2);
        if (between < distance) {
//            if (self->path->next) {
//                self->path = self->path->next;
//                return false;
//            } else {
//            self->path = NULL;
            self->goalentity = NULL;
            self->unitinfo.stand(self);
            return true;
//            }
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
    float const distance = 10 * self->unitinfo.ui->run / FRAMETIME;
    if (SV_CloseEnough(self, self->goalentity, distance))
        return;
    SV_StepDirection(self, self->s.angle, distance);
}
