#include "s_skills.h"

void attack_walk(LPEDICT ent);
void attack_melee(LPEDICT ent);
void attack_cooldown(LPEDICT ent);

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

static void ai_damagetarget(LPEDICT ent) {
    LPEDICT other = ent->goalentity;
    DWORD damage = ai_rolldamage1(ent, 1);
    if (ent->attack1.weapon == WPN_MISSILE) {
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
//        gi.WriteByte (svc_temp_entity);
//        gi.WriteByte(TE_MISSILE);
//        gi.WritePosition(&origin);
//        gi.WriteShort(ent->attack1.projectile.model);
//        gi.WriteShort(ent->attack1.projectile.speed);
//        gi.WriteShort(Vector2_len(&dir) * 1000 / ent->attack1.projectile.speed);
//        gi.WriteAngle(atan2(dir.y, dir.x));
//        gi.multicast(&ent->s.origin, MULTICAST_PHS);
    } else {
        T_Damage(other, ent, damage);
    }
}

static void ai_cooldown(LPEDICT ent) {
    M_RunWait(ent, attack_melee);
}

static void ai_melee(LPEDICT ent) {
    M_ChangeAngle(ent);
    M_RunWait(ent, ai_damagetarget);
}

static void ai_walk(LPEDICT ent) {
    FLOAT range = UNIT_ATTACK1_RANGE(ent->class_id);
    if (M_DistanceToGoal(ent) < range) {
        attack_melee(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static umove_t attack_move_walk = { "walk", ai_walk, NULL, &a_attack };
static umove_t attack_move_cooldown = { "stand ready", ai_cooldown, NULL, &a_attack };
static umove_t attack_move_melee = { "attack", ai_melee, attack_cooldown, &a_attack };

void attack_start(LPEDICT self, LPEDICT target) {
    self->goalentity = target;
    M_SetMove(self, &attack_move_walk);
}

void attack_cooldown(LPEDICT self) {
    M_SetMove(self, &attack_move_cooldown);
    self->wait = self->attack1.cooldown;
}

void attack_melee(LPEDICT self) {
    M_SetMove(self, &attack_move_melee);
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
