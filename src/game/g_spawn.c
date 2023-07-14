#include "g_local.h"

#define MAX_SPAWN_ITERATIONS 10

LPCSTR targs[] = {
    "none", // NONE
    "air",  // AIR
    "aliv", // ALIVE
    "alli", // ALLIES
    "dead", // DEAD
    "debr", // DEBRIS
    "enem", // ENEMIES
    "grou", // GROUND
    "hero", // HERO
    "invu", // INVULNERABLE
    "item", // ITEM
    "mech", // MECHANICAL
    "neut", // NEUTRAL
    "nonh", // NONHERO
    "nons", // NONSAPPER
    "nots", // NOTSELF
    "orga", // ORGANIC
    "play", // PLAYERUNITS
    "sapp", // SAPPER
    "self", // SELF
    "stru", // STRUCTURE
    "terr", // TERRAIN
    "tree", // TREE
    "vuln", // VULNERABLE
    "wall", // WALL
    "ward", // WARD
    "anci", // ANCIENT
    "nona", // NONANCIENT
    "frie", // FRIEND
    "brid", // BRIDGE
    "deco", // DECORATION
};

TARGTYPE G_GetTargetType(LPCSTR str) {
    DWORD const len = (DWORD)strlen(str);
    if (len < 3) return TARG_NONE;
    char buf[64] = { 0 };
    FOR_LOOP(c, len) buf[c] = tolower(str[c]);
    FOR_LOOP(i, sizeof(targs)/sizeof(*targs)) {
        if (*(DWORD *)buf == *(DWORD *)targs[i])
            return i;
    }
    return TARG_NONE;
}

//struct spawn {
//    LPCSTR name;
//    void (*func)(LPEDICT edict);
//};

//static struct spawn spawns[] = {
//    { "opeo", SP_monster_unit },
//    { NULL, NULL }
//};

extern sheetRow_t *Doodads;

void SP_monster_unit(LPEDICT edict);
void SP_monster_tree(LPEDICT edict);

static void G_InitEdict(LPEDICT e) {
    memset(e, 0, sizeof(edict_t));
    e->inuse = true;
    e->s.scale = 1;
    e->s.number = (int)(e - game_state.edicts);
}

void G_FreeEdict(LPEDICT ent) {
    memset(ent, 0, sizeof(edict_t));
}

LPEDICT G_Spawn(void) {
    for (DWORD i = game.max_clients + 1; i < globals.num_edicts; i++) {
        LPEDICT e = &game_state.edicts[i];
        if (!e->inuse) {
            G_InitEdict(e);
            return e;
        }
    }
    LPEDICT edict = &game_state.edicts[globals.num_edicts++];
    G_InitEdict(edict);
    return edict;
}

static void SP_SpawnDoodad(LPEDICT edict) {
    LPCSTR class_id = GetClassName(edict->class_id);
    LPCSTR dir = gi.FindSheetCell(Doodads, class_id, "dir");
    LPCSTR file = gi.FindSheetCell(Doodads, class_id, "file");
    PATHSTR buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", dir, file, file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
}

static void SP_SpawnDestructable(LPEDICT edict) {
    LPCSTR dir = DESTRUCTABLE_DIRECTORY(edict->class_id);
    LPCSTR file = DESTRUCTABLE_FILE(edict->class_id);
    PATHSTR buffer;
    sprintf(buffer, "%s.blp", DESTRUCTABLE_TEXTURE(edict->class_id));
    edict->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", dir, file, file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
    edict->s.radius = 50;//destr->radius;
    edict->collision = 50;
    edict->health.value = DESTRUCTABLE_HIT_POINT_MAXIMUM(edict->class_id);
    edict->health.max_value = DESTRUCTABLE_HIT_POINT_MAXIMUM(edict->class_id);
    edict->targtype = G_GetTargetType(DESTRUCTABLE_TARGETED_AS(edict->class_id));
}

sheetRow_t *find_row(LPCSTR dood_id) {
    FOR_EACH_LIST(sheetRow_t, d, Doodads){
        if (!strcmp(d->name, dood_id))
            return d;
    }
    return NULL;
}

void SP_CallSpawn(LPEDICT edict) {
    if (!edict->class_id)
        return;
    if (find_row(GetClassName(edict->class_id))) {
        SP_SpawnDoodad(edict);
    } else if (DESTRUCTABLE_FILE(edict->class_id)) {
        SP_SpawnDestructable(edict);
        SP_monster_tree(edict);
    } else if (UNIT_MODEL(edict->class_id)) {
        SP_SpawnUnit(edict);
        SP_monster_unit(edict);
    } else {
        edict->svflags |= SVF_NOCLIENT;
//        fprintf(stderr, "Unknown id %.4s\n", (const char *)&edict->class_id);
    }
//    for (struct spawn *s = spawns; s->func; s++) {
//        if (*((int const *)s->name) == edict->class_id) {
//            s->func(edict);
//            return;
//        }
//    }
}

void SP_worldspawn(LPEDICT ent) {
    SetAbilityNames();
}

void G_SpawnEntities(LPCSTR mapname, LPCDOODAD entities) {
    FOR_LOOP(i, game.max_clients) {
        game_state.edicts[i + 1].client = game.clients + i;
    }
    globals.num_edicts = game.max_clients + 1;
    FOR_EACH_LIST(DOODAD const, doodad, entities) {
        LPEDICT e = G_Spawn();
        e->class_id = doodad->doodID;
        e->variation = doodad->variation;
        e->hero = doodad->hero;
        e->s.player = doodad->player & 7;
        e->s.origin = doodad->position;
        e->s.angle = doodad->angle;
        e->s.scale = doodad->scale.x;
//        if (doodad->inventoryItems) {
//            int a= 0;
//        }
        SP_CallSpawn(e);
    }
    SP_worldspawn(NULL);
}
 
LPEDICT SP_SpawnAtLocation(DWORD class_id, DWORD player, LPCVECTOR2 location) {
    LPEDICT ent = G_Spawn();
    ent->class_id = class_id;
    ent->spawn_time = gi.GetTime();
    ent->s.origin.x = location->x;
    ent->s.origin.y = location->y;
    ent->s.origin.z = gi.GetHeightAtPoint(location->x, location->y);
    ent->s.scale = 1;
    ent->s.angle = -M_PI / 2;
    ent->s.player = player;
    SP_CallSpawn(ent);
    ent->birth(ent);
    return ent;
}

BOOL SP_FindEmptySpaceAround(LPEDICT townhall, DWORD class_id, LPVECTOR2 out, FLOAT *angle) {
    FLOAT const colsize = UNIT_SELECTION_SCALE(class_id) * SEL_SCALE / 2;
    FLOAT const start_angle = M_PI * 1.25f;
    FOR_LOOP(i, MAX_SPAWN_ITERATIONS) {
        FLOAT const radius = townhall->s.radius + colsize * (i * 2 + 1);
        FLOAT const num_points = M_PI * radius / colsize;
        FOR_LOOP(j, num_points) {
            *angle = start_angle + 2 * M_PI * j / num_points;
            *out = MAKE(VECTOR2,
                townhall->s.origin2.x + cosf(*angle) * radius,
                townhall->s.origin2.y + sinf(*angle) * radius,
            );
            if (M_CheckCollision(out, colsize))
                continue;
            return true;
        }
    }
    return false;
}
