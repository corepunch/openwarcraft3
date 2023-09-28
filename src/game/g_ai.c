#include "g_local.h"

#define NAVI_THRESHOLD 50

void M_ChangeAngle(LPEDICT self) {
    VECTOR2 dir;
    if (M_DistanceToGoal(self) > NAVI_THRESHOLD) {
        DWORD heatmap = M_RefreshHeatmap(self->goalentity);
        dir = gi.GetFlowDirection(heatmap, self->s.origin.x, self->s.origin.y);
    } else {
        dir = Vector2_sub(&self->goalentity->s.origin2, &self->s.origin2);
    }
    self->s.angle = atan2(dir.y, dir.x);
}

FLOAT M_MoveDistance(LPEDICT self) {
    FLOAT speed = UNIT_SPEED(self->class_id);
    return 10 * speed / FRAMETIME;
}

void M_MoveInDirection(LPEDICT self) {
    FLOAT const distance = M_MoveDistance(self);

    self->s.origin.x += cos(self->s.angle) * distance;
    self->s.origin.y += sin(self->s.angle) * distance;

    gi.LinkEntity(self);
}

void M_SetAnimation(LPEDICT self, LPCSTR anim) {
    self->animation = gi.GetAnimation(self->s.model, anim);
}

void M_SetMove(LPEDICT self, umove_t *move) {
    self->currentmove = move;
    self->animation = gi.GetAnimation(self->s.model, move->animation);
    if (!self->animation && strstr(move->animation, "stand ")) {
        self->animation = gi.GetAnimation(self->s.model, "stand");
    }
}

umove_t const *M_GetCurrentMove(LPCEDICT ent) {
    return ent->currentmove;
}

void M_RunWait(LPEDICT self, void (*callback)(LPEDICT )) {
    if (self->wait <= 0)
        return;
    if (self->wait > FRAMETIME / 1000.f) {
        self->wait -= FRAMETIME / 1000.f;
    } else {
        self->wait = 0;
        callback(self);
    }
}

void ai_idle(LPEDICT self) {
}

void attack_start(LPEDICT self, LPEDICT target);

void ai_stand(LPEDICT self) {
    if (!(self->svflags & SVF_MONSTER))
        return;
    if (level.mapinfo->players[self->s.player].playerType != kPlayerTypeComputer)
        return;
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = g_edicts+i;
        if (!(ent->svflags & SVF_MONSTER) || ent->s.player == self->s.player)
            continue;
        if (level.alliances[ent->s.player][self->s.player] != 0)
            continue;
        if (level.mapinfo->players[ent->s.player].playerType != kPlayerTypeHuman)
            continue;
        FLOAT distance = Vector2_distance(&ent->s.origin2, &self->s.origin2);
        if (distance < self->balance.sight_radius.day) {
            attack_start(self, ent);
        }
    }
}

void ai_birth(LPEDICT self) {
}

void ai_pain(LPEDICT self) {
}
