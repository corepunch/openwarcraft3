/*
 * g_doodad.c — Scripted animation control for map doodads.
 *
 * Doodads are static scenery entities rather than destructables: JASS cannot
 * hold a doodad handle, so Warcraft exposes area-based SetDoodadAnimation*
 * natives instead.  A scripted animation temporarily gives the matching
 * scenery entity a simulation animation clock; it does not change pathing or
 * convert the doodad into a destructable.
 */
#include "g_local.h"
#include <float.h>

static umove_t doodad_scripted_move = { "stand", NULL, G_DoodadAnimationEnd };

BOOL G_IsDoodad(LPCEDICT ent) {
    return ent && ent->inuse && ent->class_id && (ent->svflags & SVF_STATIC_SCENERY) &&
        ent->data.Doodads && ent->data.Doodads->id == ent->class_id;
}

void G_DoodadAnimationEnd(LPEDICT ent) {
    LPCANIMATION anim;

    if (!ent || !(anim = ent->animation)) return;
    if (!(anim->flags & 1)) return;

    /* Non-looping doodad animations persist on their authored last frame.
     * Prologue01's LOo2 banner uses this to leave the flag gone after its
     * Death sequence has emitted the smoke puff. */
    if (anim->interval[1] > anim->interval[0])
        ent->s.frame = anim->interval[1] - 1;
    ent->aiflags |= AI_HOLD_FRAME;
}

static BOOL G_DoodadSetAnimation(LPEDICT ent, LPCSTR anim_name, BOOL random_animation) {
    LPCANIMATION anim;

    if (!G_IsDoodad(ent) || !anim_name || !*anim_name) return false;

    /* Retail exposes these two special animation names for doodads.  They are
     * presentation-only and deliberately do not affect the doodad footprint. */
    if (!strcasecmp(anim_name, "hide")) {
        ent->s.renderfx |= RF_HIDDEN;
        return true;
    }
    if (!strcasecmp(anim_name, "show")) {
        ent->s.renderfx &= ~RF_HIDDEN;
        return true;
    }

    anim = G_GetAnimationVariant(ent->s.model, anim_name, random_animation);
    if (!anim) return false;

    /* Persist the resolved sequence name so an animRandom choice survives save/load. */
    strlcpy(ent->animation_request, anim->name, sizeof(ent->animation_request));
    ent->animation = anim;
    ent->currentmove = &doodad_scripted_move;
    ent->aiflags &= ~AI_HOLD_FRAME;
    ent->s.frame = anim->interval[0];
    ent->think = monster_think;
    return true;
}

DWORD G_SetDoodadAnimationRadius(FLOAT x, FLOAT y, FLOAT radius, DWORD doodad_id,
                                 BOOL nearest_only, LPCSTR anim_name, BOOL random_animation) {
    LPEDICT nearest = NULL;
    FLOAT nearest_distance_sq = FLT_MAX;
    DWORD changed = 0;

    if (radius < 0.0f || !doodad_id || !anim_name || !*anim_name) return 0;

    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = g_edicts + i;
        FLOAT dx, dy, distance_sq;

        if (!G_IsDoodad(ent) || ent->class_id != doodad_id) continue;
        dx = ent->s.origin.x - x;
        dy = ent->s.origin.y - y;
        distance_sq = dx * dx + dy * dy;
        if (distance_sq > radius * radius) continue;

        if (nearest_only) {
            if (!nearest || distance_sq < nearest_distance_sq) {
                nearest = ent;
                nearest_distance_sq = distance_sq;
            }
            continue;
        }
        if (G_DoodadSetAnimation(ent, anim_name, random_animation)) changed++;
    }

    if (nearest && G_DoodadSetAnimation(nearest, anim_name, random_animation)) changed++;
    return changed;
}

DWORD G_SetDoodadAnimationRect(LPCBOX2 rect, DWORD doodad_id,
                               LPCSTR anim_name, BOOL random_animation) {
    DWORD changed = 0;

    if (!rect || !doodad_id || !anim_name || !*anim_name) return 0;

    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = g_edicts + i;

        if (!G_IsDoodad(ent) || ent->class_id != doodad_id ||
            !Box2_containsPoint(rect, &ent->s.origin2))
            continue;
        if (G_DoodadSetAnimation(ent, anim_name, random_animation)) changed++;
    }
    return changed;
}
