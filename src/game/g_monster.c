#include "g_local.h"

#define MAX_WAYPOINTS 256
static edict_t waypoints[MAX_WAYPOINTS];
DWORD current_waypoint = 0;

static float get_unit_collision(pathTex_t const *pathtex) {
    int size = 0;
    for (int x = 0; x < pathtex->width; x++) {
        if (pathtex->map[(pathtex->width + 1) * x].b)
            size++;
    }
    return size * 16 * 1.3;
}

edict_t *Waypoint_add(LPCVECTOR2 spot) {
    edict_t *waypoint = &waypoints[current_waypoint++ % MAX_WAYPOINTS];
    waypoint->s.origin.x = spot->x;
    waypoint->s.origin.y = spot->y;
    M_CheckGround(waypoint);
    return waypoint;
}

#define MAX_SPAWN_ITERATIONS 10

bool player_pay(playerState_t *ps, DWORD project) {
    if (!ps) return false;
    if (UNIT_GOLD_COST(project) > ps->stats[STAT_GOLD]) return false;
    if (UNIT_LUMBER_COST(project) > ps->stats[STAT_LUMBER]) return false;
    ps->stats[STAT_GOLD] -= UNIT_GOLD_COST(project);
    ps->stats[STAT_LUMBER] -= UNIT_LUMBER_COST(project);
    return true;
}

void SP_TrainUnit(playerState_t *ps, edict_t *townhall, DWORD class_id) {
    float const colsize = UNIT_SELECTION_SCALE(class_id) * SEL_SCALE / 2;
    float const start_angle = M_PI * 1.25f;
    FOR_LOOP(i, MAX_SPAWN_ITERATIONS) {
        float const radius = townhall->s.radius + colsize * (i * 2 + 1);
        float const num_points = M_PI * radius / colsize;
        FOR_LOOP(j, num_points) {
            float const angle = start_angle + 2 * M_PI * j / num_points;
            VECTOR2 const org = {
                townhall->s.origin2.x + cosf(angle) * radius,
                townhall->s.origin2.y + sinf(angle) * radius,
            };
            if (M_CheckCollision(&org, colsize))
                continue;
            if (player_pay(ps, class_id)) {
                edict_t *ent = SP_SpawnAtLocation(class_id, townhall->s.player, &org);
                ent->s.angle = angle;
            } else {
                printf("Not enough resources\n");
            }
            return;
        }
    }
}

bool M_IsDead(edict_t *ent) {
    return ent->health <= 0;
}

handle_t M_RefreshHeatmap(edict_t *self) {
    if (!self->heatmap2) {
        self->heatmap2 = gi.BuildHeatmap(self);
    }
    return self->heatmap2;
}

void M_MoveFrame(edict_t *self) {
    if (self->unitinfo.aiflags & AI_HOLD_FRAME)
        return;
    umove_t const *move = self->unitinfo.currentmove;
    animation_t const *anim = self->animation;
    if (!anim)
        return;
    DWORD next_frame = self->s.frame + FRAMETIME;
    if (!strcmp(anim->name, "birth")) {
        DWORD anim_len = anim->interval[1] - anim->interval[0];
        DWORD build_time = UNIT_BUILD_TIME(self->class_id) * 1000;
        next_frame = self->s.frame + FRAMETIME * anim_len / build_time;
    }
    if (self->s.frame < anim->interval[0] ||
        self->s.frame >= anim->interval[1])
    {
        self->s.frame = anim->interval[0] ;
    } else if (next_frame >= anim->interval[1]) {
        SAFE_CALL(move->endfunc, self);
        if (!(self->unitinfo.aiflags & AI_HOLD_FRAME)) {
            self->s.frame = anim->interval[0] ;
        }
    } else {
        self->s.frame = next_frame;
    }
}

void monster_think(edict_t *self) {
    if (!self->unitinfo.currentmove)
        return;
    M_MoveFrame(self);
    if (self->unitinfo.currentmove->think) {
        self->unitinfo.currentmove->think(self);
    }
}

