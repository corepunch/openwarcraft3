#include "s_skills.h"

extern FLOAT HARVEST_GOLD_CAPACITY;

void harvestgold_walkback(LPEDICT ent);
void harvestgold_walk(LPEDICT ent);
void harvestgold_wait(LPEDICT ent);
void harvestgold_minegold(LPEDICT ent);
static umove_t harvestgold_move_wait;

static int goldmine_path_debug_level(void) {
    LPCSTR value;
    value = gi.CvarString("wc3_harvest_path_debug", "0");
    return value ? atoi(value) : 0;
}

static void goldmine_debug_dump_geometry(LPCEDICT mine) {
    pathTex_t const *pathtex;
    UnitUI_t const *ui;
    UnitData_t const *data;
    DWORD blocked = 0;
    int min_x = INT_MAX, min_y = INT_MAX, max_x = -1, max_y = -1;
    int const debug = goldmine_path_debug_level();

    if (debug < 2 || !mine)
        return;

    pathtex = mine->pathtex;
    ui = mine->data.UnitUI;
    data = mine->data.UnitData;
    if (pathtex) {
        FOR_LOOP(y, pathtex->height) {
            FOR_LOOP(x, pathtex->width) {
                if (!pathtex->map[x + y * pathtex->width].b)
                    continue;
                blocked++;
                min_x = MIN(min_x, (int)x);
                min_y = MIN(min_y, (int)y);
                max_x = MAX(max_x, (int)x);
                max_y = MAX(max_y, (int)y);
            }
        }
    }

    {
        FLOAT const cell = CM_PathCellWorldSize();
        FLOAT const local_min_x = blocked ? ((FLOAT)min_x - pathtex->width * 0.5f) * cell : 0.0f;
        FLOAT const local_min_y = blocked ? ((FLOAT)min_y - pathtex->height * 0.5f) * cell : 0.0f;
        FLOAT const local_max_x = blocked ? ((FLOAT)(max_x + 1) - pathtex->width * 0.5f) * cell : 0.0f;
        FLOAT const local_max_y = blocked ? ((FLOAT)(max_y + 1) - pathtex->height * 0.5f) * cell : 0.0f;

        fprintf(stderr,
                "WC3_GOLD_GEOMETRY mine=%d rawcode=%.4s origin=(%.1f,%.1f,%.1f) angle=%.1f "
                "model=\"%s\" scale=%.3f selection_radius=%.1f collision=%.1f "
                "pathing=\"%s\" path=%ux%u blocked=%u blocked_bbox=[%d,%d]-[%d,%d] "
                "blocked_local=[%.1f,%.1f]-[%.1f,%.1f] capacity=%u resources=%u\n",
                mine->s.number, (LPCSTR)&mine->s.class_id,
                mine->s.origin.x, mine->s.origin.y, mine->s.origin.z, mine->s.angle,
                ui && ui->modelFile ? ui->modelFile : "", mine->s.scale, mine->s.radius,
                mine->collision, data && data->pathingTexture ? data->pathingTexture : "",
                pathtex ? pathtex->width : 0, pathtex ? pathtex->height : 0, blocked,
                blocked ? min_x : -1, blocked ? min_y : -1,
                blocked ? max_x : -1, blocked ? max_y : -1,
                local_min_x, local_min_y, local_max_x, local_max_y,
                S_GoldMineCapacity(mine), mine->resources);
    }

    if (debug < 3 || !pathtex)
        return;
    if (pathtex->width > 120) {
        fprintf(stderr,
                "WC3_GOLD_FOOTPRINT mine=%d rawcode=%.4s omitted reason=width width=%u\n",
                mine->s.number, (LPCSTR)&mine->s.class_id, pathtex->width);
        return;
    }
    FOR_LOOP(y, pathtex->height) {
        char row[121];
        FOR_LOOP(x, pathtex->width)
            row[x] = pathtex->map[x + y * pathtex->width].b ? '#' : '.';
        row[pathtex->width] = '\0';
        fprintf(stderr,
                "WC3_GOLD_FOOTPRINT mine=%d row=%02u %s\n",
                mine->s.number, (unsigned)y, row);
    }
}

