#include "g_local.h"

EDICT_FUNC(attack_walk);
EDICT_FUNC(attack_melee);
EDICT_FUNC(attack_cooldown);

typedef struct {
    edict_t *target;
    VECTOR3 start;
    VECTOR3 dir;
    DWORD speed;
    DWORD model;
    DWORD damage;
}  rocketDesc_t;

void fire_rocket(edict_t *ent, rocketDesc_t const *desc) {
    VECTOR3 dir = Vector3_sub(&desc->target->s.origin, &ent->s.origin);
    Vector3_normalize(&dir);
    edict_t *rocket = G_Spawn();
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

static float ai_rolldamage1(edict_t *self, int weapon) {
    float damageBase = self->attack1.damageBase;
    FOR_LOOP(i, self->attack1.numberOfDice) {
        damageBase += rand() % self->attack1.sidesPerDie + 1;
    }
    return damageBase;
}

void M_GetEntityMatrix(entityState_t const *entity, LPMATRIX4 matrix) {
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &entity->origin);
    Matrix4_rotate(matrix, &(VECTOR3){0, 0, entity->angle * 180 / M_PI}, ROTATE_XYZ);
    Matrix4_scale(matrix, &(VECTOR3){entity->scale, entity->scale, entity->scale});
}

static EDICT_FUNC(ai_damagetarget) {
    edict_t *other = ent->goalentity;
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

static EDICT_FUNC(ai_cooldown) {
    M_RunWait(ent, attack_melee);
}

static EDICT_FUNC(ai_melee) {
    M_ChangeAngle(ent);
    M_RunWait(ent, ai_damagetarget);
}

static EDICT_FUNC(ai_walk) {
    float range = UNIT_ATTACK1_RANGE(ent->class_id);
    if (M_DistanceToGoal(ent) < range) {
        attack_melee(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static umove_t attack_move_walk = { "walk", ai_walk };
static umove_t attack_move_cooldown = { "stand ready", ai_cooldown };
static umove_t attack_move_melee = { "attack", ai_melee, attack_cooldown };

void attack_start(edict_t *self, edict_t *target) {
    self->goalentity = target;
    M_SetMove(self, &attack_move_walk);
}

void attack_cooldown(edict_t *self) {
    M_SetMove(self, &attack_move_cooldown);
    self->wait = self->attack1.cooldown;
}

void attack_melee(edict_t *self) {
    M_SetMove(self, &attack_move_melee);
    self->wait = self->attack1.damagePoint;
}

bool attack_menu_selecttarget(edict_t *ent, edict_t *target) {
    if (target->targtype == TARG_GROUND) {
        FOR_SELECTED_UNITS(ent->client, e) {
            attack_start(e, target);
        }
        return true;
    } else {
        return false;
    }
}

void attack_command(edict_t *ent) {
    UI_AddCancelButton(ent);
    ent->client->menu.on_entity_selected = attack_menu_selecttarget;
}

void SP_ability_attack(ability_t *self) {
    self->cmd = attack_command;
}
