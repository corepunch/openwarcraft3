#include "g_local.h"

#include "Doodads/Doodads.h"
#include "Units/DestructableData.h"
#include "Units/UnitUI.h"

struct spawn {
    LPCSTR name;
    void (*func)(LPEDICT edict);
};

void SP_monster_peon(LPEDICT edict);

static struct spawn spawns[] = {
    { "opeo", SP_monster_peon },
    { NULL, NULL }
};

LPEDICT G_Spawn(void) {
    LPEDICT edict = &game_state.edicts[globals.num_edicts];
    edict->s.number = globals.num_edicts;
    globals.num_edicts++;
    return edict;
}

static void SP_SpawnDoodad(LPEDICT edict, struct Doodads const *doo) {
    PATHSTR buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", doo->dir, doo->file, doo->file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
}

static void SP_SpawnDestructable(LPEDICT edict, struct DestructableData const *destr) {
    PATHSTR buffer;
    sprintf(buffer, "%s.blp", destr->texFile);
    edict->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", destr->dir, destr->file, destr->file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
}

void SP_CallSpawn(LPEDICT edict) {
    if (!edict->class_id)
        return;
    LPCDOODADS doodadInfo = NULL;
    LPCDESTRUCTABLEDATA destructableData = NULL;
    LPCUNITUI unitUI = NULL;
    if ((doodadInfo = FindDoodads(edict->class_id))) {
        SP_SpawnDoodad(edict, doodadInfo);
    } else if ((destructableData = FindDestructableData(edict->class_id))) {
        SP_SpawnDestructable(edict, destructableData);
    } else if ((unitUI = FindUnitUI(edict->class_id))) {
        SP_SpawnUnit(edict, unitUI);
        SP_monster_peon(edict);
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

void G_SpawnDoodads(LPCDOODAD entities) {
    FOR_EACH_LIST(DOODAD const, doodad, entities) {
        LPEDICT e = G_Spawn();
        entityState_t *s = &e->s;
        s->origin = doodad->position;
        s->angle = doodad->angle;
        s->scale = doodad->scale.x;
        e->class_id = doodad->doodID;
        e->variation = doodad->variation;
        s->player = doodad->player;
        SP_CallSpawn(e);
    }
}
