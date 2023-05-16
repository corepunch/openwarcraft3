#include "g_local.h"

struct spawn {
    LPCSTR name;
    void (*func)(struct edict *ent);
};

void SP_monster_peon(struct edict *ent);

static struct spawn spawns[] = {
    { "peon", SP_monster_peon },
    { NULL, NULL }
};

void SP_CallSpawn(struct edict *ent) {
    if (!ent->classname) {
        return;
    }
    for (struct spawn *s = spawns; s->func; s++) {
        if (!strcmp(spawns->name, ent->classname)) {
            s->func(ent);
            return;
        }
    }
}

struct Doodad *DoodadAtIndex(struct Doodad const *doodads, int index) {
    char *doo = (char *)doodads;
    return (struct Doodad *)(doo + index * DOODAD_SIZE);
}

struct edict *G_Spawn(void) {
    struct edict *ent = &game_state.edicts[globals.num_edicts];
    ent->s.number = globals.num_edicts;
    globals.num_edicts++;
    return ent;
}

void SP_SpawnTestEntity(void) {
    
}

void G_SpawnEntities(struct Doodad const *doodads, int numDoodads) {
    globals.num_edicts = 0;
    struct edict *e = G_Spawn();
//    e->s.model = G_LoadModelDirFile("Doodads\\Terrain", "WoodBridgeLarge45", 0);
    e->classname = "peon";
    e->s.origin = (struct vector3) { 716, -1360, 100 };
    e->s.angle = -20;
    e->s.scale = 2;
    SP_CallSpawn(e);
    FOR_LOOP(index, numDoodads) {
        struct Doodad const *doodad = DoodadAtIndex(doodads, index);
        struct DoodadInfo const *doodadInfo = G_FindDoodadInfo(doodad->doodID);
        struct DestructableData const *DestructableData = G_FindDestructableData(doodad->doodID);
        struct edict *e = G_Spawn();
        struct entity_state *s = &e->s;
        s->origin = doodad->position;
        s->angle = doodad->angle;
        s->scale = doodad->scale.x;

        if (doodadInfo) {
            s->model = G_LoadModelDirFile(doodadInfo->dir, doodadInfo->file, doodad->variation);
        } else if (DestructableData) {
            path_t buffer;
            sprintf(buffer, "%s.blp", DestructableData->texFile);
            s->model = G_LoadModelDirFile(DestructableData->dir, DestructableData->file, doodad->variation);
            s->image = gi.ImageIndex(buffer);
        }
    }
}
