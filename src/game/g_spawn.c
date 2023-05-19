#include "g_local.h"

struct spawn {
    LPCSTR name;
    void (*func)(LPEDICT lpEdict);
};

void SP_monster_peon(LPEDICT lpEdict);

static struct spawn spawns[] = {
    { "opeo", SP_monster_peon },
    { NULL, NULL }
};

LPDOODAD DoodadAtIndex(LPCDOODAD doodads, int index) {
    LPBYTE doo = (LPBYTE)doodads;
    return (LPDOODAD)(doo + index * DOODAD_SIZE);
}

LPEDICT G_Spawn(void) {
    LPEDICT lpEdict = &game_state.edicts[globals.num_edicts];
    lpEdict->s.number = globals.num_edicts;
    globals.num_edicts++;
    return lpEdict;
}

static void SP_SpawnDoodad(LPEDICT lpEdict, LPCDOODADINFO lpDoo) {
    PATHSTR buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", lpDoo->dir, lpDoo->file, lpDoo->file, lpEdict->variation);
    lpEdict->s.model = gi.ModelIndex(buffer);
}

static void SP_SpawnDestructable(LPEDICT lpEdict, LPCDESTRUCTABLEDATA lpDestr) {
    PATHSTR buffer;
    sprintf(buffer, "%s.blp", lpDestr->texFile);
    lpEdict->s.image = gi.ImageIndex(buffer);
    sprintf(buffer, "%s\\%s\\%s%d.mdx", lpDestr->dir, lpDestr->file, lpDestr->file, lpEdict->variation);
    lpEdict->s.model = gi.ModelIndex(buffer);
}

void SP_CallSpawn(LPEDICT lpEdict) {
    if (!lpEdict->class_id)
        return;
    LPCDOODADINFO doodadInfo = NULL;
    LPCDESTRUCTABLEDATA destructableData = NULL;
    LPCUNITUI unitUI = NULL;
    if ((doodadInfo = G_FindDoodadInfo(lpEdict->class_id))) {
        SP_SpawnDoodad(lpEdict, doodadInfo);
        return;
    }
    if ((destructableData = G_FindDestructableData(lpEdict->class_id))) {
        SP_SpawnDestructable(lpEdict, destructableData);
        return;
    }
    if ((unitUI = G_FindUnitUI(lpEdict->class_id))) {
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
    globals.num_edicts = 0;
    LPEDICT e = G_Spawn();
    e->class_id = MAKEFOURCC('o', 'p', 'e', 'o');
    e->s.origin = (VECTOR3) { 600, -1600, 100 };
    e->s.angle = -20;
    e->s.scale = 2;
    e->s.team = 1;
    SP_CallSpawn(e);
    gi.ModelIndex("UI\\Feedback\\SelectionCircleUnit\\selectioncircleUnit.mdx");
    FOR_LOOP(index, numDoodads) {
        LPCDOODAD doodad = DoodadAtIndex(doodads, index);
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
