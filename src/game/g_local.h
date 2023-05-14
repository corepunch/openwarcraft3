#ifndef g_local_h
#define g_local_h

#include "../server/game.h"

struct edict {
    struct entity_state s;
};

struct game_state {
    struct edict *edicts;
};

extern struct game_state game_state;

#endif