static void goldmine_debug_log_approach(LPEDICT ent, LPEDICT mine,
                                        FLOAT dist, FLOAT footprint_dist,
                                        FLOAT contact, FLOAT step) {
    static DWORD last_log[MAX_ENTITIES];
    DWORD number;
    DWORD now;

    if (goldmine_path_debug_level() < 2 || !ent || !mine)
        return;
    number = ent->s.number;
    if (number >= MAX_ENTITIES)
        return;
    now = G_Time();
    if (last_log[number] && (DWORD)(now - last_log[number]) < 250)
        return;
    last_log[number] = now;

    fprintf(stderr,
            "WC3_GOLD_PATH approach worker=%d rawcode=%.4s pos=(%.1f,%.1f) collision=%.1f "
            "mine=%d mine_rawcode=%.4s mine_pos=(%.1f,%.1f) mine_angle=%.1f mine_collision=%.1f "
            "distance=%.1f contact=%.1f step=%.1f footprint=%.1f "
            "heading=%.1f direct=%d path_valid=%d waypoint=(%.1f,%.1f) "
            "flow=%u flow_goal=%d unreachable=%d\n",
            ent->s.number, (LPCSTR)&ent->s.class_id, ent->s.origin.x, ent->s.origin.y, ent->collision,
            mine->s.number, (LPCSTR)&mine->s.class_id, mine->s.origin.x, mine->s.origin.y,
            mine->s.angle, mine->collision, dist, contact, step, footprint_dist,
            ent->movement.heading, ent->movement.flow_direct, ent->movement.path.valid,
            ent->movement.path.waypoint.x, ent->movement.path.waypoint.y,
            ent->movement.flow_generation, ent->movement.flow_goal_reached,
            ent->movement.flow_unreachable);
}

/* A resumable route miss leaves direct, accelerator, and flow states clear.
 * Gold movement must hold then: using the previous facing would send workers
 * in an unrelated direction while the shared route is still being built. */
static BOOL gold_route_pending(LPCEDICT worker) {
    return worker && !worker->movement.flow_direct &&
           !worker->movement.path.valid &&
           worker->movement.flow_generation == 0;
}

/* Warsmash-style return movement still needs a concrete static endpoint for a
 * blocked drop-off building.  Pick the innermost collision-safe pathing-cell
 * ring around the authored footprint, then choose the point on that ring
 * closest to this worker.  Unlike centre-rooted flow routing, this preserves
 * the worker's approach side (mine on the left -> left edge of the Town Hall). */
static BOOL gold_find_nearest_footprint_approach(LPEDICT worker, LPEDICT target,
                                                  LPVECTOR2 out) {
    FLOAT const route_band = worker ?
        worker->collision + CM_PathCellWorldSize() * 1.41421356237f : 0.0f;

    if (!worker || !target || !target->pathtex || !out)
        return false;
    return CM_FindInnerApproachPointToFootprintForRadius(
        target, &worker->s.origin2, route_band, worker->collision, out);
}


static AbilityData_t const *goldmine_ability_data(LPCEDICT mine) {
    LPCSTR abilities;

    if (!mine || !mine->data.UnitAbilities || !(abilities = mine->data.UnitAbilities->abilList))
        return NULL;

    PARSE_LIST(abilities, abil, parse_segment) {
        if (G_AbilityCodeName(abil) == MAKEFOURCC('A', 'g', 'l', 'd'))
            return G_AbilityDataName(abil);
    }
    return NULL;
}

BOOL S_GoldMineIsMine(LPCEDICT mine) {
    return goldmine_ability_data(mine) != NULL;
}

