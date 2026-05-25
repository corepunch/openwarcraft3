#include "../server/server.h"
#include <math.h>
#include <stdlib.h>
#include <strings.h>

#define WOW_MAX_CLIENTS 1
#define WOW_MAX_EDICTS WOW_MAX_CLIENTS
#define WOW_PLAYER_MODEL "Character\\Orc\\Male\\OrcMale.m2"
#define WOW_MOVE_FORWARD 1
#define WOW_MOVE_BACK 2
#define WOW_MOVE_LEFT 4
#define WOW_MOVE_RIGHT 8
#define WOW_WALK_SPEED 7.0f
#define WOW_CAMERA_MIN_PITCH 300.0f
#define WOW_CAMERA_MAX_PITCH 350.0f
#define WOW_CAMERA_MIN_DISTANCE 3.0f
#define WOW_CAMERA_MAX_DISTANCE 35.0f

static struct game_import gi;
static struct game_export globals;
static edict_t wow_edicts[WOW_MAX_EDICTS];
static struct client_s wow_clients[WOW_MAX_CLIENTS];
static VECTOR2 wow_spawn_origin = { 0.0f, 0.0f };
static struct {
    DWORD flags;
    FLOAT yaw;
    FLOAT pitch;
    FLOAT distance;
} wow_move = {
    .pitch = 328.0f,
    .distance = 8.5f,
};

static FLOAT Wow_Clamp(FLOAT value, FLOAT min_value, FLOAT max_value) {
    return MAX(min_value, MIN(value, max_value));
}

static FLOAT Wow_TerrainHeight(FLOAT x, FLOAT y) {
    return gi.GetHeightAtPoint ? gi.GetHeightAtPoint(x, y) : 0.0f;
}

static FLOAT Wow_ViewPitch(FLOAT wrapped_pitch) {
    return wrapped_pitch > 180.0f ? 360.0f - wrapped_pitch : -wrapped_pitch;
}

static void Wow_AngleVectors(FLOAT yaw, LPVECTOR2 forward, LPVECTOR2 right) {
    FLOAT angle = (FLOAT)DEG2RAD(yaw);
    FLOAT sy = sinf(angle);
    FLOAT cy = cosf(angle);

    if (forward) {
        forward->x = cy;
        forward->y = sy;
    }
    if (right) {
        right->x = sy;
        right->y = -cy;
    }
}

static void Wow_UpdateCamera(LPEDICT ent) {
    if (!ent || !ent->client) {
        return;
    }
    ent->client->ps.origin = (VECTOR2){ ent->s.origin.x, ent->s.origin.y };
    ent->client->ps.viewangles = (VECTOR3){ Wow_ViewPitch(wow_move.pitch), wow_move.yaw, 0.0f };
    ent->client->ps.viewquat = Quaternion_fromEuler(&MAKE(VECTOR3, wow_move.pitch, 0.0f, wow_move.yaw), ROTATE_ZYX);
    ent->client->ps.fov = 45.0f;
    ent->client->ps.distance = wow_move.distance;
}

static DWORD Wow_FrameForAnimation(LPEDICT ent, LPCSTR animation_name) {
    LPCANIMATION anim;
    DWORD length;

    if (!ent || !gi.GetAnimation || !animation_name || ent->s.model == 0) {
        return 0;
    }
    anim = gi.GetAnimation(ent->s.model, animation_name);
    if (!anim || anim->interval[1] <= anim->interval[0]) {
        return 0;
    }
    length = anim->interval[1] - anim->interval[0];
    if (length <= 1) {
        return anim->interval[0];
    }
    if (anim->flags & 1) {
        return MIN(anim->interval[0] + gi.GetTime(), anim->interval[1] - 1);
    }
    return anim->interval[0] + (gi.GetTime() % length);
}

