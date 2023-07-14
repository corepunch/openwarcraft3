#include "g_local.h"

#define MAX_WAYPOINTS 256
static edict_t waypoints[MAX_WAYPOINTS];
DWORD current_waypoint = 0;

LPCSTR attack_type[] = {
    "none",
    "normal",
    "pierce",
    "siege",
    "spells",
    "chaos",
    "magic",
    "hero",
    NULL
};

LPCSTR weapon_type[] = {
    "none",
    "normal",
    "instant",
    "artillery",
    "aline",
    "missile",
    "msplash",
    "mbounce",
    "mline",
    NULL
};

DWORD FindEnumValue(LPCSTR value, LPCSTR values[]) {
    if (!value)
        return 0;
    for (LPCSTR *s = values; *s; s++) {
        if (!strcmp(*s, value)) {
            return (DWORD)(s - values);
        }
    }
    return 0;
}

static FLOAT get_unit_collision(pathTex_t const *pathtex) {
    int size = 0;
    for (int x = 0; x < pathtex->width; x++) {
        if (pathtex->map[(pathtex->width + 1) * x].b)
            size++;
    }
    return size * 16 * 1.3;
}

LPEDICT Waypoint_add(LPCVECTOR2 spot) {
    LPEDICT waypoint = &waypoints[current_waypoint++ % MAX_WAYPOINTS];
    waypoint->s.origin.x = spot->x;
    waypoint->s.origin.y = spot->y;
    M_CheckGround(waypoint);
    return waypoint;
}

BOOL player_pay(playerState_t *ps, DWORD project) {
    if (!ps) return false;
    if (UNIT_GOLD_COST(project) > ps->stats[STAT_GOLD]) return false;
    if (UNIT_LUMBER_COST(project) > ps->stats[STAT_LUMBER]) return false;
    ps->stats[STAT_GOLD] -= UNIT_GOLD_COST(project);
    ps->stats[STAT_LUMBER] -= UNIT_LUMBER_COST(project);
    return true;
}

BOOL M_IsDead(LPEDICT ent) {
    return ent->health.value <= 0;
}

DWORD M_RefreshHeatmap(LPEDICT self) {
    if (!self->heatmap2) {
        self->heatmap2 = gi.BuildHeatmap(self);
    }
    return self->heatmap2;
}

void M_MoveFrame(LPEDICT self) {
    if (self->aiflags & AI_HOLD_FRAME)
        return;
    umove_t const *move = self->currentmove;
    LPCANIMATION anim = self->animation;
    if (!anim)
        return;
    DWORD next_frame = self->s.frame + FRAMETIME;
    if (!strcmp(anim->name, "birth")) {
        DWORD anim_len = anim->interval[1] - anim->interval[0];
        DWORD build_time = UNIT_BUILD_TIME_MSEC(self->class_id);
        next_frame = self->s.frame + FRAMETIME * anim_len / build_time;
    }
    if (self->s.frame < anim->interval[0] ||
        self->s.frame >= anim->interval[1])
    {
        self->s.frame = anim->interval[0] ;
    } else if (next_frame >= anim->interval[1]) {
        SAFE_CALL(move->endfunc, self);
        if (!(self->aiflags & AI_HOLD_FRAME)) {
            self->s.frame = anim->interval[0] ;
        }
    } else {
        self->s.frame = next_frame;
    }
}

void monster_think(LPEDICT self) {
    if (!self->currentmove)
        return;
    M_MoveFrame(self);
    if (self->currentmove->think) {
        self->currentmove->think(self);
    }
}

void monster_start(LPEDICT self) {
    LPCANIMATION anim = self->animation;
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

void SP_SpawnUnit(LPEDICT self) {
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
    self->mana.value = UNIT_MANA(self->class_id);
    self->mana.max_value = UNIT_MANA(self->class_id);
    self->health.value = UNIT_HP(self->class_id);
    self->health.max_value = UNIT_HP(self->class_id);
    self->think = monster_think;
    
    self->attack1.type = FindEnumValue(UNIT_ATTACK1_ATTACK_TYPE(self->class_id), attack_type);
    self->attack1.weapon = FindEnumValue(UNIT_ATTACK1_WEAPON_TYPE(self->class_id), weapon_type);
    self->attack1.damageBase = UNIT_ATTACK1_DAMAGE_BASE(self->class_id);
    self->attack1.numberOfDice = UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(self->class_id);
    self->attack1.sidesPerDie = UNIT_ATTACK1_DAMAGE_SIDES_PER_DIE(self->class_id);
    self->attack1.cooldown = UNIT_ATTACK1_BASE_COOLDOWN(self->class_id);
    self->attack1.damagePoint = UNIT_ATTACK1_DAMAGE_POINT(self->class_id);

    if (self->attack1.weapon == WPN_MISSILE) {
        self->attack1.origin.x = UNIT_ATTACK1_LAUNCH_X(self->class_id);
        self->attack1.origin.y = UNIT_ATTACK1_LAUNCH_Y(self->class_id);
        self->attack1.origin.z = UNIT_ATTACK1_LAUNCH_Z(self->class_id);
        self->attack1.projectile.model = gi.ModelIndex(UNIT_ATTACK1_PROJECTILE_ART(self->class_id));
        self->attack1.projectile.arc = UNIT_ATTACK1_PROJECTILE_ARC(self->class_id);
        self->attack1.projectile.speed = UNIT_ATTACK1_PROJECTILE_SPEED(self->class_id);        
//        printf("%.4s %s\n", &self->class_id, UNIT_ATTACK1_PROJECTILE_ART(self->class_id));
    }

    if ((self->pathtex = M_LoadPathTex(path_tex))) {
        self->collision = get_unit_collision(self->pathtex);
    }
}

void M_CheckGround(LPEDICT self) {
    self->s.origin.z = gi.GetHeightAtPoint(self->s.origin.x, self->s.origin.y);
}

BOOL M_CheckAttack(LPEDICT self) {
    return false;
}

FLOAT M_DistanceToGoal(LPEDICT ent) {
    if (ent->goalentity) {
        return Vector2_distance(&ent->goalentity->s.origin2, &ent->s.origin2);
    } else {
        return 0;
    }
}

BYTE compress_stat(EDICTSTAT const *stat) {
    if (stat->max_value <= 0) {
        return 0;
    } else {
        return 255 * stat->value / stat->max_value;
    }
}
