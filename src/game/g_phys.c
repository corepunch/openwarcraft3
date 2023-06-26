#include "g_local.h"

void SV_Physics_Step(edict_t *edict) {
    M_CheckGround(edict);
}

void G_RunEntity(edict_t *edict) {
    SAFE_CALL(edict->prethink, edict);
    switch (edict->movetype) {
        case MOVETYPE_STEP:
            SV_Physics_Step(edict);
            break;
        default:
//            gi.error("SV_Physics: bad movetype %d", edict->movetype);
            break;
    }
    SAFE_CALL(edict->think, edict);
    
    edict->s.health = edict->health / MAX(1, edict->max_health);
}

bool M_IsHollow(edict_t *ent) {
    return M_IsDead(ent) || ent->s.renderfx & RF_HIDDEN;
}

bool M_CheckCollision(LPCVECTOR2 origin, float radius) {
    for (edict_t *a = globals.edicts; a - globals.edicts < globals.num_edicts; a++) {
        VECTOR2 d = Vector2_sub(&a->s.origin2, origin);
        if (!a->s.model || M_IsHollow(a))
            continue;
        if (!(a->s.flags & EF_MOVABLE))
            continue;
        if (Vector2_len(&d) < radius + a->collision)
            return true;
    }
    return false;
}

void G_SolveCollisions(void) {
    for (edict_t *a = globals.edicts; a - globals.edicts < globals.num_edicts; a++) {
        if (!a->s.model || M_IsHollow(a))
            continue;
        for (edict_t *b = a+1; b - globals.edicts < globals.num_edicts; b++) {
            VECTOR2 d = Vector2_sub(&a->s.origin2, &b->s.origin2);
            if (!b->s.model || M_IsHollow(b))
                continue;
            if (!(a->s.flags & EF_MOVABLE) && !(b->s.flags & EF_MOVABLE))
                continue;
            float const distance = Vector2_len(&d);
            float const radius = (a->collision + b->collision) * 0.85;
            if (distance >= radius)
                continue;
            Vector2_normalize(&d);
            float const diff = distance - radius;
            if ((a->s.flags & EF_MOVABLE) && (b->s.flags & EF_MOVABLE)) {
                if (a->goalentity && b->goalentity) {
                    float const ad = M_DistanceToGoal(a);
                    float const bd = M_DistanceToGoal(b);
                    a->s.origin2 = Vector2_mad(&a->s.origin2, -diff * ad / (ad + bd), &d);
                    b->s.origin2 = Vector2_mad(&b->s.origin2, diff * bd / (ad + bd), &d);
                } else {
                    a->s.origin2 = Vector2_mad(&a->s.origin2, -diff * 0.5f, &d);
                    b->s.origin2 = Vector2_mad(&b->s.origin2, diff * 0.5f, &d);
                }
            } else if (a->s.flags & EF_MOVABLE) {
                a->s.origin2 = Vector2_mad(&a->s.origin2, -diff, &d);
            } else {
                b->s.origin2 = Vector2_mad(&b->s.origin2, diff, &d);
            }
        }
    }
}
