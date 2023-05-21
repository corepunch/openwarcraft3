#include "g_local.h"

#include "Doodads/Doodads.h"
#include "Units/DestructableData.h"
#include "Units/UnitUI.h"

struct spawn {
    LPCSTR name;
    void (*func)(LPEDICT lpEdict);
};

void SP_monster_peon(LPEDICT lpEdict);

static struct spawn spawns[] = {
    { "opeo", SP_monster_peon },
    { NULL, NULL }
};

LPEDICT G_Spawn(void) {
    LPEDICT lpEdict = &game_state.edicts[globals.num_edicts];
    lpEdict->s.number = globals.num_edicts;
    globals.num_edicts++;
    return lpEdict;
}

static void SP_SpawnDoodad(LPEDICT lpEdict, struct Doodads const *lpDoo) {
    PATHSTR buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", lpDoo->dir, lpDoo->file, lpDoo->file, lpEdict->variation);
    lpEdict->s.model = gi.ModelIndex(buffer);
}

static void SP_SpawnDestructable(LPEDICT lpEdict, struct DestructableData const *lpDestr) {
    PATHSTR buffer;
    sprintf(buffer, "%s.blp", lpDestr->texFile);
    lpEdict->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", lpDestr->dir, lpDestr->file, lpDestr->file, lpEdict->variation);
    lpEdict->s.model = gi.ModelIndex(buffer);
}

void SP_CallSpawn(LPEDICT lpEdict) {
    if (!lpEdict->class_id)
        return;
    struct Doodads const *doodadInfo = FindDoodads(lpEdict->class_id);
    struct DestructableData const *destructableData = FindDestructableData(lpEdict->class_id);
    struct UnitUI const *unitUI = FindUnitUI(lpEdict->class_id);
    if (doodadInfo) {
        SP_SpawnDoodad(lpEdict, doodadInfo);
        return;
    }
    if (destructableData) {
        SP_SpawnDestructable(lpEdict, destructableData);
        return;
    }
    if (unitUI) {
        SP_SpawnUnit(lpEdict, unitUI);
        return;
    }
    for (struct spawn *s = spawns; s->func; s++) {
        if (*((int const *)spawns->name) == lpEdict->class_id) {
            s->func(lpEdict);
            return;
        }
    }
}

void G_SpawnEntities(LPCDOODAD doodads, DWORD numDoodads) {
    LPEDICT e = G_Spawn();
    e->class_id = MAKEFOURCC('o', 'p', 'e', 'o');
    e->s.origin = (VECTOR3) { -200, -1600, 100 };
    e->s.angle = 0;
    e->s.scale = 2.5;
    e->s.team = 1;
    SP_CallSpawn(e);
    gi.ModelIndex("UI\\Feedback\\SelectionCircleUnit\\selectioncircleUnit.mdx");

    e = G_Spawn();
    e->class_id = MAKEFOURCC('o', 'p', 'e', 'o');
    e->s.origin = (VECTOR3) { 800, -1400, 100 };
    e->s.angle = -1;
    e->s.scale = 2.5;
    e->s.team = 2;
    SP_CallSpawn(e);

    FOR_LOOP(index, numDoodads) {
        LPCDOODAD doodad = &doodads[index];
        LPEDICT e = G_Spawn();
        LPENTITYSTATE s = &e->s;
        s->origin = doodad->position;
        s->angle = doodad->angle;
        s->scale = doodad->scale.x;
        e->class_id = doodad->doodID;
        e->variation = doodad->variation;
        SP_CallSpawn(e);
    }
}
