#include "g_local.h"

EDICT_FUNC(attack_walk);
EDICT_FUNC(attack_melee);
EDICT_FUNC(attack_cooldown);

static float ai_rolldamage1(edict_t *self, int weapon) {
    DWORD damageBase = UNIT_ATTACK1_DAMAGE_BASE(self->class_id);
    DWORD numberOfDice = UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(self->class_id);
    DWORD sidesPerDie = UNIT_ATTACK1_DAMAGE_SIDES_PER_DIE(self->class_id);
    FOR_LOOP(i, numberOfDice) {
        damageBase += rand() % sidesPerDie + 1;
    }
    return damageBase;
}

static EDICT_FUNC(ai_damagetarget) {
    edict_t *other = ent->goalentity;
    DWORD damage = ai_rolldamage1(ent, 1);
    if (other->health <= damage) {
        other->health = 0;
        other->die(other, ent);
        ent->stand(ent);
        return;
    }
    other->health -= damage;
    /*if (other->unitinfo.melee && !other->enemy) {
        other->enemy = ent;
        other->unitinfo.melee(other);
        M_ChangeAngle(other);
    } else */if (other->pain) {
        other->pain(other);
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
    
    self->unitinfo.wait = UNIT_ATTACK1_BASE_COOLDOWN(self->class_id);
}

void attack_melee(edict_t *self) {
    M_SetMove(self, &attack_move_melee);
    self->unitinfo.wait = UNIT_ATTACK1_DAMAGE_POINT(self->class_id);
}

void attack_menu_selecttarget(edict_t *ent, LPCVECTOR2 mouse) {
    edict_t *target = client_getentityatpoint(ent->client, mouse);
    ability_t const *ability = GetAbilityByIndex(ent->client->lastcode);
    if (!target)
        return;
    LPCSTR targType = UNIT_TARGETED_AS(target->class_id);
    if (targType && !strcmp(targType, "ground")) {
        handle_t heatmap = gi.BuildHeatmap(&target->s.origin2);
        FOR_LOOP(i, globals.num_edicts) {
            edict_t *ent = &globals.edicts[i];
            if (!client_isentityselected(ent->client, ent))
                continue;
            ent->heatmap = heatmap;
            ability->use(ent, target);
        }
    }
}

void attack_command(edict_t *ent) {
    UI_AddCancelButton(ent);
    ent->client->menu.mouseup = attack_menu_selecttarget;
}

void SP_ability_attack(ability_t *self) {
    self->use = attack_start;
    self->cmd = attack_command;
}