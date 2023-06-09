#include "g_local.h"

#define NAVI_THRESHOLD 50

void M_ChangeAngle(edict_t *self) {
    VECTOR2 dir;
    if (M_DistanceToGoal(self) > NAVI_THRESHOLD) {
        handle_t heatmap = M_RefreshHeatmap(self->goalentity);
        dir = gi.GetFlowDirection(heatmap, self->s.origin.x, self->s.origin.y);
    } else {
        dir = Vector2_sub(&self->goalentity->s.origin2, &self->s.origin2);
    }
    self->s.angle = atan2(dir.y, dir.x);
}

float M_MoveDistance(edict_t *self) {
    float speed = UNIT_SPEED(self->class_id);
    return 10 * speed / FRAMETIME;
}

void M_MoveInDirection(edict_t *self) {
    float const distance = M_MoveDistance(self);
    self->s.origin.x += cos(self->s.angle) * distance;
    self->s.origin.y += sin(self->s.angle) * distance;
}

void M_SetAnimation(edict_t *self, LPCSTR anim) {
    self->animation = gi.GetAnimation(self->s.model, anim);
}

void M_SetMove(edict_t *self, umove_t *move) {
    self->currentmove = move;
    self->animation = gi.GetAnimation(self->s.model, move->animation);
    if (!self->animation && strstr(move->animation, "stand ")) {
        self->animation = gi.GetAnimation(self->s.model, "stand");
    }
}

void M_RunWait(edict_t *self, void (*callback)(edict_t *)) {
    if (self->wait <= 0)
        return;
    if (self->wait > FRAMETIME / 1000.f) {
        self->wait -= FRAMETIME / 1000.f;
    } else {
        self->wait = 0;
        callback(self);
    }
}

void ai_stand(edict_t *self) {
}

void ai_birth(edict_t *self) {
}

void ai_pain(edict_t *self) {
}
