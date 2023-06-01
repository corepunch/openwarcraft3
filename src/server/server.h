#ifndef server_h
#define server_h

#include "../common/common.h"
#include "game.h"

#include <StormLib.h>

#define EDICT_NUM(n) ((edict_t *)((LPSTR)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) (DWORD)(((LPSTR)(e)-(LPSTR)ge->edicts) / ge->edict_size)

KNOWN_AS(client_frame, CLIENTFRAME);
KNOWN_AS(client, CLIENT);

struct gclient_s {
    playerState_t ps; // communicated by server to clients
    int ping;
    // the game dll can add anything it wants after
    // this point in the structure
};

struct edict_s {
    entityState_t s;
    gclient_t *client;
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
    edict_t *edict; // EDICT_NUM(clientnum+1)
    VECTOR2 camera_position;
    DWORD lastframe;
};

extern struct server_static {
    struct client clients[MAX_CLIENTS];
    entityState_t *client_entities;
    bool initialized;
    DWORD num_clients;
    DWORD num_client_entities;
    DWORD next_client_entities;
    DWORD realtime;
} svs;

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

typedef struct {
    animationInfo_t animations[MAX_ANIMS_IN_TYPE];
    DWORD num_animations;
} animationTypeVariants_t;

struct cmodel {
    struct mdx_sequence *animations;
    animationTypeVariants_t animtypes[NUM_ANIM_TYPES];
    DWORD num_animations;
};

extern struct server {
    PATHSTR name;
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    struct cmodel *models[MAX_MODELS];
    DWORD framenum;
    DWORD time;
    entityState_t *baselines;
    struct sizebuf multicast;
    BYTE multicast_buf[MAX_MSGLEN];
} sv;

extern struct game_export *ge;

void SV_Map(LPCSTR pFilename);
void SV_InitGame(void);
void SV_BuildClientFrame(LPCLIENT client);
void SV_WriteFrameToClient(LPCLIENT client);
void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client);
int SV_ModelIndex(LPCSTR name);
int SV_SoundIndex(LPCSTR name);
int SV_ImageIndex(LPCSTR name);

void SV_Multicast(LPCVECTOR3 origin, multicast_t to);
void SV_InitGameProgs(void);

#endif