DWORD S_GoldMineMaximumGold(LPCEDICT mine) {
    AbilityData_t const *data = goldmine_ability_data(mine);
    if (!data || data->level[0].data[0].number <= 0)
        return 0;
    return (DWORD)data->level[0].data[0].number;
}

FLOAT S_GoldMineMiningDuration(LPCEDICT mine) {
    AbilityData_t const *data = goldmine_ability_data(mine);
    return data ? MAX(0.0f, data->level[0].data[1].number) : 0.0f;
}

DWORD S_GoldMineCapacity(LPCEDICT mine) {
    AbilityData_t const *data = goldmine_ability_data(mine);
    if (!data || data->level[0].data[2].number <= 0)
        return 0;
    return (DWORD)data->level[0].data[2].number;
}

BOOL S_GoldMineCanHarvest(LPCEDICT mine) {
    return mine && mine->inuse && mine->health.value > 0 && S_GoldMineIsMine(mine) && mine->resources > 0;
}

BOOL S_GoldMineWorkerIsInside(LPCEDICT worker) {
    return worker && worker->goldmine.mine != NULL;
}

void S_GoldMineInitUnit(LPEDICT mine) {
    DWORD maximum;

    if (!S_GoldMineIsMine(mine))
        return;
    maximum = S_GoldMineMaximumGold(mine);
    if (mine->resources == 0 && maximum > 0)
        mine->resources = maximum;
    goldmine_debug_dump_geometry(mine);
}

static BOOL goldmine_membership_valid(LPCEDICT worker, LPCEDICT mine) {
    return worker && mine && worker->goldmine.mine == mine && mine->inuse &&
        worker->goldmine.mine_spawn_time == mine->spawn_time;
}

static void goldmine_register_miner(LPEDICT worker, LPEDICT mine) {
    worker->goldmine.mine = mine;
    worker->goldmine.mine_spawn_time = mine->spawn_time;
    worker->goldmine.restore_invulnerable = worker->invulnerable;
    worker->invulnerable = true;
    worker->s.renderfx |= RF_HIDDEN;
    mine->peonsinside++;
}

static LPEDICT goldmine_unregister_miner(LPEDICT worker) {
    LPEDICT mine;

    if (!worker || !(mine = worker->goldmine.mine))
        return NULL;
    if (goldmine_membership_valid(worker, mine) && mine->peonsinside > 0)
        mine->peonsinside--;
    worker->goldmine.mine = NULL;
    worker->goldmine.mine_spawn_time = 0;
    worker->invulnerable = worker->goldmine.restore_invulnerable;
    worker->goldmine.restore_invulnerable = false;
    worker->s.renderfx &= ~RF_HIDDEN;
    return mine;
}

static void goldmine_deplete(LPEDICT mine) {
    if (!mine || !mine->inuse || mine->resources > 0 || M_IsDead(mine))
        return;
    G_SetHealth(mine, 0);
    if (mine->die)
        mine->die(mine, NULL);
    else
        mine->svflags |= SVF_DEADMONSTER;
}

static void goldmine_wake_waiters(LPEDICT mine) {
    /* Called immediately after goldmine_deplete; checks mine->inuse so a
     * synchronous G_FreeEdict in die() does not iterate freed memory. */
    if (!mine || !mine->inuse)
        return;
    FILTER_EDICTS(other, other->goalentity == mine &&
                  other->currentmove == &harvestgold_move_wait)
    {
        harvestgold_minegold(other);
    }
}

void S_GoldMineReleaseWorker(LPEDICT worker) {
    LPEDICT mine;

    if (!S_GoldMineWorkerIsInside(worker))
        return;
    mine = goldmine_unregister_miner(worker);
    goldmine_wake_waiters(mine);
}


