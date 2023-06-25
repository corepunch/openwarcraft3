#include "g_local.h"

float MAX_GOLD;
float MINING_DURATION;
float MINING_CAPACITY;
extern float HARVEST_GOLD_CAPACITY;

EDICT_FUNC(harvestgold_walkback);
EDICT_FUNC(harvestgold_walk);
EDICT_FUNC(harvestgold_wait);
EDICT_FUNC(harvestgold_minegold);

edict_t *find_townhall(edict_t *unit) {
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *ent = &globals.edicts[i];
        if (ent->class_id == MAKEFOURCC('h', 't', 'o', 'w') &&
            ent->s.player == unit->s.player)
        {
            return ent;
        }
    }
    return NULL;
}

static EDICT_FUNC(ai_walkmine) {
    if (M_DistanceToGoal(ent) <  ent->collision + 180) {
        harvestgold_minegold(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static EDICT_FUNC(ai_walkback) {
    if (M_DistanceToGoal(ent) < (ent->collision + ent->goalentity->collision + 5)) {
        ent->goalentity = ent->secondarygoal;
        playerState_t *player = G_GetPlayerByNumber(ent->s.player);
        if (player) {
            player->stats[STAT_GOLD] += ent->harvested_gold;
        }
        ent->s.renderfx &= ~RF_HAS_GOLD;
        ent->harvested_gold = 0;
        harvestgold_walk(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static EDICT_FUNC(ai_minegold) {
    M_RunWait(ent, harvestgold_walkback);
}

static EDICT_FUNC(ai_waittoenter) {
}

static umove_t harvestgold_move_walk = { "walk", ai_walkmine };
static umove_t harvestgold_move_walkback = { "walk", ai_walkback };
static umove_t harvestgold_move_minegold = { "attack", ai_minegold };
static umove_t harvestgold_move_wait = { "stand", ai_waittoenter };

EDICT_FUNC(harvestgold_walk) {
    M_SetMove(ent, &harvestgold_move_walk);
}

EDICT_FUNC(harvestgold_minegold) {
    if (ent->goalentity->peonsinside < MINING_CAPACITY) {
        M_SetMove(ent, &harvestgold_move_minegold);
        ent->unitinfo.wait = MINING_DURATION;
        ent->s.renderfx |= RF_HIDDEN;
        ent->goalentity->peonsinside++;
    } else {
        harvestgold_wait(ent);
    }
}

EDICT_FUNC(harvestgold_walkback) {
    ent->goalentity->peonsinside--;
    ent->s.renderfx |= RF_HAS_GOLD;
    ent->s.renderfx &= ~RF_HIDDEN;
    ent->harvested_gold += HARVEST_GOLD_CAPACITY;
    FILTER_EDICTS(other, other->goalentity == ent->goalentity &&
                  other->unitinfo.currentmove == &harvestgold_move_wait)
    {
        harvestgold_minegold(other);
    }
    edict_t *townhall = find_townhall(ent);
    if (townhall) {
        ent->goalentity = townhall;
        M_SetMove(ent, &harvestgold_move_walkback);
    } else {
        ent->stand(ent);
    }
}

EDICT_FUNC(harvestgold_wait) {
    M_SetMove(ent, &harvestgold_move_wait);
}

void harvest_gold_start(edict_t *self, edict_t *target) {
    self->goalentity = target;
    self->secondarygoal = target;
    harvestgold_walk(self);
}

void SP_ability_goldmine(ability_t *self) {
    MAX_GOLD = AB_Number(self, "Data11");;
    MINING_DURATION = AB_Number(self, "Data12");
    MINING_CAPACITY = AB_Number(self, "Data13");
}
