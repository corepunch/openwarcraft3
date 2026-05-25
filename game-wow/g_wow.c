#include "../server/server.h"
#include <strings.h>

#define WOW_MAX_CLIENTS 1
#define WOW_MAX_EDICTS WOW_MAX_CLIENTS
#define WOW_PLAYER_MODEL "Character\\Orc\\Male\\OrcMale.m2"

static struct game_import gi;
static struct game_export globals;
static edict_t wow_edicts[WOW_MAX_EDICTS];
static struct client_s wow_clients[WOW_MAX_CLIENTS];

static void Wow_InitPlayer(LPEDICT ent) {
    LPPLAYER ps;

    memset(ent, 0, sizeof(*ent));
    ent->client = &wow_clients[0];
    ent->inuse = true;
    ent->s.number = 0;
    ent->s.model = gi.ModelIndex ? gi.ModelIndex(WOW_PLAYER_MODEL) : 0;
    ent->s.origin = (VECTOR3){ WOW_START_POSITION_X, WOW_START_POSITION_Y, gi.GetHeightAtPoint ? gi.GetHeightAtPoint(WOW_START_POSITION_X, WOW_START_POSITION_Y) : WOW_START_POSITION_Z };
    ent->s.scale = 1.0f;
    ent->s.radius = 1.0f;
    ent->s.renderfx = RF_NO_SHADOW;

    ps = &ent->client->ps;
    memset(ps, 0, sizeof(*ps));
    ps->number = 0;
    ps->origin = (VECTOR2){ WOW_START_POSITION_X, WOW_START_POSITION_Y };
#ifdef WOW
    ps->viewquat = Quaternion_fromEuler(&MAKE(VECTOR3, 328.0f, 0.0f, 0.0f), ROTATE_ZYX);
    ps->fov = 45;
    ps->distance = 10.0f;
#else
    ps->viewquat = Quaternion_fromEuler(&MAKE(VECTOR3, 326.0f, 0.0f, 0.0f), ROTATE_ZYX);
    ps->fov = 54;
    ps->distance = 250.0f;
#endif
    ps->client_ui_state = CLIENT_UI_GAME;
}

static void Wow_Init(void) {
    memset(wow_edicts, 0, sizeof(wow_edicts));
    memset(wow_clients, 0, sizeof(wow_clients));

    globals.edicts = wow_edicts;
    globals.max_edicts = WOW_MAX_EDICTS;
    globals.max_clients = WOW_MAX_CLIENTS;
    globals.num_edicts = WOW_MAX_CLIENTS;
    globals.edict_size = sizeof(edict_t);

    Wow_InitPlayer(&wow_edicts[0]);
}

static void Wow_Shutdown(void) {
    globals.edicts = NULL;
    globals.num_edicts = 0;
}

static void Wow_SpawnEntities(LPCMAPINFO mapinfo, LPCDOODAD doodads) {
    (void)mapinfo;
    (void)doodads;
    Wow_InitPlayer(&wow_edicts[0]);
    globals.num_edicts = WOW_MAX_CLIENTS;
    fprintf(stderr, "WoW doodads: static ADT doodads are renderer-owned and not synced as entities\n");
}

static void Wow_RunFrame(void) {
}

static LPCSTR Wow_GetThemeValue(LPCSTR filename) {
    return filename ? filename : "";
}

static void Wow_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    (void)ent;
    (void)argc;
    (void)argv;
}

static void Wow_ClientSetCameraPosition(LPEDICT ent, LPCVECTOR2 position) {
    if (!ent || !ent->client || !position) {
        return;
    }
    ent->client->ps.origin = *position;
}

static void Wow_ClientBegin(LPEDICT ent) {
    if (!ent || ent->client) {
        return;
    }
    ent->client = &wow_clients[0];
}

struct game_export *GetGameAPI(struct game_import *import) {
    gi = *import;
    (void)gi;

    globals.Init = Wow_Init;
    globals.Shutdown = Wow_Shutdown;
    globals.SpawnEntities = Wow_SpawnEntities;
    globals.RunFrame = Wow_RunFrame;
    globals.GetThemeValue = Wow_GetThemeValue;
    globals.ClientCommand = Wow_ClientCommand;
    globals.ClientSetCameraPosition = Wow_ClientSetCameraPosition;
    globals.ClientBegin = Wow_ClientBegin;
    globals.max_edicts = WOW_MAX_EDICTS;
    globals.max_clients = WOW_MAX_CLIENTS;
    globals.edict_size = sizeof(edict_t);

    return &globals;
}
