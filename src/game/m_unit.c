#include "g_local.h"

void M_SetAnimation(LPEDICT e, LPCSTR anim) {
    e->monsterinfo.animation = gi.GetAnimation(e->s.model, anim);
    e->s.frame = e->monsterinfo.animation.start_frame;
}

void M_MakeAnimation(LPEDICT e, LPCSTR anim, MONSTERSTATE state) {
    if (e->state != state) {
        M_SetAnimation(e, anim);
        e->state = state;
    }
}

void M_PlayAnimation(LPEDICT e, DWORD msec) {
    struct AnimationInfo const *anim = &e->monsterinfo.animation;
    if (anim->start_frame == anim->end_frame)
        return;
    e->s.frame += msec;
    while (e->s.frame >= anim->end_frame) {
        e->s.frame -= MAX(1, anim->end_frame - anim->start_frame);
    }
}

void unit_walk(LPEDICT e, DWORD msec) {
    const float DISTANCE = 0.5 * msec;
    const float TARGET = Vector2_distance((LPVECTOR2)&e->s.origin, &e->objective);
    if (TARGET < DISTANCE) {
        M_MakeAnimation(e, "Stand", MS_STAND);
        e->objective = (VECTOR2) {0,0};
        return;
    }
    VECTOR2 dir = {
        e->s.origin.x - e->objective.x,
        e->s.origin.y - e->objective.y,
    };
    Vector2_normalize(&dir);
    e->s.angle = atan2(-dir.y, -dir.x);
    e->s.origin.x += cos(e->s.angle) * DISTANCE;
    e->s.origin.y += sin(e->s.angle) * DISTANCE;
}

void unit_stand(LPEDICT e, DWORD msec) {
    if (e->objective.x != 0) {
        M_MakeAnimation(e, "Walk", MS_MOVE);
    }
}

void unit_think(LPEDICT e, DWORD msec) {
    M_PlayAnimation(e, msec);
    switch (e->state) {
        case MS_MOVE:
            unit_walk(e, msec);
            break;
        case MS_STAND:
            unit_stand(e, msec);
            break;
    }
    e->s.origin.z = gi.GetHeightAtPoint(e->s.origin.x, e->s.origin.y);
}

void SP_SpawnUnit(LPEDICT lpEdict, LPCUNITUI lpUnit) {
    PATHSTR buffer;
    sprintf(buffer, "%s.mdx", lpUnit->file);
    lpEdict->s.model = gi.ModelIndex(buffer);
    lpEdict->think = unit_think;
    M_SetAnimation(lpEdict, "Stand");
}
