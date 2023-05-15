#ifndef server_h
#define server_h

#include "../common/common.h"
#include "game.h"

#define EDICT_NUM(n) ((struct edict *)((char *)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) ( ((char *)(e)-(char *)ge->edicts ) / ge->edict_size)

struct edict {
    struct entity_state s;
};

struct client_frame {
    int num_entities;
    int first_entity;        // into the circular sv_packet_entities[]
};

struct client {
    bool initialized;
    struct client_frame frames[UPDATE_BACKUP];
    struct netchan netchan;
    struct vector3 camera_position;
    int lastframe;
};

struct server_static {
    struct client clients[MAX_CLIENTS];
    struct entity_state *client_entities;
    bool initialized;
    int num_clients;
    int num_client_entities;
    int next_client_entities;
};

struct server {
    path_t name;
    path_t configstrings[MAX_CONFIGSTRINGS];
    int framenum;
    int time;
    struct entity_state *baselines;
};

extern struct game_export *ge;
extern struct server sv;
extern struct server_static svs;

void SV_Map(LPCSTR pFilename);
void SV_InitGame(void);
void SV_BuildClientFrame(struct client *client);
void SV_WriteFrameToClient(struct client *client);
void MSG_WriteDeltaEntity(struct sizebuf *msg, struct entity_state const *from, struct entity_state const *to);
int SV_ModelIndex(LPCSTR name);
int SV_SoundIndex(LPCSTR name);
int SV_ImageIndex(LPCSTR name);

#endif
