#ifndef server_h
#define server_h

#include "../common/common.h"
#include "game.h"

#include <StormLib.h>

#define EDICT_NUM(n) ((LPEDICT)((LPSTR)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) (((LPSTR)(e)-(LPSTR)ge->edicts) / ge->edict_size)

KNOWN_AS(client_frame, CLIENTFRAME);

struct edict {
    ENTITYSTATE s;
};

struct client_frame {
    int num_entities;
    int first_entity;        // into the circular sv_packet_entities[]
};

struct client {
    bool initialized;
    struct client_frame frames[UPDATE_BACKUP];
    struct netchan netchan;
    VECTOR3 camera_position;
    DWORD lastframe;
};

struct server_static {
    struct client clients[MAX_CLIENTS];
    LPENTITYSTATE client_entities;
    bool initialized;
    DWORD num_clients;
    DWORD num_client_entities;
    DWORD next_client_entities;
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
    int num_animations;
};

struct server {
    PATHSTR name;
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    struct cmodel *models[MAX_MODELS];
    int framenum;
    int time;
    LPENTITYSTATE baselines;
};

extern struct game_export *ge;
extern struct server sv;
extern struct server_static svs;

void SV_Map(LPCSTR pFilename);
void SV_InitGame(void);
void SV_BuildClientFrame(struct client *client);
void SV_WriteFrameToClient(struct client *client);
void MSG_WriteDeltaEntity(LPSIZEBUF msg,
                          LPCENTITYSTATE from,
                          LPCENTITYSTATE to);
int SV_ModelIndex(LPCSTR name);
int SV_SoundIndex(LPCSTR name);
int SV_ImageIndex(LPCSTR name);

#endif
