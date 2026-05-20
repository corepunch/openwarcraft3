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
    PLAYER ps; // communicated by server to clients
    int ping;
    // the game dll can add anything it wants after
    // this point in the structure
};

struct edict_s {
    entityState_t s;
    LPGAMECLIENT client;
    pathTex_t *pathtex;
    FLOAT collision;
    BOX2 bounds;
    DWORD svflags;
    DWORD selected;
    DWORD areanum;
    LINK area;
    BOOL inuse;
    BOX2 areabounds;
};

struct client_frame {
    PLAYER ps;
    DWORD num_entities;
    DWORD first_entity;        // into the circular sv_packet_entities[]
};

struct client {
    struct client_frame frames[UPDATE_BACKUP];
    struct netchan netchan;
    clientState_t state;
    edict_t *edict; // EDICT_NUM(clientnum+1)
    DWORD lastframe;
    DWORD drop_time;       // svs.realtime at SV_DropClient; used by
                           // SV_Frame to transition cs_zombie -> cs_free
                           // after ZOMBIE_TIME_MS.
    DWORD last_packet_ms;  // svs.realtime at last received packet from
                           // this client.  Used by SV_RunClientTimeouts
                           // to drop silent clients after CLIENT_TIMEOUT_MS.
};

/* Milliseconds a cs_zombie slot stays unreusable so an unintended
 * reconnect from the same address doesn't immediately get the
 * still-warm state.  Chosen to be longer than typical NAT rebind
 * latency. */
#define ZOMBIE_TIME_MS 2000

/* Milliseconds without a packet from a non-loopback client before we
 * drop them as timed out.  30s tolerates brief network blips while
 * still reclaiming slots from genuinely-dead peers in bounded time. */
#define CLIENT_TIMEOUT_MS 30000

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
    BOOL syncstrings[MAX_CONFIGSTRINGS];
    struct cmodel *models[MAX_MODELS];
    DWORD framenum;
    DWORD time;
    LPENTITYSTATE baselines;
    sizeBuf_t multicast;
    BYTE multicast_buf[MAX_MSGLEN];
} sv;

extern struct game_export *ge;

/* Monotonically incremented whenever an "advertised" property of the
 * server changes (map, client count, max clients).  Consumed by:
 *   - sv_main.c     SV_OOB_GetInfoResponse caches its formatted reply
 *                   keyed on this counter.
 *   - sv_mdns.c     SV_MDNS_UpdateInfo skips its TXT push if the
 *                   counter hasn't moved since the last push.
 * Callers that mutate the relevant state must bump this; see
 * SV_ClientConnect, SV_DirectConnect, SV_Map. */
extern DWORD sv_advertised_state_version;

// sv_init.c
void SV_Map(LPCSTR pFilename);
void SV_InitGame(void);
LPCLIENT SV_FindClientByAddr(const netadr_t *from);
void SV_DirectConnect(const netadr_t *from);

/* Transition `client` to cs_zombie, record drop_time, and bump
 * sv_advertised_state_version.  The slot moves to cs_free after
 * ZOMBIE_TIME_MS via the per-frame sweep in SV_Frame.
 *
 * Does NOT decrement svs.num_clients; cs_free slots within the
 * existing range are reused by SV_DirectConnect before growing
 * num_clients further. */
void SV_DropClient(LPCLIENT client, LPCSTR reason);

/* Count of currently live (cs_connected or cs_spawned) slots.  This
 * is what advertised "clients" fields should report, not
 * svs.num_clients, which counts all slots including cs_zombie / cs_free. */
DWORD SV_NumLiveClients(void);

/* Walk svs.clients and transition cs_zombie -> cs_free for slots whose
 * grace period elapsed.  Called once per game tick from SV_Frame. */
void SV_RunZombieExpiry(void);

/* Walk svs.clients and SV_DropClient any non-loopback cs_connected /
 * cs_spawned slot that hasn't sent a packet in CLIENT_TIMEOUT_MS.
 * Called once per game tick from SV_Frame. */
void SV_RunClientTimeouts(void);
void SV_BuildClientFrame(LPCLIENT client);
void SV_WriteFrameToClient(LPCLIENT client);
void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client);
int SV_ModelIndex(LPCSTR name);
int SV_SoundIndex(LPCSTR name);
int SV_ImageIndex(LPCSTR name);
int SV_FontIndex(LPCSTR name, DWORD fontSize);

// sv_game.c
struct cmodel *SV_LoadModel(LPCSTR filename);

void SV_Multicast(LPCVECTOR3 origin, multicast_t to);
void SV_InitGameProgs(void);

// sv_main.c
void SV_WriteConfigString(LPSIZEBUF msg, DWORD i);

// sv_lan.c
/* Central OOB packet dispatcher.  Called from SV_ReadPackets with the
 * full netchan buffer (including the leading 4-byte -1 marker).  No-op
 * if the packet is shorter than 4 bytes or doesn't carry the OOB
 * sequence marker.
 *
 * Adopted from upstream feature/lan-games' abstraction (corepunch/
 * openwarcraft3@634b82e).  We keep our Quake 3-flavored wire format
 * (OOB_GETINFO / OOB_INFORESPONSE) inside the dispatcher; only the
 * structural pattern is borrowed. */
void SV_ConnectionlessPacket(const netadr_t *from, LPSIZEBUF msg);

// sv_user.c
void SV_ExecuteUserCommand(LPSIZEBUF msg, LPCLIENT client);

// sv_world.c
void SV_LinkEntity(LPEDICT ent);
void SV_UnlinkEntity(LPEDICT ent);
DWORD SV_AreaEdicts(LPCBOX2 area, LPEDICT *list, DWORD maxcount, BOOL (*pred)(LPCEDICT));
void SV_ClearWorld(void);

#endif
