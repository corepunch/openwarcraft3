#include "g_local.h"

struct spawn {
    LPCSTR name;
    void (*func)(struct edict *ent);
};

void SP_monster_peon(struct edict *ent);

static struct spawn spawns[] = {
    { "opeo", SP_monster_peon },
    { NULL, NULL }
};

void unit_think(struct edict *ent, int msec) {
    struct animation_info const *anim = &ent->monsterinfo.animation;
    if (anim->start_frame == anim->end_frame)
        return;
    ent->s.frame += msec;
    while (ent->s.frame >= anim->end_frame) {
        ent->s.frame -= MAX(1, anim->end_frame - anim->start_frame);
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

static void
SP_SpawnDoodad(struct edict *ent,
               struct DoodadInfo const *doo)
{
    path_t buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", doo->dir, doo->file, doo->file, ent->variation);
    ent->s.model = gi.ModelIndex(buffer);
}

static void
SP_SpawnDestructable(struct edict *ent,
                     struct DestructableData const *destr)
{
    path_t buffer;
    sprintf(buffer, "%s.blp", destr->texFile);
    ent->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", destr->dir, destr->file, destr->file, ent->variation);
    ent->s.model = gi.ModelIndex(buffer);
}

static void
SP_SpawnUnit(struct edict *ent,
             struct UnitUI const *unit)
{
    path_t buffer;
    sprintf(buffer, "%s.mdx", unit->file);
    ent->s.model = gi.ModelIndex(buffer);
    ent->monsterinfo.animation = gi.GetAnimation(ent->s.model, "Stand");
    ent->s.frame = ent->monsterinfo.animation.start_frame;
    ent->think = unit_think;
}

void SP_CallSpawn(struct edict *ent) {
    if (!ent->class_id)
        return;
    struct DoodadInfo const *doodadInfo = NULL;
    struct DestructableData const *destructableData = NULL;
    struct UnitUI const *unitUI = NULL;
    if ((doodadInfo = G_FindDoodadInfo(ent->class_id))) {
        SP_SpawnDoodad(ent, doodadInfo);
        return;
    }
    if ((destructableData = G_FindDestructableData(ent->class_id))) {
        SP_SpawnDestructable(ent, destructableData);
        return;
    }
    if ((unitUI = G_FindUnitUI(ent->class_id))) {
        SP_SpawnUnit(ent, unitUI);
        return;
    }
    for (struct spawn *s = spawns; s->func; s++) {
        if (*((int const *)spawns->name) == ent->class_id) {
            s->func(ent);
            return;
        }
    }
}

void G_SpawnEntities(struct Doodad const *doodads, int numDoodads) {
    globals.num_edicts = 0;
    struct edict *e = G_Spawn();
    e->class_id = MAKEFOURCC('o', 'p', 'e', 'o');
    e->s.origin = (struct vector3) { 716, -1360, 100 };
    e->s.angle = -20;
    e->s.scale = 2;
    SP_CallSpawn(e);
    FOR_LOOP(index, numDoodads) {
        struct Doodad const *doodad = DoodadAtIndex(doodads, index);
        struct edict *e = G_Spawn();
        struct entity_state *s = &e->s;
        s->origin = doodad->position;
        s->angle = doodad->angle;
        s->scale = doodad->scale.x;
        e->class_id = doodad->doodID;
        e->variation = doodad->variation;
        SP_CallSpawn(e);
    }
}