static void ai_walkmine(LPEDICT ent) {
    LPEDICT mine = ent ? ent->goalentity : NULL;
    FLOAT dist, contact, step, footprint_dist;
    BOOL footprint_entry, circle_entry;
    int const debug = goldmine_path_debug_level();

    if (!S_GoldMineCanHarvest(mine)) {
        if (debug >= 1 && ent) {
            fprintf(stderr,
                    "WC3_GOLD_PATH stop worker=%d mine=%d reason=not_harvestable resources=%u\n",
                    ent->s.number, mine ? mine->s.number : -1,
                    mine ? mine->resources : 0);
        }
        ent->stand(ent);
        return;
    }

    dist = M_DistanceToGoal(ent);
    contact = ent->collision + mine->collision;
    step = unit_movedistance(ent);
    footprint_dist = CM_DistanceToPathingFootprint(mine, &ent->s.origin2);
    footprint_entry = footprint_dist < FLT_MAX && footprint_dist <= ent->collision + step;
    circle_entry = dist <= contact + step;

    /* Mine entry is an interaction with the authored building footprint, not
     * necessarily its centre collision circle.  A worker approaching a square
     * mine at a corner can be stopped by pathing while its centre-to-centre
     * distance is still larger than collision+step.  Complete the interaction
     * when the next step would touch the mine's own no-walk footprint.  Keep
     * the historical collision-circle rule as a fallback for mines with no
     * path texture. */
    if (footprint_entry || circle_entry) {
        if (debug >= 1) {
            fprintf(stderr,
                    "WC3_GOLD_PATH enter_range worker=%d mine=%d distance=%.1f contact=%.1f step=%.1f footprint=%.1f via=%s peons=%u capacity=%u resources=%u\n",
                    ent->s.number, mine->s.number, dist, contact, step,
                    footprint_dist, footprint_entry ? "footprint" : "circle",
                    mine->peonsinside, S_GoldMineCapacity(mine), mine->resources);
        }
        harvestgold_minegold(ent);
    } else {
        /* Warsmash's CBehaviorHarvest disables live-unit collision when its
         * target is a unit. Gold Mines are units, so retain authored/static
         * pathing while allowing miners to share the same approach space. */
        unit_changeangle_interaction_ignore_units(ent);
        goldmine_debug_log_approach(ent, mine, dist, footprint_dist, contact, step);
        if (gold_route_pending(ent))
            return;
        /* The collision-sized route ends outside the mine footprint. The
         * behavior-owned range check above remains authoritative for entry. */
        unit_moveindirection_ignore_units(ent);
    }
}

static void goldmine_finish_deposit(LPEDICT ent, LPEDICT dropoff, int debug) {
    LPPLAYER player;

    G_PublishMessage(ent, GAME_MSG_HARVEST_DEPOSIT_GOLD, dropoff);
    ent->goalentity = ent->secondarygoal;
    player = G_GetPlayerByNumber(ent->s.player);
    if (player) {
        G_CreditResourceIncome(player, ent, PLAYERSTATE_RESOURCE_GOLD,
                               (LONG)ent->harvested_gold);
    }
    if (debug >= 1)
        fprintf(stderr,
                "WC3_GOLD_RETURN deposit worker=%d dropoff=%d resume_mine=%d gold=%u\n",
                ent->s.number, dropoff->s.number,
                ent->goalentity ? ent->goalentity->s.number : -1,
                ent->harvested_gold);
    S_SetCarriedResource(ent, RETURN_RESOURCE_GOLD, 0);
    if (S_GoldMineCanHarvest(ent->goalentity)) {
        G_PublishMessage(ent, GAME_MSG_HARVEST_RESUME_GOLD, ent->goalentity);
        harvestgold_walk(ent);
    } else {
        ent->stand(ent);
    }
}

