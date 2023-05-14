#ifndef server_h
#define server_h

#include "../common/common.h"
#include "game.h"

#define EDICT_NUM(n) ((struct edict *)((char *)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) ( ((char *)(e)-(char *)ge->edicts ) / ge->edict_size)

struct edict {
    struct entity_state s;
};

struct client {
    bool initialized;
    struct netchan netchan;
};

struct server_static {
    struct client clients[MAX_CLIENTS];
    int num_clients;
};

struct server {
    path_t name;
    path_t configstrings[MAX_CONFIGSTRINGS];
};

void SV_Map(LPCSTR pFilename);

extern struct game_export *ge;
extern struct server sv;

//struct PathMapNode const *SV_PathMapNode(struct Terrain const *heightmap, int x, int y);

#endif
