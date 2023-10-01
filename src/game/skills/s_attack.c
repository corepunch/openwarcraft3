#include "s_skills.h"

void attack_walk(LPEDICT ent);
void attack_melee(LPEDICT ent);
void attack_melee_cooldown(LPEDICT ent);
void attack_ranged(LPEDICT ent);
void attack_ranged_cooldown(LPEDICT ent);
void attack_start(LPEDICT self, LPEDICT target);

typedef struct {
    LPEDICT target;
    VECTOR3 start;
    VECTOR3 dir;
    DWORD speed;
    DWORD model;
    DWORD damage;
}  rocketDesc_t;

void fire_rocket(LPEDICT ent, rocketDesc_t const *desc) {
    VECTOR3 dir = Vector3_sub(&desc->target->s.origin, &ent->s.origin);
    Vector3_normalize(&dir);
    LPEDICT rocket = G_Spawn();
    rocket->s.origin = desc->start;
    rocket->s.angle = atan2f(dir.y, dir.x);
    rocket->s.model = desc->model;
    rocket->velocity = desc->speed / 1000.f;
    rocket->damage = desc->damage;
    rocket->goalentity = desc->target;
    rocket->owner = ent;
    rocket->movetype = MOVETYPE_FLYMISSILE;
    rocket->s.renderfx |= 64;
//    rocket->clipmask = MASK_SHOT;
//    rocket->solid = SOLID_BBOX;
//    rocket->s.effects |= EF_ROCKET;
//    VectorClear (rocket->mins);
//    VectorClear (rocket->maxs);
//    rocket->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
//    rocket->owner = self;
//    rocket->touch = rocket_touch;
//    rocket->nextthink = level.time + 8000/speed;
//    rocket->think = G_FreeEdict;
//    rocket->dmg = damage;
//    rocket->radius_dmg = radius_damage;
//    rocket->dmg_radius = damage_radius;
//    rocket->s.sound = gi.soundindex ("weapons/rockfly.wav");
//    rocket->classname = "rocket";
//
//    if (self->client)
//        check_dodge (self, rocket->s.origin, dir, speed);
//
//    gi.linkentity (rocket);
}

static FLOAT ai_rolldamage1(LPEDICT self, int weapon) {
    FLOAT damageBase = self->attack1.damageBase;
    FOR_LOOP(i, self->attack1.numberOfDice) {
        damageBase += rand() % self->attack1.sidesPerDie + 1;
    }
    return damageBase;
}

void M_GetEntityMatrix(LPCENTITYSTATE entity, LPMATRIX4 matrix) {
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &entity->origin);
    Matrix4_rotate(matrix, &(VECTOR3){0, 0, entity->angle * 180 / M_PI}, ROTATE_XYZ);
    Matrix4_scale(matrix, &(VECTOR3){entity->scale, entity->scale, entity->scale});
}

static BOOL can_attack(LPCEDICT ent) {
    if (ent->attack1.type == ATK_NONE)
        return false;
    if (!ent->currentmove || ent->currentmove->ability != &a_attack)
        return true;
    return false;
}

void T_Damage(LPEDICT target, LPEDICT attacker, int damage) {
    if (target->health.value <= damage) {
        target->health.value = 0;
        target->die(target, attacker);
        attacker->stand(attacker);
        return;
    } else {
        target->health.value -= damage;
    }
    if (can_attack(target)) {
        attack_start(target, attacker);
    } else if (target->pain) {
        target->pain(target);
    }
}

static void damage_target(LPEDICT ent) {
    LPEDICT other = ent->goalentity;
    DWORD damage = ai_rolldamage1(ent, 1);
    T_Damage(other, ent, damage);
}

static void throw_missile(LPEDICT ent) {
    LPEDICT other = ent->goalentity;
    DWORD damage = ai_rolldamage1(ent, 1);
    MATRIX4 matrix;
    M_GetEntityMatrix(&ent->s, &matrix);
    VECTOR3 origin = Matrix4_multiply_vector3(&matrix, &ent->attack1.origin);
    fire_rocket(ent, &(rocketDesc_t) {
        .start = origin,
        .target = other,
        .speed = ent->attack1.projectile.speed,
        .model = ent->attack1.projectile.model,
        .damage = damage,
    });
//    gi.WriteByte (svc_temp_entity);
//    gi.WriteByte(TE_MISSILE);
//    gi.WritePosition(&origin);
//    gi.WriteShort(ent->attack1.projectile.model);
//    gi.WriteShort(ent->attack1.projectile.speed);
//    gi.WriteShort(Vector2_len(&dir) * 1000 / ent->attack1.projectile.speed);
//    gi.WriteAngle(atan2(dir.y, dir.x));
//    gi.multicast(&ent->s.origin, MULTICAST_PHS);
}


static void ai_melee(LPEDICT ent) {
    M_ChangeAngle(ent);
    M_RunWait(ent, damage_target);
}

static void ai_ranged(LPEDICT ent) {
    M_ChangeAngle(ent);
    M_RunWait(ent, throw_missile);
}

static void ai_melee_cooldown(LPEDICT ent) {
    if (M_DistanceToGoal(ent) > ent->attack1.range) {
        attack_walk(ent);
    } else {
        M_RunWait(ent, attack_melee);
    }
}

static void ai_ranged_cooldown(LPEDICT ent) {
    if (M_DistanceToGoal(ent) > ent->attack1.range) {
        attack_walk(ent);
    } else {
        M_RunWait(ent, attack_ranged);
    }
}

static void ai_walk(LPEDICT ent) {
    if (M_DistanceToGoal(ent) > ent->attack1.range) {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    } else if (ent->attack1.weapon == WPN_MISSILE) {
        attack_ranged(ent);
    } else {
        attack_melee(ent);
    }
}

static umove_t attack_move_walk = { "walk", ai_walk, NULL, &a_attack };
static umove_t attack_move_melee_cooldown = { "stand ready", ai_melee_cooldown, NULL, &a_attack };
static umove_t attack_move_melee = { "attack", ai_melee, attack_melee_cooldown, &a_attack };
static umove_t attack_move_ranged_cooldown = { "stand ready", ai_ranged_cooldown, NULL, &a_attack };
static umove_t attack_move_ranged = { "attack range", ai_ranged, attack_ranged_cooldown, &a_attack };

void attack_walk(LPEDICT self) {
    M_SetMove(self, &attack_move_walk);
}

void attack_start(LPEDICT self, LPEDICT target) {
    self->goalentity = target;
    attack_walk(self);
}

void attack_melee_cooldown(LPEDICT self) {
    M_SetMove(self, &attack_move_melee_cooldown);
    self->wait = self->attack1.cooldown;
}

void attack_melee(LPEDICT self) {
    M_SetMove(self, &attack_move_melee);
    self->wait = self->attack1.damagePoint;
}

void attack_ranged_cooldown(LPEDICT self) {
    M_SetMove(self, &attack_move_ranged_cooldown);
    self->wait = self->attack1.cooldown;
}

void attack_ranged(LPEDICT self) {
    M_SetMove(self, &attack_move_ranged);
    self->wait = self->attack1.damagePoint;
}

BOOL attack_menu_selecttarget(LPEDICT ent, LPEDICT target) {
    if (target->targtype == TARG_GROUND) {
        FOR_SELECTED_UNITS(ent->client, e) {
            attack_start(e, target);
        }
        return true;
    } else {
        return false;
    }
}

void attack_command(LPEDICT ent) {
    UI_AddCancelButton(ent);
    ent->client->menu.on_entity_selected = attack_menu_selecttarget;
}

ability_t a_attack = {
    .cmd = attack_command,
};