void monster_start(edict_t *self) {
    animation_t const *anim = self->animation;
    if (anim) {
        DWORD len = MAX(1, anim->interval[1] - anim->interval[0] - 1);
        self->s.frame = (anim->interval[0] + (rand() % len));
    }
}

//unitRace_t M_GetRace(LPCSTR string) {
//    if (!strcmp(string, STR_HUMAN)) return RACE_HUMAN;
//    if (!strcmp(string, STR_ORC)) return RACE_ORC;
//    if (!strcmp(string, STR_UNDEAD)) return RACE_UNDEAD;
//    if (!strcmp(string, STR_NIGHTELF)) return RACE_NIGHTELF;
//    if (!strcmp(string, STR_DEMON)) return RACE_DEMON;
//    if (!strcmp(string, STR_CREEPS)) return RACE_CREEPS;
//    if (!strcmp(string, STR_CRITTERS)) return RACE_CRITTERS;
//    if (!strcmp(string, STR_OTHER)) return RACE_OTHER;
//    if (!strcmp(string, STR_COMMONER)) return RACE_COMMONER;
//    return RACE_UNKNOWN;
//}


struct jpeg_imageinfo {
    int width;
    int height;
    int channels;
    DWORD size;
    int num_components;
    BYTE *data;
};

pathTex_t *M_LoadPathTex(LPCSTR filename) {
    pathTex_t *pathTex = NULL;
    if (filename && strlen(filename) > 1) {
        DWORD filesize;
        HANDLE buffer = gi.ReadFile(filename, &filesize);
        if (buffer) {
            pathTex = LoadTGA(buffer, filesize);
        }
        gi.MemFree(buffer);
        return pathTex;
    }
    return NULL;
}

DWORD M_LoadUberSplat(LPCSTR uber_splat) {
    if (IS_FOURCC(uber_splat)) {
        LPCSTR dir = gi.FindSheetCell(game.config.uberSplats, uber_splat, "dir");
        LPCSTR file = gi.FindSheetCell(game.config.uberSplats, uber_splat, "file");
        LPCSTR scale = gi.FindSheetCell(game.config.uberSplats, uber_splat, "scale");
        PATHSTR filename;
        snprintf(filename, sizeof(PATHSTR), "%s\\%s.blp", dir, file);
        return gi.ImageIndex(filename) | (atoi(scale) << 16);
    } else {
        return 0;
    }
}

void SP_SpawnUnit(edict_t *self) {
    PATHSTR model_filename;
    LPCSTR uber_splat = UNIT_UBER_SPLAT(self->class_id);
    LPCSTR path_tex = UNIT_PATH_TEX(self->class_id);
    sprintf(model_filename, "%s.mdx", UNIT_MODEL(self->class_id));
    self->s.model = gi.ModelIndex(model_filename);
    self->s.splat = M_LoadUberSplat(uber_splat);
    self->s.scale = UNIT_SCALING_VALUE(self->class_id);
    self->s.radius = UNIT_SELECTION_SCALE(self->class_id) * SEL_SCALE / 2;
    self->s.flags |= UNIT_SPEED(self->class_id) > 0 ? EF_MOVABLE : 0;
    self->collision = self->s.radius;//UNIT_COLLISION(self->class_id);
    self->targtype = G_GetTargetType(UNIT_TARGETED_AS(self->class_id));
    self->max_health = UNIT_HP(self->class_id);
    self->health = UNIT_HP(self->class_id);
    self->think = monster_think;
    if ((self->pathtex = M_LoadPathTex(path_tex))) {
        self->collision = get_unit_collision(self->pathtex);
    }
}

void M_CheckGround(edict_t *self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

bool M_CheckAttack(edict_t *self) {
    return false;
}

float M_DistanceToGoal(edict_t *ent) {
    if (ent->goalentity) {
        return Vector2_distance(&ent->goalentity->s.origin2, &ent->s.origin2);
    } else {
        return 0;
    }
}
