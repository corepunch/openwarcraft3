#include "g_local.h"

#define NAVI_THRESHOLD 50

void unit_changeangle(LPEDICT self) {
    VECTOR2 dir;
    if (M_DistanceToGoal(self) > NAVI_THRESHOLD) {
        DWORD heatmap = M_RefreshHeatmap(self->goalentity);
        dir = gi.GetFlowDirection(heatmap, self->s.origin.x, self->s.origin.y);
    } else {
        dir = Vector2_sub(&self->goalentity->s.origin2, &self->s.origin2);
    }
    self->s.angle = atan2(dir.y, dir.x);
}

FLOAT unit_movedistance(LPEDICT self) {
    FLOAT speed = UNIT_SPEED(self->class_id);
    return 10 * speed / FRAMETIME;
}

void unit_moveindirection(LPEDICT self) {
    G_PushEntity(self,
                 unit_movedistance(self),
                 &MAKE(VECTOR2, cos(self->s.angle), sin(self->s.angle)));
}

void unit_setanimation(LPEDICT self, LPCSTR anim) {
    self->animation = gi.GetAnimation(self->s.model, anim);
}

void unit_setmove(LPEDICT self, umove_t *move) {
    self->currentmove = move;
    self->animation = gi.GetAnimation(self->s.model, move->animation);
    if (self->animation) {
        // skip
    } else if (strstr(move->animation, "stand ")) {
        self->animation = gi.GetAnimation(self->s.model, "stand");
    } else if (strstr(move->animation, "attack ")) {
        self->animation = gi.GetAnimation(self->s.model, "attack");
    }
}

void unit_runwait(LPEDICT self, void (*callback)(LPEDICT )) {
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

void order_attack(LPEDICT self, LPEDICT target);

#define MAX_SIGHT_ENTITIES 32

static LPEDICT current_entity = NULL;
static LPEDICT sight_entities[MAX_SIGHT_ENTITIES];

static BOOL filter_sight(LPCEDICT ent) {
    if (!(ent->svflags & SVF_MONSTER) || ent->s.player == current_entity->s.player)
        return false;
    if (level.alliances[ent->s.player][current_entity->s.player] != 0)
        return false;
    if (level.mapinfo->players[ent->s.player].playerType != kPlayerTypeHuman)
        return false;
    if (ent->svflags & SVF_DEADMONSTER)
        return false;
    if (UNIT_IS_BUILDING(ent->class_id))
        return false;
    return true;
}

void ai_stand(LPEDICT self) {
    if (!(self->svflags & SVF_MONSTER))
        return;
    if (level.mapinfo->players[self->s.player].playerType != kPlayerTypeComputer)
        return;
    current_entity = self;
    FLOAT const sight = self->balance.sight_radius.day / 2;
    BOX2 const sightbox = {
        { self->s.origin2.x - sight, self->s.origin2.y - sight },
        { self->s.origin2.x + sight, self->s.origin2.y + sight },
    };
    DWORD numents = gi.BoxEdicts(&sightbox, sight_entities, MAX_SIGHT_ENTITIES, filter_sight);
    FOR_LOOP(i, numents) {
        LPEDICT ent = sight_entities[i];
        if (Vector2_distance(&ent->s.origin2, &self->s.origin2) < sight) {
            order_attack(self, ent);
        }
    }
}

void ai_birth(LPEDICT self) {
}

void ai_pain(LPEDICT self) {
}
