#include "g_local.h"

void SV_Physics_Step(edict_t *ent) {
    M_CheckGround(ent);
}

void SV_Physics_Toss(edict_t *ent) {
    float distance = ent->velocity * FRAMETIME;
    VECTOR3 dir = Vector3_sub(&ent->goalentity->s.origin, &ent->s.origin);
    if (Vector3_len(&dir) < distance) {
        T_Damage(ent->goalentity, ent->owner, ent->damage);
        G_FreeEdict(ent);
    } else {
        Vector3_normalize(&dir);
        ent->s.origin = Vector3_mad(&ent->s.origin, distance, &dir);
    }
}

DWORD M_GetBuildEndTime(DWORD start, DWORD class_id) {
    return start + UNIT_BUILD_TIME(class_id) * 1000;
}

void G_GetNextBuildFromQueue(edict_t *ent) {
    FOR_LOOP(q, MAX_BUILD_QUEUE-1) {
        ent->build.queue[q] = ent->build.queue[q+1];
    }
    ent->build.queue[MAX_BUILD_QUEUE-1] = 0;
    if (ent->build.queue[0]) {
        ent->build.start = gi.GetTime();
        ent->build.end = gi.GetTime() + UNIT_BUILD_TIME(ent->build.queue[0]) * 1000;
    }
}

void G_ProcessBuilds(edict_t *ent) {
    if (ent->build.queue[0] && ent->build.end <= gi.GetTime()) {
        SP_SpawnTrainedUnit(ent, ent->build.queue[0]);
        G_GetNextBuildFromQueue(ent);
    }
}

void G_RunEntity(edict_t *ent) {
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
    G_ProcessBuilds(ent);
    SAFE_CALL(ent->think, ent);
    ent->s.stats[ENT_HEALTH] = compress_stat(&ent->health);
    ent->s.stats[ENT_MANA] = compress_stat(&ent->mana);
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
