#include "g_local.h"

#define MAX_ENTITIES 4096

static struct game_export globals;

struct game_import gi;
struct game_state game_state;

void Game_Init(void) {
    game_state.edicts = gi.MemAlloc(sizeof(struct edict) * MAX_ENTITIES);
    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;
}

void Game_Shutdown(void) {
    gi.MemFree(game_state.edicts);
}

static int G_LoadModelDirFile(LPCSTR szDirectory, LPCSTR szFileName, int dwVariation) {
    path_t buffer;
    sprintf(buffer, "%s\\%s\\%s%d.mdx", szDirectory, szFileName, szFileName, dwVariation);
    return gi.ModelIndex(buffer);
//    struct tModel *model = R_LoadModel(buffer);
//    if (model) {
//        return model;
//    } else {
//        sprintf(buffer, "%s\\%s\\%s.mdx", szDirectory, szFileName, szFileName);
//        return R_LoadModel(buffer);
//    }
}

struct Doodad *DoodadAtIndex(struct Doodad const *doodads, int index) {
    char *doo = (char *)doodads;
    return (struct Doodad *)(doo + index * DOODAD_SIZE);
}

void Game_SpawnEntities(struct Doodad const *doodads, int numDoodads) {
    globals.num_edicts = 0;
//    struct edict *e = &game_state.edicts[globals.num_edicts++];
//    e->s.model = G_LoadModelDirFile("Units\\Orc", "Peon", 0);
//    e->s.model = G_LoadModelDirFile("Doodads\\Terrain", "WoodBridgeLarge45", 0);
//    e->s.position = (struct vector3) { 716, -1360, 100 };
//    e->s.angle = -45;
//    e->s.scale = (struct vector3) { 5, 5, 5 };
    FOR_LOOP(index, numDoodads) {
        struct Doodad const *doodad = DoodadAtIndex(doodads, index);
        struct DoodadInfo const *doodadInfo = FindDoodadInfo(doodad->doodID);
        struct DestructableInfo const *destructableInfo = FindDestructableInfo(doodad->doodID);
        struct edict *e = &game_state.edicts[globals.num_edicts++];
        struct entity_state *s = &e->s;
        s->position = doodad->position;
        s->angle = doodad->angle;
        s->scale = doodad->scale;

        if (doodadInfo) {
            s->model = G_LoadModelDirFile(doodadInfo->dir, doodadInfo->file, doodad->variation);
        } else if (destructableInfo) {
            path_t buffer;
            sprintf(buffer, "%s.blp", destructableInfo->texFile);
            s->model = G_LoadModelDirFile(destructableInfo->dir, destructableInfo->file, doodad->variation);
            s->image = gi.ImageIndex(buffer);
        }
    }
}

struct game_export *GetGameAPI(struct game_import *import) {
    memset(&game_state, 0, sizeof(struct game_state));
    gi = *import;
    globals.Init = Game_Init;
    globals.Shutdown = Game_Shutdown;
    globals.SpawnEntities = Game_SpawnEntities;
    globals.edict_size = sizeof(struct edict);
    return &globals;
}
