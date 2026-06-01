#include "g_wow_local.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

struct game_import gi;
struct game_export globals;
edict_t wow_edicts[WOW_MAX_EDICTS];
wowEntityLocal_t wow_entity_locals[WOW_MAX_EDICTS];
static struct client_s wow_clients[WOW_MAX_CLIENTS];
static VECTOR2 wow_spawn_origin = { 0.0f, 0.0f };
static char wow_loading_texture[MAX_PATHLEN] = "Interface\\Glues\\LoadingScreens\\LoadScreenEnviroment.blp";
static char wow_loading_title[128] = "World of Warcraft";
static struct {
    DWORD flags;
    FLOAT yaw;
    FLOAT pitch;
    FLOAT distance;
} wow_move = {
    .pitch = 328.0f,
    .distance = 8.5f,
};

#define WOW_MISSING_ANIMATION_LOG_SLOTS 128

typedef struct {
    DWORD model;
    char name[64];
} wowMissingAnimationLog_t;

static wowMissingAnimationLog_t wow_missing_animation_log[WOW_MISSING_ANIMATION_LOG_SLOTS];

static void Wow_LogMissingAnimation(LPEDICT ent, LPCSTR animation_name, BOOL invalid_interval) {
    DWORD model;

    if (!ent || !animation_name || !*animation_name) {
        return;
    }
    model = ent->s.model;
    FOR_LOOP(i, WOW_MISSING_ANIMATION_LOG_SLOTS) {
        wowMissingAnimationLog_t *entry = &wow_missing_animation_log[i];

        if (entry->model == model && !strcasecmp(entry->name, animation_name)) {
            return;
        }
        if (entry->model == 0) {
            entry->model = model;
            strncpy(entry->name, animation_name, sizeof(entry->name) - 1);
            fprintf(stderr,
                    "WoW missing animation: entity=%u model=%u animation=%s%s\n",
                    (unsigned)ent->s.number,
                    (unsigned)model,
                    animation_name,
                    invalid_interval ? " invalid-interval" : "");
            return;
        }
    }

    fprintf(stderr,
            "WoW missing animation: entity=%u model=%u animation=%s%s\n",
            (unsigned)ent->s.number,
            (unsigned)model,
            animation_name,
            invalid_interval ? " invalid-interval" : "");
}

FLOAT Wow_Clamp(FLOAT value, FLOAT min_value, FLOAT max_value) {
    return MAX(min_value, MIN(value, max_value));
}

DWORD Wow_Read32(BYTE const *p) {
    return ((DWORD)p[0]) | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}

FLOAT Wow_ReadFloat(BYTE const *p) {
    FLOAT value;
    memcpy(&value, p, sizeof(value));
    return value;
}

LPCSTR Wow_DbcString(BYTE const *string_block, DWORD string_size, DWORD offset) {
    if (offset >= string_size) {
        return NULL;
    }
    return (LPCSTR)(string_block + offset);
}

BOOL Wow_ValidDbc(BYTE const *data,
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

BOOL Wow_FindDbcRecord(LPCSTR filename,
                       DWORD wanted_id,
                       LPBYTE *data_out,
                       DWORD *fields_out,
                       DWORD *record_size_out,
                       BYTE const **record_out,
                       BYTE const **strings_out,
                       DWORD *string_size_out) {
    LPBYTE data;
    DWORD size = 0;
    DWORD records;
    DWORD fields;
    DWORD record_size;
    DWORD string_size;
    BYTE const *records_base;

    if (!filename || !data_out || !fields_out || !record_size_out ||
        !record_out || !strings_out || !string_size_out) {
        return false;
    }

    data = gi.ReadFile ? gi.ReadFile(filename, &size) : NULL;
    if (!Wow_ValidDbc(data, size, &records, &fields, &record_size, &string_size) ||
        fields < 1 || record_size < fields * sizeof(DWORD)) {
        SAFE_DELETE(data, gi.MemFree);
        return false;
    }

    records_base = data + 20;
    FOR_LOOP(record_index, records) {
        BYTE const *record = records_base + record_index * record_size;
        DWORD id = Wow_Read32(record);

        if (id == wanted_id) {
            *data_out = data;
            *fields_out = fields;
            *record_size_out = record_size;
            *record_out = record;
            *strings_out = records_base + records * record_size;
            *string_size_out = string_size;
            return true;
        }
    }

    gi.MemFree(data);
    return false;
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

FLOAT Wow_TerrainHeight(FLOAT x, FLOAT y) {
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

DWORD Wow_EntityIndex(LPCEDICT ent) {
    if (!ent || ent < wow_edicts || ent >= wow_edicts + WOW_MAX_EDICTS) {
        return WOW_MAX_EDICTS;
    }
    return (DWORD)(ent - wow_edicts);
}

wowEntityLocal_t *Wow_EntityLocal(LPCEDICT ent) {
    DWORD index = Wow_EntityIndex(ent);

    if (index >= WOW_MAX_EDICTS) {
        return NULL;
    }
    return &wow_entity_locals[index];
}

LPCANIMATION Wow_SetEntityAnimation(LPEDICT ent, LPCSTR animation_name) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    LPCANIMATION anim;

    if (!ent || !local || !gi.GetAnimation || !animation_name || ent->s.model == 0) {
        if (local) {
            local->animation = NULL;
        }
        return NULL;
    }
    anim = gi.GetAnimation(ent->s.model, animation_name);
    if (!anim || anim->interval[1] <= anim->interval[0]) {
        Wow_LogMissingAnimation(ent, animation_name, anim != NULL);
        local->animation = NULL;
        return NULL;
    }
    if (local->animation != anim) {
        ent->s.frame = anim->interval[0];
        local->animation = anim;
    }
    return local->animation;
}

BOOL Wow_SetEntityMoveFirstAnimation(LPEDICT ent, LPWOWMOVE move, LPCSTR const *animation_names) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);

    if (!ent || !local || !move) {
        return false;
    }
    if (local->currentmove == move && local->animation) {
        return true;
    }
    for (LPCSTR const *name = animation_names; name && *name; name++) {
        if (Wow_SetEntityAnimation(ent, *name)) {
            local->currentmove = move;
            return true;
        }
    }
    local->currentmove = NULL;
    return false;
}