static void ai_goldmine_walkback(LPEDICT ent) {
    LPEDICT dropoff;
    FLOAT dist, contact, step, footprint_dist;
    BOOL footprint_deposit, circle_deposit;
    int const debug = goldmine_path_debug_level();

    if (!S_CanReturnResourceAt(ent, ent->goalentity, RETURN_RESOURCE_GOLD)) {
        dropoff = S_FindNearestResourceDropoff(ent, RETURN_RESOURCE_GOLD);
        if (!dropoff) {
            if (debug >= 1)
                fprintf(stderr,
                        "WC3_GOLD_RETURN stop worker=%d reason=no_dropoff gold=%u\n",
                        ent->s.number, ent->harvested_gold);
            ent->stand(ent);
            return;
        }
        G_PublishMessage(ent, GAME_MSG_HARVEST_RETURN_GOLD, dropoff);
        ent->goalentity = dropoff;
        move_reset_progress(ent);
        if (debug >= 1)
            fprintf(stderr,
                    "WC3_GOLD_RETURN retarget worker=%d dropoff=%d gold=%u\n",
                    ent->s.number, dropoff->s.number, ent->harvested_gold);
    }

    dropoff = ent->goalentity;
    dist = M_DistanceToGoal(ent);
    contact = ent->collision + dropoff->collision;
    step = unit_movedistance(ent);
    footprint_dist = CM_DistanceToPathingFootprint(dropoff, &ent->s.origin2);
    footprint_deposit = footprint_dist < FLT_MAX &&
                        footprint_dist <= ent->collision + step;
    circle_deposit = dist <= contact + step;

    /* Return-resource range is footprint-aware for the same reason mine entry
     * is: a Town Hall's authored no-walk cells can stop the worker before a
     * centre-circle approximation reaches contact.  Deposit when the worker's
     * radius plus one simulation step reaches the actual building footprint.
     * Keep collision+step as the fallback for buildings with no path texture. */
    if (footprint_deposit || circle_deposit) {
        if (debug >= 1)
            fprintf(stderr,
                    "WC3_GOLD_RETURN deposit_range worker=%d dropoff=%d distance=%.1f contact=%.1f step=%.1f footprint=%.1f via=%s gold=%u\n",
                    ent->s.number, dropoff->s.number, dist, contact, step,
                    footprint_dist, footprint_deposit ? "footprint" : "circle",
                    ent->harvested_gold);
        goldmine_finish_deposit(ent, dropoff, debug);
    } else {
        VECTOR2 approach;

        /* Return Resources targets the building interaction boundary, not an
         * arbitrary legal cell around its blocked centre. Pick the innermost
         * collision-safe ring and then the worker's current side. */
        if (gold_find_nearest_footprint_approach(ent, dropoff, &approach)) {
            if (unit_snap_to_point_ignore_units(ent, &approach)) {
                goldmine_finish_deposit(ent, dropoff, debug);
                return;
            }
            if (unit_changeangle_towards_point_ignore_units(ent, &approach)) {
                unit_moveindirection_ignore_units(ent);
                return;
            }
        }

        /* Longer detours retain the shared collision-sized fallback while
         * live units remain non-blocking on resource-return legs. */
        unit_changeangle_interaction_ignore_units(ent);
        if (gold_route_pending(ent))
            return;
        unit_moveindirection_ignore_units(ent);
    }
}

static void ai_minegold(LPEDICT ent) {
    unit_runwait(ent, harvestgold_walkback);
}

static void ai_waittoenter(LPEDICT ent) {
}

static umove_t harvestgold_move_walk = { "walk", ai_walkmine, NULL, CAbilityGoldMine };
static umove_t harvestgold_move_walkback = { "walk", ai_goldmine_walkback, NULL, CAbilityGoldMine };
static umove_t harvestgold_move_minegold = { "attack", ai_minegold, NULL, CAbilityGoldMine };
static umove_t harvestgold_move_wait = { "stand", ai_waittoenter, NULL, CAbilityGoldMine };

BOOL harvest_gold_return_to(LPEDICT ent, LPEDICT dropoff) {
    if (!ent || !dropoff || !ent->harvested_gold ||
        !S_CanReturnResourceAt(ent, dropoff, RETURN_RESOURCE_GOLD)) {
        return false;
    }

    G_PublishMessage(ent, GAME_MSG_HARVEST_RETURN_GOLD, dropoff);
    ent->goalentity = dropoff;
    move_reset_progress(ent);
    unit_setmove(ent, &harvestgold_move_walkback);
    return true;
}

