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
}

void G_SolveCollisions(void) {
//    float allowed_dist = 100;
//    float allowed_dist_sq = allowed_dist * allowed_dist;
    for (DWORD a = 0; a < globals.num_edicts; a++) {
        edict_t *ea = &globals.edicts[a];
        LPVECTOR2 apos = (LPVECTOR2)&ea->s.origin;
        if (!ea->s.model)
            continue;
        for (DWORD b = a + 1; b < globals.num_edicts; b++) {
            edict_t *eb = &globals.edicts[b];
            LPVECTOR2 bpos = (LPVECTOR2)&eb->s.origin;
            VECTOR2 d = Vector2_sub(apos, bpos);
            if (!eb->s.model)
                continue;
            if (!(ea->s.flags & EF_MOVABLE) && !(eb->s.flags & EF_MOVABLE))
                continue;
            float const distance = Vector2_len(&d);
            float const radius = ea->s.radius + eb->s.radius;
            if (distance < radius) {
                Vector2_normalize(&d);
                float const diff = distance - radius;
                if ((ea->s.flags & EF_MOVABLE) && (eb->s.flags & EF_MOVABLE)) {
                    if (ea->goalentity && eb->goalentity) {
                        float ad = Vector2_distance(apos, (LPCVECTOR2)&ea->goalentity->s.origin);
                        float bd = Vector2_distance(bpos, (LPCVECTOR2)&eb->goalentity->s.origin);
                        *apos = Vector2_mad(apos, -diff * ad / (ad + bd), &d);
                        *bpos = Vector2_mad(bpos, diff * bd / (ad + bd), &d);
                    } else {
                        *apos = Vector2_mad(apos, -diff * 0.5f, &d);
                        *bpos = Vector2_mad(bpos, diff * 0.5f, &d);
                    }
                } else if (ea->s.flags & EF_MOVABLE) {
                    *apos = Vector2_mad(apos, -diff, &d);
                } else {
                    *bpos = Vector2_mad(bpos, diff, &d);
                }
            }
        }
    }
}
