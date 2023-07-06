#include "g_local.h"

float HARVEST_LUMBER_CAPACITY;
float HARVEST_GOLD_CAPACITY;
float HARVEST_TREE_DAMAGE;
float HARVEST_RANGE;
float HARVEST_COOLDOWN;
float HARVEST_SEARCH_RANGE;

EDICT_FUNC(harvest_cooldown);
EDICT_FUNC(harvest_swing);
EDICT_FUNC(harvest_walkback);
EDICT_FUNC(harvest_walk);

void harvest_start(edict_t *self, edict_t *target);
void harvest_gold_start(edict_t *self, edict_t *target);
edict_t *find_townhall(edict_t *unit);
    
static edict_t *find_another_tree(edict_t *ent) {
    float min_dist = HARVEST_SEARCH_RANGE;
    edict_t *other = NULL;
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *tree = &globals.edicts[i];
        if (tree->targtype != TARG_TREE || M_IsDead(tree))
            continue;
        float dist = Vector2_distance(&ent->s.origin2, &tree->s.origin2);
        if (dist < min_dist) {
            other = tree;
            min_dist = dist;
        }
    }
    return other;
}

static EDICT_FUNC(look_for_another_tree) {
    edict_t *other = find_another_tree(ent);
    if (other) {
        harvest_start(ent, other);
    } else {
        ent->stand(ent);
    }
}

bool G_ActorHasSkill(edict_t *ent, LPCSTR id) {
    LPCSTR abilities = UNIT_ABILITIES_NORMAL(ent->class_id);
    if (abilities) {
        PARSE_LIST(abilities, abil, getNextSegment) {
            if (!strcmp(abil, id))
                return true;
        }
    }
    return false;
}

static EDICT_FUNC(ai_walktree) {
    if (M_DistanceToGoal(ent) > HARVEST_RANGE) {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    } else if (M_IsDead(ent->goalentity)) {
        look_for_another_tree(ent);
    } else {
        harvest_swing(ent);
    }
}

static EDICT_FUNC(ai_walkback) {
    if (M_DistanceToGoal(ent) < (ent->collision + ent->goalentity->collision + 5)) {
        ent->goalentity = ent->secondarygoal;
        playerState_t *player = G_GetPlayerByNumber(ent->s.player);
        if (player) {
            player->stats[STAT_LUMBER] += ent->harvested_lumber;
        }
        ent->s.renderfx &= ~RF_HAS_LUMBER;
        ent->harvested_lumber = 0;
        harvest_walk(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static EDICT_FUNC(ai_chop) {
    edict_t *tree = ent->secondarygoal;
    if (!M_IsDead(tree)) {
        ent->harvested_lumber += HARVEST_TREE_DAMAGE;
        ent->s.renderfx |= RF_HAS_LUMBER;
    }
    if (tree->health < HARVEST_TREE_DAMAGE) {
        tree->health = 0;
        tree->die(tree, ent);
    } else if (tree->pain) {
        tree->health -= HARVEST_TREE_DAMAGE;
        tree->pain(tree);
    }
}

static EDICT_FUNC(ai_swing) {
    M_RunWait(ent, ai_chop);
}

static EDICT_FUNC(ai_cooldown) {
    M_RunWait(ent, harvest_swing);
}

static umove_t harvest_move_walk = { "walk", ai_walktree };
static umove_t harvest_move_walkback = { "walk", ai_walkback };
static umove_t harvest_move_swing = { "attack", ai_swing, harvest_cooldown };
static umove_t harvest_move_cooldown = { "stand ready", ai_cooldown };

EDICT_FUNC(harvest_cooldown) {
    if (ent->harvested_lumber >= HARVEST_LUMBER_CAPACITY) {
        harvest_walkback(ent);
    } else if (M_IsDead(ent->goalentity)) {
        look_for_another_tree(ent);
    } else {
        M_SetMove(ent, &harvest_move_cooldown);
        ent->wait = HARVEST_COOLDOWN;
    }
}

EDICT_FUNC(harvest_walk) {
    M_SetMove(ent, &harvest_move_walk);
}

EDICT_FUNC(harvest_swing) {
    M_SetMove(ent, &harvest_move_swing);
    ent->wait = UNIT_ATTACK1_DAMAGE_POINT(ent->class_id);
}

EDICT_FUNC(harvest_walkback) {
    edict_t *townhall = find_townhall(ent);
    if (townhall) {
        ent->goalentity = townhall;
        M_SetMove(ent, &harvest_move_walkback);
    } else {
        ent->stand(ent);
    }
}

EDICT_FUNC(CMD_Harvest);

void harvest_start(edict_t *self, edict_t *target) {
    self->goalentity = target;
    self->secondarygoal = target;
    harvest_walk(self);
}

bool harvest_menu_selecttarget(edict_t *clent, edict_t *target) {
    if (G_ActorHasSkill(target, "Agld")) {
        FOR_SELECTED_UNITS(clent->client, ent) {
            harvest_gold_start(ent, target);
        }
    } else if (target->targtype == TARG_TREE) {
        FOR_SELECTED_UNITS(clent->client, ent) {
            harvest_start(ent, target);
        }
    }
    return true;
}

void harvest_command(edict_t *ent) {
    UI_AddCancelButton(ent);
    ent->client->menu.on_entity_selected = harvest_menu_selecttarget;
}

void SP_ability_harvest(ability_t *self) {
    HARVEST_LUMBER_CAPACITY = AB_Number(self, "Data12");
    HARVEST_GOLD_CAPACITY = AB_Number(self, "Data13");
    HARVEST_TREE_DAMAGE = AB_Number(self, "Data11");
    HARVEST_RANGE = AB_Number(self, "Rng1");
    HARVEST_COOLDOWN = AB_Number(self, "Dur1");
    HARVEST_SEARCH_RANGE = AB_Number(self, "Area1");
    
    self->cmd = harvest_command;
}