void harvestgold_walk(LPEDICT ent) {
    move_reset_progress(ent);
    unit_setmove(ent, &harvestgold_move_walk);
}

void harvestgold_minegold(LPEDICT ent) {
    LPEDICT mine = ent ? ent->goalentity : NULL;
    LPEDICT dropoff;
    DWORD capacity;

    if (!ent || !S_GoldMineCanHarvest(mine)) {
        if (ent) ent->stand(ent);
        return;
    }
    if (S_GoldMineWorkerIsInside(ent))
        return;

    /* Retail honors the clicked mine first even when the worker is already
     * carrying gold.  Once the worker reaches the mine interaction boundary,
     * do not mine another load: return the existing gold to the nearest valid
     * drop-off, then resume this clicked mine after the deposit completes. */
    if (ent->harvested_gold > 0) {
        dropoff = S_FindNearestResourceDropoff(ent, RETURN_RESOURCE_GOLD);
        if (!dropoff || !harvest_gold_return_to(ent, dropoff)) {
            ent->stand(ent);
        }
        return;
    }

    capacity = S_GoldMineCapacity(mine);
    if (capacity == 0) {
        if (goldmine_path_debug_level() >= 1)
            fprintf(stderr, "WC3_GOLD_PATH stop worker=%d mine=%d reason=zero_capacity\n",
                    ent->s.number, mine->s.number);
        ent->stand(ent);
        return;
    }
    if (mine->peonsinside < capacity) {
        if (goldmine_path_debug_level() >= 1)
            fprintf(stderr,
                    "WC3_GOLD_PATH enter worker=%d mine=%d peons=%u capacity=%u duration=%.3f resources=%u\n",
                    ent->s.number, mine->s.number, mine->peonsinside, capacity,
                    S_GoldMineMiningDuration(mine), mine->resources);
        G_PublishMessage(ent, GAME_MSG_HARVEST_ENTER_MINE, mine);
        unit_setmove(ent, &harvestgold_move_minegold);
        ent->wait = S_GoldMineMiningDuration(mine);
        goldmine_register_miner(ent, mine);
    } else {
        if (goldmine_path_debug_level() >= 1)
            fprintf(stderr,
                    "WC3_GOLD_PATH wait worker=%d mine=%d peons=%u capacity=%u resources=%u\n",
                    ent->s.number, mine->s.number, mine->peonsinside, capacity, mine->resources);
        harvestgold_wait(ent);
    }
}

void harvestgold_walkback(LPEDICT ent) {
    LPEDICT mine;
    DWORD amount = 0;
    DWORD carry_capacity;

    if (!ent || !(mine = ent->goldmine.mine)) {
        if (ent) ent->stand(ent);
        return;
    }

    if (goldmine_membership_valid(ent, mine) && !M_IsDead(mine) && S_GoldMineIsMine(mine)) {
        carry_capacity = HARVEST_GOLD_CAPACITY > 0 ? (DWORD)HARVEST_GOLD_CAPACITY : 0;
        amount = MIN(mine->resources, carry_capacity);
        mine->resources -= amount;
    }

    goldmine_unregister_miner(ent);
    if (mine->inuse && mine->resources == 0)
        goldmine_deplete(mine);
    goldmine_wake_waiters(mine);

    if (amount == 0) {
        ent->stand(ent);
        return;
    }

    S_SetCarriedResource(ent, RETURN_RESOURCE_GOLD, ent->harvested_gold + amount);
    LPEDICT dropoff = S_FindNearestResourceDropoff(ent, RETURN_RESOURCE_GOLD);
    if (dropoff) {
        if (goldmine_path_debug_level() >= 1)
            fprintf(stderr,
                    "WC3_GOLD_RETURN start worker=%d mine=%d dropoff=%d gold=%u\n",
                    ent->s.number, mine->s.number, dropoff->s.number,
                    ent->harvested_gold);
        G_PublishMessage(ent, GAME_MSG_HARVEST_RETURN_GOLD, dropoff);
        ent->goalentity = dropoff;
        move_reset_progress(ent);
        unit_setmove(ent, &harvestgold_move_walkback);
    } else {
        ent->stand(ent);
    }
}

