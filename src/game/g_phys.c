#include "g_local.h"

#define IS_HOLLOW(ent) (ent->health.value <= 0 || ent->s.renderfx & RF_HIDDEN || !ent->s.model || !ent->inuse)
#define IS_STATIC(ent) (ent->movetype == MOVETYPE_NONE)
#define IS_MOVING(ent) (ent->currentmove->ability == &a_move)

extern ability_t a_move;

void SV_Physics_Step(LPEDICT ent) {
    M_CheckGround(ent);
}

void SV_Physics_Toss(LPEDICT ent) {
    FLOAT distance = ent->velocity * FRAMETIME;
    VECTOR3 dir = Vector3_sub(&ent->goalentity->s.origin, &ent->s.origin);
    if (Vector3_len(&dir) < distance) {
        T_Damage(ent->goalentity, ent->owner, ent->damage);
        G_FreeEdict(ent);
    } else {
        Vector3_normalize(&dir);
        ent->s.origin = Vector3_mad(&ent->s.origin, distance, &dir);
    }
}

void G_RunEntity(LPEDICT ent) {
    SAFE_CALL(ent->prethink, ent);
    switch (ent->movetype) {
        case MOVETYPE_STEP:
            SV_Physics_Step(ent);
            break;
        case MOVETYPE_FLYMISSILE:
            SV_Physics_Toss(ent);
            break;
        default:
//            gi.error("SV_Physics: bad movetype %d", edict->movetype);
            break;
    }
    SAFE_CALL(ent->think, ent);
    ent->s.stats[ENT_HEALTH] = compress_stat(&ent->health);
    ent->s.stats[ENT_MANA] = compress_stat(&ent->mana);
    if (M_GetCurrentMove(ent)) {
        ent->s.ability = GetAbilityIndex(M_GetCurrentMove(ent)->ability);
    } else {
        ent->s.ability = 0;
    }
}

inline BOOL M_CheckCollision(LPCVECTOR2 origin, FLOAT radius) {
    for (LPEDICT a = globals.edicts; a - globals.edicts < globals.num_edicts; a++) {
        VECTOR2 d = Vector2_sub(&a->s.origin2, origin);
        if (IS_HOLLOW(a))
            continue;
        if (IS_STATIC(a))
            continue;
        if (Vector2_len(&d) < radius + a->collision)
            return true;
    }
    return false;
}

void G_SolveCollisions(void) {
    for (LPEDICT a = globals.edicts; a - globals.edicts < globals.num_edicts; a++) {
        if (IS_HOLLOW(a))
            continue;
        BOOL const a_static = IS_STATIC(a);
        for (LPEDICT b = a+1; b - globals.edicts < globals.num_edicts; b++) {
            BOOL const b_static = IS_STATIC(b);
            if (IS_HOLLOW(b))
                continue;
            if (a_static && b_static)
                continue;
            FLOAT const radius = (a->collision + b->collision);
            if (fabs(a->s.origin.x - b->s.origin.x) >= radius)
                continue;
            if (fabs(a->s.origin.y - b->s.origin.y) >= radius)
                continue;
            FLOAT const distance = Vector2_distance(&a->s.origin2, &b->s.origin2);
            if (distance >= radius)
                continue;
            VECTOR2 d = Vector2_sub(&a->s.origin2, &b->s.origin2);
            Vector2_normalize(&d);
            FLOAT const diff = distance - radius;
            if (a_static) {
                b->s.origin2 = Vector2_mad(&b->s.origin2, diff, &d);
            } else if (b_static) {
                a->s.origin2 = Vector2_mad(&a->s.origin2, -diff, &d);
            } else if (IS_MOVING(a) && IS_MOVING(b)) {
                FLOAT const ad = M_DistanceToGoal(a);
                FLOAT const bd = M_DistanceToGoal(b);
                a->s.origin2 = Vector2_mad(&a->s.origin2, -diff * ad / (ad + bd), &d);
                b->s.origin2 = Vector2_mad(&b->s.origin2, diff * bd / (ad + bd), &d);
            } else {
                a->s.origin2 = Vector2_mad(&a->s.origin2, -diff * 0.5f, &d);
                b->s.origin2 = Vector2_mad(&b->s.origin2, diff * 0.5f, &d);
                // one of the colliders reached the point?
                // then stop the other one as well
                if (a->goalentity == b->goalentity) {
                    if (IS_MOVING(a)) a->stand(a);
                    if (IS_MOVING(b)) b->stand(b);
                }
            }
        }
    }
}
