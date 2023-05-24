#ifndef server_h
#define server_h

#include "../common/common.h"
#include "game.h"

#include <StormLib.h>

#define EDICT_NUM(n) ((LPEDICT)((LPSTR)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) (((LPSTR)(e)-(LPSTR)ge->edicts) / ge->edict_size)

KNOWN_AS(client_frame, CLIENTFRAME);
KNOWN_AS(client, CLIENT);

struct edict {
    entityState_t s;
    DWORD svflags;
};

struct client_frame {
    DWORD num_entities;
    DWORD first_entity;        // into the circular sv_packet_entities[]
};

struct client {
    bool initialized;
    struct client_frame frames[UPDATE_BACKUP];
    struct netchan netchan;
    VECTOR2 camera_position;
    DWORD lastframe;
};

struct server_static {
    struct client clients[MAX_CLIENTS];
    entityState_t *client_entities;
    bool initialized;
    DWORD num_clients;
    DWORD num_client_entities;
    DWORD next_client_entities;
    DWORD realtime;
};

struct mdx_sequence {
    char name[80];
    DWORD interval[2];
    float movespeed;     // movement speed of the entity while playing this animation
    DWORD flags;      // &1: non looping
    float rarity;
    int syncpoint;
    float radius;
    VECTOR3 min;
    VECTOR3 max;
};

struct cmodel {
    struct mdx_sequence *animations;
    struct AnimationInfo animtypes[NUM_ANIM_TYPES];
    int num_animations;
};

struct server {
    PATHSTR name;
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    struct cmodel *models[MAX_MODELS];
    int framenum;
    int time;
    entityState_t *baselines;
};

extern struct game_export *ge;
extern struct server sv;
extern struct server_static svs;

void SV_Map(LPCSTR pFilename);
void SV_InitGame(void);
void SV_BuildClientFrame(LPCLIENT client);
void SV_WriteFrameToClient(LPCLIENT client);
void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client);
void MSG_WriteDeltaEntity(LPSIZEBUF msg, entityState_t const *from, entityState_t const *to);
int SV_ModelIndex(LPCSTR name);
int SV_SoundIndex(LPCSTR name);
int SV_ImageIndex(LPCSTR name);

#endif