static void Wow_InitPlayer(LPEDICT ent) {
    LPPLAYER ps;
    FLOAT height = Wow_TerrainHeight(wow_spawn_origin.x, wow_spawn_origin.y);

    memset(ent, 0, sizeof(*ent));
    ent->client = &wow_clients[0];
    ent->inuse = true;
    ent->s.number = 0;
    ent->s.model = gi.ModelIndex ? gi.ModelIndex(WOW_PLAYER_MODEL) : 0;
    ent->s.origin = (VECTOR3){ wow_spawn_origin.x, wow_spawn_origin.y, height };
    ent->s.origin2 = (VECTOR2){ ent->s.origin.x, ent->s.origin.y };
    ent->s.rotation = (VECTOR3){ wow_move.yaw, 0.0f, 0.0f };
    ent->s.scale = 1.0f;
    ent->s.radius = 1.0f;
    ent->s.renderfx = RF_NO_SHADOW;
    ent->s.flags = EF_GROUND_ANCHOR;
    ent->s.frame = Wow_FrameForAnimation(ent, "Stand");

    ps = &ent->client->ps;
    memset(ps, 0, sizeof(*ps));
    ps->number = 0;
#ifdef WOW
    ps->origin = wow_spawn_origin;
    ps->viewangles = (VECTOR3){ Wow_ViewPitch(wow_move.pitch), wow_move.yaw, 0.0f };
    ps->viewquat = Quaternion_fromEuler(&MAKE(VECTOR3, wow_move.pitch, 0.0f, wow_move.yaw), ROTATE_ZYX);
    ps->fov = 45;
    ps->distance = wow_move.distance;
#else
    ps->origin = wow_spawn_origin;
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
    (void)doodads;
    if (mapinfo && mapinfo->players[0].used) {
        wow_spawn_origin = mapinfo->players[0].startingPosition;
    }
    wow_move.flags = 0;
    wow_move.yaw = 0.0f;
    wow_move.pitch = 328.0f;
    wow_move.distance = 8.5f;
    Wow_InitPlayer(&wow_edicts[0]);
    globals.num_edicts = WOW_MAX_CLIENTS;
    fprintf(stderr, "WoW doodads: static ADT doodads are renderer-owned and not synced as entities\n");
}

static void Wow_RunFrame(void) {
    LPEDICT ent = &wow_edicts[0];
    VECTOR2 forward;
    VECTOR2 right;
    VECTOR2 dir = { 0.0f, 0.0f };
    FLOAT len;
    BOOL moving;

    if (!ent->inuse || !ent->client) {
        return;
    }

    Wow_AngleVectors(wow_move.yaw, &forward, &right);

    if (wow_move.flags & WOW_MOVE_FORWARD) {
        dir.x += forward.x;
        dir.y += forward.y;
    }
    if (wow_move.flags & WOW_MOVE_BACK) {
        dir.x -= forward.x;
        dir.y -= forward.y;
    }
    if (wow_move.flags & WOW_MOVE_LEFT) {
        dir.x -= right.x;
        dir.y -= right.y;
    }
    if (wow_move.flags & WOW_MOVE_RIGHT) {
        dir.x += right.x;
        dir.y += right.y;
    }

    len = sqrtf(dir.x * dir.x + dir.y * dir.y);
    moving = len > 0.001f;
    ent->s.origin2 = (VECTOR2){ ent->s.origin.x, ent->s.origin.y };
    if (moving) {
        FLOAT step = WOW_WALK_SPEED * ((FLOAT)FRAMETIME / 1000.0f) / len;
        ent->s.origin.x += dir.x * step;
        ent->s.origin.y += dir.y * step;
    }
    ent->s.origin.z = Wow_TerrainHeight(ent->s.origin.x, ent->s.origin.y);
    ent->s.rotation = (VECTOR3){ wow_move.yaw, 0.0f, 0.0f };
    ent->s.frame = Wow_FrameForAnimation(ent, moving ? "Walk" : "Stand");
    Wow_UpdateCamera(ent);
}

static LPCSTR Wow_GetThemeValue(LPCSTR filename) {
    return filename ? filename : "";
}

static void Wow_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    (void)ent;
    if (argc >= 5 && !strcasecmp(argv[0], "wowmove")) {
        wow_move.flags = (DWORD)strtoul(argv[1], NULL, 10);
        wow_move.yaw = (FLOAT)atof(argv[2]);
        wow_move.pitch = Wow_Clamp((FLOAT)atof(argv[3]), WOW_CAMERA_MIN_PITCH, WOW_CAMERA_MAX_PITCH);
        wow_move.distance = Wow_Clamp((FLOAT)atof(argv[4]), WOW_CAMERA_MIN_DISTANCE, WOW_CAMERA_MAX_DISTANCE);
    }
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
