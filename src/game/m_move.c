#include "g_local.h"
#include <stdlib.h>

#define DISTANCE (0.5 * FRAMETIME)
#define ATTACK_DISTANCE 200

void M_ChangeAngle(LPEDICT self) {
    VECTOR2 const dir = Vector2_sub((LPVECTOR2)&self->s.origin,
                                    (LPVECTOR2)&self->goalentity->s.origin);
    self->s.angle = atan2(-dir.y, -dir.x);
}


bool SV_CloseEnough(LPEDICT self, LPCEDICT goal, float distance) {
    float between = Vector2_distance((LPVECTOR2)&self->s.origin, (LPVECTOR2)&self->goalentity->s.origin);
    if (self->enemy) {
        if (between < ATTACK_DISTANCE) {
            self->goalentity = NULL;
            self->monsterinfo.melee(self);
            return true;
        } else {
            return false;
        }
    } else {
        if (between < distance) {
            self->goalentity = NULL;
            self->monsterinfo.stand(self);
            return true;
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
    if (SV_CloseEnough(self, self->goalentity, DISTANCE))
        return;
    SV_StepDirection(self, self->s.angle, DISTANCE);
}
