#ifndef g_local_h
#define g_local_h

#include "../server/game.h"

struct monstermove {
    int firstframe;
    int lastframe;
};

struct monsterinfo {
    struct monstermove currentmove;
};

struct edict {
    struct entity_state s;

    LPCSTR classname;
    
    void (*think)(struct edict *, int msec);

    struct monsterinfo monsterinfo;
};

struct game_state {
    struct edict *edicts;
};

struct edict *G_Spawn(void);
int G_LoadModelDirFile(LPCSTR szDirectory, LPCSTR szFileName, int dwVariation);
void SP_CallSpawn(struct edict *ent);
void G_SpawnEntities(struct Doodad const *doodads, int numDoodads);

extern struct game_state game_state;
extern struct game_export globals;
extern struct game_import gi;

#endif
