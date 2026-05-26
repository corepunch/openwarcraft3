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
static char wow_loading_texture[MAX_PATHLEN] = "Interface\\Glues\\LoadingScreens\\LoadScreenEnviroment.blp";
static char wow_loading_title[128] = "World of Warcraft";
static LPCANIMATION wow_player_animation;
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

static DWORD Wow_Read32(BYTE const *p) {
    return ((DWORD)p[0]) | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

static LPCSTR Wow_DbcString(BYTE const *string_block, DWORD string_size, DWORD offset) {
    if (offset >= string_size) {
        return NULL;
    }
    return (LPCSTR)(string_block + offset);
}

static BOOL Wow_ValidDbc(BYTE const *data,
                         DWORD size,
                         DWORD *records,
                         DWORD *fields,
                         DWORD *record_size,
                         DWORD *string_size) {
    if (!data || size <= 20 || memcmp(data, "WDBC", 4) != 0) {
        return false;
    }

    *records = Wow_Read32(data + 4);
    *fields = Wow_Read32(data + 8);
    *record_size = Wow_Read32(data + 12);
    *string_size = Wow_Read32(data + 16);

    if (*fields == 0 || *record_size < *fields * sizeof(DWORD) ||
        20 + *records * *record_size + *string_size > size) {
        return false;
    }
    return true;
}

static LPCSTR Wow_PathBasename(LPCSTR path) {
    LPCSTR slash = strrchr(path, '/');
    LPCSTR backslash = strrchr(path, '\\');

    if (slash && backslash) {
        return MAX(slash, backslash) + 1;
    }
    if (slash) {
        return slash + 1;
    }
    if (backslash) {
        return backslash + 1;
    }
    return path;
}

static void Wow_MapNameFromPath(LPCSTR path, LPSTR out, DWORD out_size) {
    LPCSTR base;
    size_t len;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    if (!path || !*path) {
        return;
    }

    base = Wow_PathBasename(path);
    len = strlen(base);
    if (len > 4 && !strcasecmp(base + len - 4, ".wdt")) {
        len -= 4;
    }
    if (len >= out_size) {
        len = out_size - 1;
    }
    memcpy(out, base, len);
    out[len] = '\0';
}

static BOOL Wow_ResolveLoadingScreenById(DWORD loading_screen_id, LPSTR out, DWORD out_size) {
    LPBYTE data;
    DWORD size = 0;
    DWORD records;
    DWORD fields;
    DWORD record_size;
    DWORD string_size;
    BYTE const *records_base;
    BYTE const *strings_base;

    if (!out || out_size == 0) {
        return false;
    }

    data = gi.ReadFile ? gi.ReadFile("DBFilesClient\\LoadingScreens.dbc", &size) : NULL;
    if (!Wow_ValidDbc(data, size, &records, &fields, &record_size, &string_size) ||
        fields < 3 || record_size < 3 * sizeof(DWORD)) {
        SAFE_DELETE(data, gi.MemFree);
        return false;
    }

    records_base = data + 20;
    strings_base = records_base + records * record_size;
    FOR_LOOP(record_index, records) {
        BYTE const *record = records_base + record_index * record_size;
        DWORD id = Wow_Read32(record);

        if (id == loading_screen_id) {
            DWORD path_offset = Wow_Read32(record + 2 * sizeof(DWORD));
            LPCSTR path = Wow_DbcString(strings_base, string_size, path_offset);

            if (path && *path) {
                snprintf(out, out_size, "%s", path);
                gi.MemFree(data);
                return true;
            }
            break;
        }
    }

    gi.MemFree(data);
    return false;
}

static void Wow_SelectLoadingScreen(LPCSTR map_path) {
    LPBYTE data;
    DWORD size = 0;
    DWORD records;
    DWORD fields;
    DWORD record_size;
    DWORD string_size;
    BYTE const *records_base;
    BYTE const *strings_base;
    char map_name[128] = { 0 };

    snprintf(wow_loading_texture,
             sizeof(wow_loading_texture),
             "%s",
             "Interface\\Glues\\LoadingScreens\\LoadScreenEnviroment.blp");
    snprintf(wow_loading_title, sizeof(wow_loading_title), "%s", "World of Warcraft");

    if (!map_path || !*map_path) {
        return;
    }

    Wow_MapNameFromPath(map_path, map_name, sizeof(map_name));
    if (!map_name[0]) {
        return;
    }

    data = gi.ReadFile ? gi.ReadFile("DBFilesClient\\Map.dbc", &size) : NULL;
    if (!Wow_ValidDbc(data, size, &records, &fields, &record_size, &string_size) ||
        fields < 4 || record_size < fields * sizeof(DWORD)) {
        SAFE_DELETE(data, gi.MemFree);
        return;
    }

    records_base = data + 20;
    strings_base = records_base + records * record_size;
    FOR_LOOP(record_index, records) {
        BYTE const *record = records_base + record_index * record_size;
        DWORD map_dir_offset = Wow_Read32(record + sizeof(DWORD));
        LPCSTR map_dir = Wow_DbcString(strings_base, string_size, map_dir_offset);

        if (!map_dir || strcasecmp(map_dir, map_name)) {
            continue;
        }

        DWORD map_title_offset = Wow_Read32(record + 3 * sizeof(DWORD));
        DWORD loading_screen_id = Wow_Read32(record + (fields - 1) * sizeof(DWORD));
        LPCSTR map_title = Wow_DbcString(strings_base, string_size, map_title_offset);

        if (map_title && *map_title) {
            snprintf(wow_loading_title, sizeof(wow_loading_title), "%s", map_title);
        } else {
            snprintf(wow_loading_title, sizeof(wow_loading_title), "%s", map_name);
        }

        if (!Wow_ResolveLoadingScreenById(loading_screen_id,
                                          wow_loading_texture,
                                          sizeof(wow_loading_texture))) {
            snprintf(wow_loading_texture,
                     sizeof(wow_loading_texture),
                     "%s",
                     "Interface\\Glues\\LoadingScreens\\LoadScreenEnviroment.blp");
        }

        if (gi.error) {
            gi.error("Wow_SelectLoadingScreen: map=%s title=%s loadingId=%u texture=%s\n",
                     map_name,
                     wow_loading_title,
                     (unsigned)loading_screen_id,
                     wow_loading_texture);
        }
        gi.MemFree(data);
        return;
    }

    gi.MemFree(data);
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

static LPCANIMATION Wow_SetPlayerAnimation(LPEDICT ent, LPCSTR animation_name) {
    LPCANIMATION anim;

    if (!ent || !gi.GetAnimation || !animation_name || ent->s.model == 0) {
        wow_player_animation = NULL;
        return NULL;
    }
    anim = gi.GetAnimation(ent->s.model, animation_name);
    if (!anim || anim->interval[1] <= anim->interval[0]) {
        wow_player_animation = NULL;
        return NULL;
    }
    if (wow_player_animation != anim) {
        ent->s.frame = anim->interval[0];
        wow_player_animation = anim;
    }
    return wow_player_animation;
}

static void Wow_MovePlayerFrame(LPEDICT ent) {
    DWORD next_frame;

    if (!ent || !wow_player_animation) {
        return;
    }
    next_frame = ent->s.frame + FRAMETIME;
    if (ent->s.frame < wow_player_animation->interval[0] ||
        ent->s.frame >= wow_player_animation->interval[1] ||
        next_frame >= wow_player_animation->interval[1]) {
        ent->s.frame = wow_player_animation->interval[0];
    } else {
        ent->s.frame = next_frame;
    }
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
    wow_player_animation = NULL;
    Wow_SetPlayerAnimation(ent, "Stand");

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
    ps->texts[PLAYERTEXT_MAP_TITLE] = wow_loading_title;
    ps->texts[PLAYERTEXT_MAP_PREVIEW] = wow_loading_texture;
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
    Wow_SelectLoadingScreen(mapinfo ? mapinfo->mapName : NULL);
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
    if (Wow_SetPlayerAnimation(ent, moving ? "Walk" : "Stand")) {
        Wow_MovePlayerFrame(ent);
    }
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
