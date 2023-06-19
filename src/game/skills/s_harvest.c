#include "g_local.h"

struct {
    float LUMBER_CAPACITY;
    float GOLD_CAPACITY;
    float TREE_DAMAGE;
    float RANGE;
    float COOLDOWN;
    float SEARCH_RANGE;
} harvest;

EDICT_FUNC(harvest_cooldown);
EDICT_FUNC(harvest_swing);
EDICT_FUNC(harvest_walkback);
EDICT_FUNC(harvest_walk);

static EDICT_FUNC(ai_walktree) {
    if (M_DistanceToGoal(ent) < harvest.RANGE) {
        harvest_swing(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static EDICT_FUNC(ai_walkback) {
    if (M_DistanceToGoal(ent) < (ent->s.radius + ent->goalentity->s.radius + 5)) {
        handle_t heatmap = gi.BuildHeatmap(&ent->secondarygoal->s.origin2);
        ent->goalentity = ent->secondarygoal;
        ent->heatmap = heatmap;
        
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

edict_t *find_townhall(edict_t *unit) {
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *ent = &globals.edicts[i];
        if (ent->class_id == MAKEFOURCC('h', 't', 'o', 'w') && ent->s.player == unit->s.player) {
            return ent;
        }
    }
    return NULL;
}

static EDICT_FUNC(ai_chop) {
    edict_t *tree = ent->secondarygoal;
    if (tree->health < harvest.TREE_DAMAGE) {
        tree->health = 0;
        tree->die(tree, ent);
        ent->stand(ent);
        return;
    }
    tree->pain(tree);
    tree->health -= harvest.TREE_DAMAGE;
    ent->harvested_lumber += harvest.TREE_DAMAGE;
    ent->s.renderfx |= RF_HAS_LUMBER;
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

float AB_Number(ability_t const *ability, LPCSTR field) {
    LPCSTR str = gi.FindSheetCell(AbilityData, ability->classname, field);
    return str ? atof(str) : 0;
}

EDICT_FUNC(harvest_cooldown) {
    if (ent->harvested_lumber >= harvest.LUMBER_CAPACITY) {
        harvest_walkback(ent);
    } else {
        M_SetMove(ent, &harvest_move_cooldown);
        ent->unitinfo.wait = harvest.COOLDOWN;
    }
}

EDICT_FUNC(harvest_walk) {
    M_SetMove(ent, &harvest_move_walk);
}

EDICT_FUNC(harvest_swing) {
    M_SetMove(ent, &harvest_move_swing);
    ent->unitinfo.wait = UNIT_ATTACK1_DAMAGE_POINT(ent->class_id);
}

EDICT_FUNC(harvest_walkback) {
    edict_t *townhall = find_townhall(ent);
    if (townhall) {
        handle_t heatmap = gi.BuildHeatmap(&townhall->s.origin2);
        ent->goalentity = townhall;
        ent->heatmap = heatmap;
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

void harvest_menu_selecttarget(edict_t *clent, LPCVECTOR2 mouse) {
    gclient_t *client = clent->client;
    edict_t *target = client_getentityatpoint(client, mouse);
    ability_t const *ability = GetAbilityByIndex(client->lastcode);
    if (!target)
        return;
    LPCSTR targType = DESTRUCTABLE_TARGETED_AS(target->class_id);
    if (targType && !strcmp(targType, "tree")) {
        handle_t heatmap = gi.BuildHeatmap(&target->s.origin2);
        FOR_LOOP(i, globals.num_edicts) {
            edict_t *ent = &globals.edicts[i];
            if (!client_isentityselected(client, ent))
                continue;
            ent->heatmap = heatmap;
            ability->use(ent, target);
        }
    }
}

void harvest_command(edict_t *ent) {
    UI_AddCancelButton(ent);
    ent->client->menu.mouseup = harvest_menu_selecttarget;
}

void SP_ability_harvest(ability_t *self) {
    harvest.LUMBER_CAPACITY = AB_Number(self, "Data12");
    harvest.GOLD_CAPACITY = AB_Number(self, "Data13");
    harvest.TREE_DAMAGE = AB_Number(self, "Data11");
    harvest.RANGE = AB_Number(self, "Rng1");
    harvest.COOLDOWN = AB_Number(self, "Dur1");
    harvest.SEARCH_RANGE = AB_Number(self, "Area1");
    
    self->use = harvest_start;
    self->cmd = harvest_command;
}

