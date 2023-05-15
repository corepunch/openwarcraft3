#include "g_local.h"

#define MAX_ENTITIES 4096

struct game_export globals;
struct game_import gi;
struct game_state game_state;

void G_Init(void) {
    game_state.edicts = gi.MemAlloc(sizeof(struct edict) * MAX_ENTITIES);
    globals.edicts = game_state.edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;
}

void G_Shutdown(void) {
    gi.MemFree(game_state.edicts);
}

int G_LoadModelDirFile(LPCSTR szDirectory, LPCSTR szFileName, int dwVariation) {
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

static void G_RunEntity(struct edict *ent, int msec) {
    if (ent->think) {
        ent->think(ent, msec);
    }
    ent->s.frame++;
}

void G_RunFrame(int msec) {
    FOR_LOOP(i, globals.num_edicts) {
        G_RunEntity(&globals.edicts[i], msec);
    }
}

struct game_export *GetGameAPI(struct game_import *import) {
    memset(&game_state, 0, sizeof(struct game_state));
    gi = *import;
    globals.Init = G_Init;
    globals.Shutdown = G_Shutdown;
    globals.SpawnEntities = G_SpawnEntities;
    globals.RunFrame = G_RunFrame;
    globals.edict_size = sizeof(struct edict);
    return &globals;
}