BOOL Wow_SetEntityMove(LPEDICT ent, LPWOWMOVE move) {
    LPCSTR names[2];

    if (!move || !move->animation) {
        return false;
    }
    names[0] = move->animation;
    names[1] = NULL;
    return Wow_SetEntityMoveFirstAnimation(ent, move, names);
}

void Wow_AdvanceEntityFrame(LPEDICT ent) {
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    DWORD next_frame;

    if (!ent || !local || !local->animation) {
        return;
    }
    next_frame = ent->s.frame + FRAMETIME;
    if (ent->s.frame < local->animation->interval[0] ||
        ent->s.frame >= local->animation->interval[1] ||
        next_frame >= local->animation->interval[1]) {
        ent->s.frame = local->animation->interval[0];
    } else {
        ent->s.frame = next_frame;
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

static void Wow_MovePlayerFrame(LPEDICT ent) {
    Wow_AdvanceEntityFrame(ent);
}

static LPEDICT Wow_EdictByNumber(DWORD number) {
    if (number >= (DWORD)globals.num_edicts || number >= WOW_MAX_EDICTS) {
        return NULL;
    }
    if (!wow_edicts[number].inuse) {
        return NULL;
    }
    return &wow_edicts[number];
}

static LPEDICT Wow_FindNearestAttackTarget(LPEDICT ent) {
    LPEDICT best = NULL;
    FLOAT best_dist2 = 6.0f * 6.0f;

    if (!ent) {
        return NULL;
    }

    for (DWORD i = WOW_MAX_CLIENTS; i < (DWORD)globals.num_edicts && i < WOW_MAX_EDICTS; i++) {
        LPEDICT candidate = &wow_edicts[i];
        VECTOR2 delta;
        FLOAT dist2;

        if (!candidate->inuse || candidate == ent || !(candidate->svflags & SVF_MONSTER)) {
            continue;
        }

        delta = Vector2_sub(&candidate->s.origin2, &ent->s.origin2);
        dist2 = delta.x * delta.x + delta.y * delta.y;
        if (dist2 < best_dist2) {
            best_dist2 = dist2;
            best = candidate;
        }
    }

    return best;
}

LPEDICT Wow_Spawn(void) {
    LPEDICT ent = NULL;
    DWORD index;

    if (globals.num_edicts < globals.max_edicts) {
        index = globals.num_edicts++;
        ent = &wow_edicts[index];
    } else {
        for (index = WOW_MAX_CLIENTS; index < WOW_MAX_EDICTS; index++) {
            if (!wow_edicts[index].inuse) {
                ent = &wow_edicts[index];
                break;
            }
        }
    }
    if (!ent) {
        return NULL;
    }

    memset(ent, 0, sizeof(*ent));
    memset(&wow_entity_locals[index], 0, sizeof(wow_entity_locals[index]));
    ent->inuse = true;
    ent->s.number = index;
    return ent;
}

static void Wow_InitPlayer(LPEDICT ent) {
    LPPLAYER ps;
    wowEntityLocal_t *local = Wow_EntityLocal(ent);
    FLOAT height = Wow_TerrainHeight(wow_spawn_origin.x, wow_spawn_origin.y);

    memset(ent, 0, sizeof(*ent));
    if (local) {
        memset(local, 0, sizeof(*local));
        local->kind = WOW_ENTITY_PLAYER;
        local->home = wow_spawn_origin;
        local->yaw = wow_move.yaw;
        local->health = 100;
        local->attack_damage_point = 250;
        local->attack_backswing = 450;
    }
    ent->client = &wow_clients[0];
    ent->inuse = true;
    ent->s.number = 0;
    ent->s.model = gi.ModelIndex ? gi.ModelIndex(WOW_PLAYER_MODEL) : 0;
    ent->s.model2 = gi.ModelIndex ? gi.ModelIndex(WOW_PLAYER_WEAPON_MODEL) : 0;
    ent->s.appearance = Wow_PackAppearance(0, 0, 0, 0, 0, WOW_CLASS_WARRIOR, 0);
    ent->s.equipment = Wow_PackEquipment(0, 0, 0, 0);
    ent->s.origin = (VECTOR3){ wow_spawn_origin.x, wow_spawn_origin.y, height };
    ent->s.origin2 = (VECTOR2){ ent->s.origin.x, ent->s.origin.y };
    ent->s.rotation = (VECTOR3){ wow_move.yaw, 0.0f, 0.0f };
    ent->s.scale = 1.0f;
    ent->s.radius = 1.0f;
    ent->s.flags = EF_GROUND_ANCHOR;
    ent->idle = Wow_AIIdle;
    ent->move = NULL;
    ent->run = NULL;
    ent->attack = Wow_AIAttack;
    ent->pain = Wow_AIPain;
    Wow_SetStandMove(ent);

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
    memset(wow_entity_locals, 0, sizeof(wow_entity_locals));
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
    Wow_SpawnAmbientCreatures(&wow_spawn_origin);
    fprintf(stderr, "WoW doodads: static ADT doodads are renderer-owned and not synced as entities\n");
}

static void Wow_RunFrame(void) {
    LPEDICT ent = &wow_edicts[0];
    VECTOR2 forward;
    VECTOR2 right;
    VECTOR2 dir = { 0.0f, 0.0f };
    FLOAT len;
    BOOL moving;
    BOOL locked;

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
    locked = Wow_AIAdvanceLockedFrame(ent);
    if (locked) {
        Wow_UpdateCamera(ent);
    } else if (moving
        ? Wow_SetRunMove(ent)
        : (Wow_EntityAffectingCombat(ent)
            ? Wow_SetCombatReadyAnimation(ent)
            : Wow_SetStandMove(ent))) {
        ent->s.rotation = (VECTOR3){ wow_move.yaw, 0.0f, 0.0f };
        Wow_MovePlayerFrame(ent);
        Wow_UpdateCamera(ent);
    } else {
        ent->s.rotation = (VECTOR3){ wow_move.yaw, 0.0f, 0.0f };
        Wow_UpdateCamera(ent);
    }

    for (DWORD i = WOW_MAX_CLIENTS; i < (DWORD)globals.num_edicts; i++) {
        if (wow_edicts[i].inuse) {
            Wow_RunCreatureFrame(&wow_edicts[i]);
        }
    }
}

static LPCSTR Wow_GetThemeValue(LPCSTR filename) {
    return filename ? filename : "";
}

static void Wow_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    if (argc >= 5 && (!strcasecmp(argv[0], "move") || !strcasecmp(argv[0], "wowmove"))) {
        wow_move.flags = (DWORD)strtoul(argv[1], NULL, 10);
        wow_move.yaw = (FLOAT)atof(argv[2]);
        wow_move.pitch = Wow_Clamp((FLOAT)atof(argv[3]), WOW_CAMERA_MIN_PITCH, WOW_CAMERA_MAX_PITCH);
        wow_move.distance = Wow_Clamp((FLOAT)atof(argv[4]), WOW_CAMERA_MIN_DISTANCE, WOW_CAMERA_MAX_DISTANCE);
    } else if (argc >= 1 && (!strcasecmp(argv[0], "attack") || !strcasecmp(argv[0], "wowattack"))) {
        LPEDICT target = argc >= 2
            ? Wow_EdictByNumber((DWORD)strtoul(argv[1], NULL, 10))
            : Wow_FindNearestAttackTarget(ent);
        wowEntityLocal_t *local = Wow_EntityLocal(ent);

        if (!ent || !local || local->dead || !ent->attack) {
            return;
        }
        local->enemy = target && target != ent ? target : NULL;
        ent->attack(ent);
    } else if (argc >= 1 && (!strcasecmp(argv[0], "stopattack") || !strcasecmp(argv[0], "wowstopattack"))) {
        wowEntityLocal_t *local = Wow_EntityLocal(ent);

        if (local) {
            local->enemy = NULL;
            local->attack_time = 0;
            local->attack_damage_time = 0;
            local->attack_backswing_time = 0;
            local->attack_damage_done = false;
            local->pain_time = 0;
        }
        if (!local || !local->dead) {
            Wow_SetStandMove(ent);
        }
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