void harvestgold_wait(LPEDICT ent) {
    unit_setmove(ent, &harvestgold_move_wait);
}

void harvest_gold_start(LPEDICT self, LPEDICT target) {
    if (goldmine_path_debug_level() >= 1) {
        fprintf(stderr,
                "WC3_GOLD_PATH start worker=%d rawcode=%.4s pos=(%.1f,%.1f) "
                "mine=%d mine_rawcode=%.4s mine_pos=(%.1f,%.1f) mine_angle=%.1f\n",
                self->s.number, (LPCSTR)&self->s.class_id, self->s.origin.x, self->s.origin.y,
                target ? target->s.number : -1, target ? (LPCSTR)&target->s.class_id : "----",
                target ? target->s.origin.x : 0.0f, target ? target->s.origin.y : 0.0f,
                target ? target->s.angle : 0.0f);
    }
    self->goalentity = target;
    self->secondarygoal = target;
    G_PublishMessage(self, GAME_MSG_HARVEST_MOVE_GOLD, target);
    harvestgold_walk(self);
}

BOOL harvest_gold_order(LPEDICT self, LPEDICT target) {
    if (!self || !target || !S_GoldMineCanHarvest(target))
        return false;

    /* Keep the clicked mine as the immediate goal regardless of carried
     * resources.  Gold already in hand is handled only after the worker
     * reaches this mine, matching retail's visible order transition. */
    harvest_gold_start(self, target);
    return true;
}

intptr_t CAbilityGoldMine(LPEDICT ent, abilityMsg_t msg, abilityCall_t const *call) {
    return CAbilityNoop(ent, msg, call);
}

/* ---- Entangle Gold Mine (Aent): NE transforms ownership of a mine ------- */
static BOOL entangle_goldmine_selecttarget(LPEDICT clent, LPEDICT target) {
    if (!target || !S_GoldMineIsMine(target)) {
        return false;
    }
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    if (!caster) {
        return false;
    }
    /* Transfer mine ownership to the caster's player. */
    G_SetUnitPlayer(target, caster->s.player);
    return true;
}

static void entangle_goldmine_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
    clent->client->menu.on_entity_selected = entangle_goldmine_selecttarget;
}

BZ_COMMAND_PROC(AbilityEntangle, entangle_goldmine_command)

/* ---- Blighted Gold Mine (Abgm): interval-based income for Undead -------- */
static FLOAT blight_gold_per_interval;
static FLOAT blight_interval_duration;

void blight_mine_think(LPEDICT ent) {
    monster_think(ent);
    DWORD now = G_Time();
    if (ent->freetime && now < ent->freetime)
        return;
    LPPLAYER player = G_GetPlayerByNumber(ent->s.player);

    if (!player || ent->health.value <= 0) {
        return;
    }
    /* Apply the same player income rate used by worker deposits. */
    G_CreditResourceIncome(player, ent, PLAYERSTATE_RESOURCE_GOLD,
                           (LONG)blight_gold_per_interval);
    ent->freetime = now + (DWORD)(blight_interval_duration * 1000.0f);
}

intptr_t CAbilityBlightedGoldMine(LPEDICT ent, abilityMsg_t msg, abilityCall_t const *call) {
    (void)ent;
    if (msg != A_INIT || !call || !call->classname) return false;
    blight_gold_per_interval = G_AbilityDataName(call->classname)->level[0].data[0].number;
    blight_interval_duration = G_AbilityDataName(call->classname)->level[0].data[1].number;
    return true;
}
