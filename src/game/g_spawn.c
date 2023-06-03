#include "g_local.h"

#include "Doodads/Doodads.h"
#include "Units/DestructableData.h"
#include "Units/UnitUI.h"

//struct spawn {
//    LPCSTR name;
//    void (*func)(edict_t *edict);
//};

void SP_monster_unit(edict_t *edict);

//static struct spawn spawns[] = {
//    { "opeo", SP_monster_unit },
//    { NULL, NULL }
//};

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

static void SP_SpawnDoodad(edict_t *edict, struct Doodads const *doo) {
    PATHSTR buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", doo->dir, doo->file, doo->file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
}

static void SP_SpawnDestructable(edict_t *edict, struct DestructableData const *destr) {
    PATHSTR buffer;
    sprintf(buffer, "%s.blp", destr->texFile);
    edict->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", destr->dir, destr->file, destr->file, edict->variation);
    edict->s.model = gi.ModelIndex(buffer);
}

void SP_CallSpawn(edict_t *edict) {
    if (!edict->s.class_id)
        return;
    LPCDOODADS doodadInfo = NULL;
    LPCDESTRUCTABLEDATA destructableData = NULL;
    LPCUNITUI unitUI = NULL;
    if ((doodadInfo = FindDoodads(edict->s.class_id))) {
        SP_SpawnDoodad(edict, doodadInfo);
    } else if ((destructableData = FindDestructableData(edict->s.class_id))) {
        SP_SpawnDestructable(edict, destructableData);
    } else if ((unitUI = FindUnitUI(edict->s.class_id))) {
        SP_SpawnUnit(edict, unitUI);
        SP_monster_unit(edict);
    } else {
        edict->svflags |= SVF_NOCLIENT;
//        fprintf(stderr, "Unknown id %.4s\n", (const char *)&edict->s.class_id);
    }
//    for (struct spawn *s = spawns; s->func; s++) {
//        if (*((int const *)s->name) == edict->s.class_id) {
//            s->func(edict);
//            return;
//        }
//    }
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
        e->s.class_id = doodad->doodID;
        e->variation = doodad->variation;
        s->player = doodad->player & 7;
        SP_CallSpawn(e);
    }
    
    SetAbilityNames();
}
