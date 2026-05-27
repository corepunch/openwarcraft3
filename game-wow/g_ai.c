#include "g_wow_local.h"
#include <math.h>

static BOOL Wow_SetFirstAnimation(LPEDICT ent, LPCSTR const *names) {
    for (LPCSTR const *name = names; name && *name; name++) {
        if (Wow_SetEntityAnimation(ent, *name)) {
            return true;
        }
    }
    return false;
}

static void Wow_FaceTarget(LPEDICT ent, LPEDICT target) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    VECTOR2 delta;

    if (!ent || !target || !local) {
        return;
    }

    delta = Vector2_sub(&target->s.origin2, &ent->s.origin2);
    if (Vector2_len(&delta) <= 0.001f) {
        return;
    }
    local->yaw = (FLOAT)RAD2DEG(atan2f(delta.y, delta.x));
    ent->s.rotation = (VECTOR3){ local->yaw, 0.0f, 0.0f };
}

void Wow_AIIdle(LPEDICT ent) {
    Wow_SetEntityAnimation(ent, "Stand");
}

void Wow_AIMove(LPEDICT ent) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    VECTOR2 target;
    VECTOR2 delta;
    FLOAT len;
    FLOAT step;

    if (!ent || !local) {
        return;
    }

    target = (VECTOR2){
        local->home.x + cosf(local->patrol_phase) * local->patrol_radius,
        local->home.y + sinf(local->patrol_phase) * local->patrol_radius,
    };
    delta = Vector2_sub(&target, &ent->s.origin2);
    len = Vector2_len(&delta);
    if (len <= 0.001f) {
        return;
    }

    step = MIN(local->walk_speed * ((FLOAT)FRAMETIME / 1000.0f), len);
    ent->s.origin.x += delta.x * step / len;
    ent->s.origin.y += delta.y * step / len;
    local->yaw = (FLOAT)RAD2DEG(atan2f(delta.y, delta.x));
    ent->s.rotation = (VECTOR3){ local->yaw, 0.0f, 0.0f };
    Wow_SetEntityAnimation(ent, "Walk");
}

void Wow_AIAttack(LPEDICT ent) {
    static LPCSTR const attack_animations[] = {
        "Attack1H",
        "AttackUnarmed",
        "Attack2H",
        "Attack",
        NULL,
    };
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    LPEDICT target;

    if (!ent || !local) {
        return;
    }

    target = local->enemy;
    if (!target || !target->inuse || target == ent) {
        local->enemy = NULL;
        target = NULL;
    }

    if (target) {
        Wow_FaceTarget(ent, target);
    }
    local->attack_time = 700;
    Wow_SetFirstAnimation(ent, attack_animations);
    if (target && target->pain) {
        target->pain(target);
    }
}

void Wow_AIPain(LPEDICT ent) {
    static LPCSTR const pain_animations[] = {
        "CombatWound",
        "StandWound",
        "Stun",
        NULL,
    };
    wowEntityLocal_t *local = Wow_EntityLocal(ent);

    if (!ent || !local) {
        return;
    }

    if (local->health > 0) {
        local->health--;
    }
    local->pain_time = 450;
    Wow_SetFirstAnimation(ent, pain_animations);
}

BOOL Wow_AIAdvanceLockedFrame(LPEDICT ent) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    DWORD *timer = NULL;

    if (!ent || !local) {
        return false;
    }

    if (local->attack_time > 0) {
        timer = &local->attack_time;
    } else if (local->pain_time > 0) {
        timer = &local->pain_time;
    }
    if (!timer) {
        return false;
    }

    if (*timer > FRAMETIME) {
        *timer -= FRAMETIME;
    } else {
        *timer = 0;
    }
    Wow_AdvanceEntityFrame(ent);
    return true;
}

void Wow_AIRunFrame(LPEDICT ent) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);

    if (!ent || !local) {
        return;
    }

    if (Wow_AIAdvanceLockedFrame(ent)) {
        return;
    }

    ent->s.origin2 = (VECTOR2){ ent->s.origin.x, ent->s.origin.y };
    if (local->patrol_radius > 0.0f && local->walk_speed > 0.0f) {
        local->patrol_phase += ((FLOAT)FRAMETIME / 1000.0f) * 0.6f;
        if (ent->move) {
            ent->move(ent);
        }
    } else if (ent->idle) {
        ent->idle(ent);
    }

    Wow_AdvanceEntityFrame(ent);
}

void Wow_RunCreatureFrame(LPEDICT ent) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);

    if (!ent || !local || local->kind != WOW_ENTITY_CREATURE) {
        return;
    }
    if (ent->run) {
        ent->run(ent);
    }
}
