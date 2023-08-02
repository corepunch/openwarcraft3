#ifndef server_h
#define server_h

#include "../common/common.h"
#include "game.h"

#include <StormLib.h>

#define EDICT_NUM(n) ((edict_t *)((LPSTR)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) (DWORD)(((LPSTR)(e)-(LPSTR)ge->edicts) / ge->edict_size)

KNOWN_AS(client_frame, CLIENTFRAME);
KNOWN_AS(client, CLIENT);

typedef enum {
    ss_dead, // no map loaded
    ss_loading, // spawning level edicts
    ss_game, // actively running
    ss_cinematic,
    ss_demo,
    ss_pic
} serverState_t;

typedef enum {
    cs_free,        // can be reused for a new connection
    cs_zombie,      // client has been disconnected, but don't reuse
                    // connection for a couple seconds
    cs_connected,   // has been assigned to a client_t, but not in game yet
    cs_spawned      // client is fully in game
} clientState_t;

struct client_s {
    playerState_t ps; // communicated by server to clients
    int ping;
    // the game dll can add anything it wants after
    // this point in the structure
};

struct edict_s {
    entityState_t s;
    LPGAMECLIENT client;
    pathTex_t *pathtex;
    DWORD svflags;
    DWORD selected;
};

struct client_frame {
    playerState_t ps;
    DWORD num_entities;
    DWORD first_entity;        // into the circular sv_packet_entities[]
};

struct client {
    struct client_frame frames[UPDATE_BACKUP];
    struct netchan netchan;
    clientState_t state;
    edict_t *edict; // EDICT_NUM(clientnum+1)
    DWORD lastframe;
};

extern struct server_static {
    struct client clients[MAX_CLIENTS];
    LPENTITYSTATE client_entities;
    bool initialized;
    DWORD num_clients;
    DWORD num_client_entities;
    DWORD next_client_entities;
    DWORD realtime;
} svs;

typedef enum {
    SHAPETYPE_BOX,
    SHAPETYPE_PLANE,
    SHAPETYPE_SPHERE,
    SHAPETYPE_CYLINDER,
} MODELCOLLISIONSHAPETYPE;

struct cmodel {
    LPANIMATION animations;
    DWORD num_animations;
};

extern struct server {
    serverState_t state;
    PATHSTR name;
    PATHSTR configstrings[MAX_CONFIGSTRINGS];
    struct cmodel *models[MAX_MODELS];
    DWORD framenum;
    DWORD time;
    LPENTITYSTATE baselines;
    sizeBuf_t multicast;
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
int SV_FontIndex(LPCSTR name, DWORD fontSize);

void SV_Multicast(LPCVECTOR3 origin, multicast_t to);
void SV_InitGameProgs(void);

// sv_main.c
void SV_WriteConfigString(LPSIZEBUF msg, DWORD i);

// sv_user.c
void SV_ExecuteUserCommand(LPSIZEBUF msg, LPCLIENT client);

#endif
