#include "g_sc2_local.h"
#include "games/starcraft-2/common/sc2_map.h"
#include <stdio.h>
#include <string.h>

struct game_import gi;
struct game_export globals;

static edict_t sc2_edicts[SC2_MAX_EDICTS];
static struct client_s sc2_clients[SC2_MAX_CLIENTS];

static void SC2_Init(void) {
    memset(sc2_edicts,  0, sizeof(sc2_edicts));
    memset(sc2_clients, 0, sizeof(sc2_clients));

    globals.edicts     = sc2_edicts;
    globals.num_edicts = 1;
    globals.max_edicts = SC2_MAX_EDICTS;
    globals.max_clients = SC2_MAX_CLIENTS;
}

static void SC2_Shutdown(void) {
    G_FreeModels();
    SC2_MapShutdown();
}

static void SC2_SpawnEntities(void);

static bool SC2_LoadMap(LPCSTR mapFilename) {
    if (!CM_LoadMap(mapFilename)) {
        return false;
    }
    if (gi.ApplyLobbySettings) {
        gi.ApplyLobbySettings((LPMAPINFO)CM_GetMapInfo());
    }
    if (gi.ClearWorld) {
        gi.ClearWorld();
    }
    SC2_SpawnEntities();
    return true;
}

static void SC2_SpawnEntities(void) {
    sc2Map_t const *map = SC2_MapCurrent();

    globals.num_edicts = globals.max_clients;

    FOR_LOOP(i, map->num_objects) {
        sc2MapObject_t const *object = &map->objects[i];
        LPEDICT ent;

        if (!object->model[0]) {
            continue;
        }
        if (globals.num_edicts >= globals.max_edicts) {
            fprintf(stderr, "SC2_SpawnEntities: hit max edicts at %u objects\n", (unsigned)i);
            break;
        }
        ent = &sc2_edicts[globals.num_edicts++];
        memset(ent, 0, sizeof(*ent));
        ent->inuse = true;
        ent->s.number = (DWORD)(ent - sc2_edicts);
        ent->s.class_id = SC2_MapObjectClassId(object);
        ent->s.origin = object->position;
        ent->s.origin.z = CM_GetHeightAtPoint(object->position.x, object->position.y) + object->position.z;
        ent->s.angle = object->angle;
        ent->s.scale = object->scale > 0.0f ? object->scale : 1.0f;
        ent->s.radius = object->type == SC2_OBJECT_UNIT ? 1.0f : 0.5f;
        ent->s.player = object->player;
        ent->s.model = G_RegisterModel(object->model);
        ent->s.flags |= EF_GROUND_ANCHOR;
        ent->collision = ent->s.radius;
        gi.LinkEntity(ent);
    }
    CM_BakeStaticObstacles();
}

static void SC2_RunFrame(void) {
}

static void SC2_ClientBegin(LPEDICT ent) {
    (void)ent;
}

static void SC2_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    (void)ent; (void)argc; (void)argv;
}

static void SC2_ClientSetCameraPosition(LPEDICT ent, LPCVECTOR2 position) {
    (void)ent; (void)position;
}

static BOOL SC2_CanSeeEntity(DWORD player, LPCEDICT ent) {
    (void)player; (void)ent;
    return true;
}

static LPCSTR SC2_GetThemeValue(LPCSTR filename) {
    (void)filename;
    return NULL;
}

struct game_export *GetGameAPI(struct game_import *import) {
    gi = *import;

    globals.Init                  = SC2_Init;
    globals.Shutdown              = SC2_Shutdown;
    globals.RunFrame              = SC2_RunFrame;
    globals.ClientBegin           = SC2_ClientBegin;
    globals.ClientCommand         = SC2_ClientCommand;
    globals.ClientSetCameraPosition = SC2_ClientSetCameraPosition;
    globals.CanSeeEntity          = SC2_CanSeeEntity;
    globals.GetThemeValue         = SC2_GetThemeValue;
    globals.LoadMap               = SC2_LoadMap;
    globals.GetWorldBounds        = CM_GetWorldBounds;
    globals.edict_size            = sizeof(edict_t);
    globals.max_clients           = SC2_MAX_CLIENTS;
    globals.max_edicts            = SC2_MAX_EDICTS;

    return &globals;
}
