#include "g_local.h"

//struct spawn {
//    LPCSTR name;
//    void (*func)(edict_t *edict);
//};

//static struct spawn spawns[] = {
//    { "opeo", SP_monster_unit },
//    { NULL, NULL }
//};

extern sheetRow_t *Doodads;

void SP_monster_unit(edict_t *edict);
void SP_monster_tree(edict_t *edict);

static void G_InitEdict(edict_t *e) {
    memset(e, 0, sizeof(edict_t));
    e->inuse = true;
    e->s.scale = 1;
    e->s.number = (int)(e - game_state.edicts);
}

edict_t *G_Spawn(void) {
    for (DWORD i = game.max_clients + 1; i < globals.num_edicts; i++) {
        edict_t *e = &game_state.edicts[i];
        if (!e->inuse) {
            G_InitEdict(e);
            return e;
        }
    }
    edict_t *edict = &game_state.edicts[globals.num_edicts++];
    G_InitEdict(edict);
    return edict;
}

static void SP_SpawnDoodad(edict_t *edict) {
    LPCSTR class_id = GetClassName(edict->class_id);
    LPCSTR dir = gi.FindSheetCell(Doodads, class_id, "dir");
    LPCSTR file = gi.FindSheetCell(Doodads, class_id, "file");
    PATHSTR buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", dir, file, file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
}

static void SP_SpawnDestructable(edict_t *edict) {
    LPCSTR dir = DESTRUCTABLE_DIRECTORY(edict->class_id);
    LPCSTR file = DESTRUCTABLE_FILE(edict->class_id);
    PATHSTR buffer;
    sprintf(buffer, "%s.blp", DESTRUCTABLE_TEXTURE(edict->class_id));
    edict->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", dir, file, file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
    edict->s.radius = 50;//destr->radius;
    edict->health = DESTRUCTABLE_HIT_POINT_MAXIMUM(edict->class_id);
}

sheetRow_t *find_row(LPCSTR dood_id) {
    FOR_EACH_LIST(sheetRow_t, d, Doodads){
        if (!strcmp(d->name, dood_id))
            return d;
    }
    return NULL;
}

void SP_CallSpawn(edict_t *edict) {
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

void SP_worldspawn(edict_t *ent) {
    SetAbilityNames();
}

void G_SpawnEntities(LPCDOODAD entities) {
    FOR_LOOP(i, game.max_clients) {
        game_state.edicts[i + 1].client = game.clients + i;
    }
    
    globals.num_edicts = game.max_clients + 1;

    FOR_EACH_LIST(DOODAD const, doodad, entities) {
        edict_t *e = G_Spawn();
        entityState_t *s = &e->s;
        s->origin = doodad->position;
        s->angle = doodad->angle;
        s->scale = doodad->scale.x;
        e->class_id = doodad->doodID;
        e->variation = doodad->variation;
        s->player = doodad->player & 7;
        SP_CallSpawn(e);
    }

    SP_worldspawn(NULL);
}
 
