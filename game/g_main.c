/*
 * g_main.c — Game library entry point and main simulation loop.
 *
 * This file implements the game_export interface consumed by the server
 * (sv_game.c).  GetGameAPI() is called once at startup and returns a
 * vtable of function pointers used by the server to drive the game.
 *
 * Key callbacks:
 *   Init        — allocates entity pool, loads config/unit data tables.
 *   SpawnEntities — loads a map and spawns all its entities.
 *   RunFrame    — called once per server frame; runs events, client camera
 *                 interpolation, entity physics/AI, and collision resolution.
 *   ClientBegin — called when a client finishes connecting; sends the initial
 *                 UI layout (svc_layout) and tallies food counts.
 *   ClientCommand — routes player commands to the skills system.
 *
 * G_RunFrame() is the inner loop:
 *   1. G_RunEvents()      — dispatch queued game events to triggers.
 *   2. G_RunClients()     — interpolate camera positions for smooth panning.
 *   3. G_RunEntities()    — call G_RunEntity() on every live entity.
 *   4. G_SolveCollisions() — resolve entity overlaps (g_phys.c).
 */
#include "g_local.h"
#include "g_unitdata.h"

struct game_export globals;
struct game_import gi;
struct game_locals game;
struct level_locals level;
struct edict_s *g_edicts;

LPCSTR miscdata_files[] = {
    "UI\\MiscData.txt",
    "Units\\MiscData.txt",
    "Units\\MiscGame.txt",
    "UI\\MiscUI.txt",
    "UI\\SoundInfo\\MiscData.txt",
    "war3mapMisc.txt",
    NULL
};

static void InitMiscValue(LPCSTR name, FLOAT *dest) {
    LPCSTR strvalue = gi.FindSheetCell(game.config.misc, "Misc", name);
    *dest = strvalue ? atof(strvalue) : 0;
}

static void InitConstants(void) {
    for (LPCSTR *config = miscdata_files; *config; config++) {
        sheetRow_t *current = gi.ReadConfig(*config);
        if (current) {
            PUSH_BACK(sheetRow_t, current, game.config.misc);
        }
    }
    InitMiscValue("AttackHalfAngle", &game.constants.attackHalfAngle);
    InitMiscValue("MaxCollisionRadius", &game.constants.maxCollisionRadius);
    InitMiscValue("DecayTime", &game.constants.decayTime);
    InitMiscValue("BoneDecayTime", &game.constants.boneDecayTime);
    InitMiscValue("DissipateTime", &game.constants.dissipateTime);
    InitMiscValue("StructureDecayTime", &game.constants.structureDecayTime);
    InitMiscValue("BulletDeathTime", &game.constants.bulletDeathTime);
    InitMiscValue("CloseEnoughRange", &game.constants.closeEnoughRange);
    InitMiscValue("Dawn", &game.constants.dawnTimeGameHours);
    InitMiscValue("Dusk", &game.constants.duskTimeGameHours);
    InitMiscValue("DayHours", &game.constants.gameDayHours);
    InitMiscValue("DayLength", &game.constants.gameDayLength);
    InitMiscValue("BuildingAngle", &game.constants.buildingAngle);
    InitMiscValue("RootAngle", &game.constants.rootAngle);
}

static void G_InitGame(void) {
    g_edicts = gi.MemAlloc(sizeof(edict_t) * MAX_ENTITIES);
    
    globals.edicts = g_edicts;
    globals.num_edicts = 0;
    globals.max_edicts = MAX_ENTITIES;
    globals.max_clients = MAX_CLIENTS;

    game.max_clients = globals.max_clients;
    game.clients = gi.MemAlloc(game.max_clients * sizeof(GAMECLIENT));
    game.config.theme = gi.ReadConfig("UI\\war3skins.txt");
    game.config.splats = gi.ReadSheet("Splats\\SplatData.slk");
    game.config.uberSplats = gi.ReadSheet("Splats\\UberSplatData.slk");
    game.config.abilities = gi.ReadSheet("Units\\AbilityData.slk");
    game.config.items = gi.ReadSheet("Units\\ItemData.slk");
    
    InitConstants();
    InitUnitData();
    InitAbilities();
}

static void G_ShutdownGame(void) {
    gi.MemFree(g_edicts);

    ShutdownUnitData();
}

FLOAT G_Cinefade(void) {
    DWORD duration = level.cinefilter.end.time - level.cinefilter.start.time;
    if (gi.GetTime() > level.cinefilter.end.time) {
        return level.cinefilter.end.color.a / 255.0;
    } else {
        FLOAT k = (gi.GetTime() - level.cinefilter.start.time) / (FLOAT)duration;
        return LerpNumber(level.cinefilter.start.color.a, level.cinefilter.end.color.a, k) / 255.0;
    }
}

static void G_RunClients(void) {
    FLOAT cinefade = G_Cinefade();
    FOR_LOOP(i, game.max_clients) {
        LPGAMECLIENT client = game.clients+i;
        DWORD duration = client->camera.end_time - client->camera.start_time;
        if (gi.GetTime() < client->camera.end_time && duration > 0) {
            FLOAT k = (gi.GetTime() - client->camera.start_time) / (FLOAT)duration;
            LPCCAMERASETUP a = &client->camera.old_state;
            LPCCAMERASETUP b = &client->camera.state;
            QUATERNION qa = Quaternion_fromEuler(&a->viewangles, ROTATE_ZYX);
            QUATERNION qb = Quaternion_fromEuler(&b->viewangles, ROTATE_ZYX);
            client->ps.origin = Vector2_lerp(&a->position, &b->position, k);
            client->ps.viewquat = Quaternion_slerp(&qa, &qb, k);
            client->ps.fov = LerpNumber(a->fov, b->fov, k) / FOV_ASPECT;
            client->ps.distance = LerpNumber(a->target_distance, b->target_distance, k);
        } else {
            client->ps.origin = client->camera.state.position;
            client->ps.viewquat = Quaternion_fromEuler(&client->camera.state.viewangles, ROTATE_ZYX);
            client->ps.fov = client->camera.state.fov / FOV_ASPECT;
            client->ps.distance = client->camera.state.target_distance;
        }
        client->ps.cinefade = cinefade;
    }
}

/* One complete server-frame simulation step.
 * Skipped until the first map has been started; on the very first frame after
 * a map loads, the JASS "main" function is invoked to run map initialization
 * triggers. */
static void G_RunFrame(void) {
    if (!level.started)
        return;

    if (!level.scriptsStarted) {
        jass_callbyname(level.vm, "main", true);
        level.scriptsStarted = true;
    }
    
    G_RunEvents();

    G_RunClients();

    G_RunEntities();

    G_SolveCollisions();
}

static LPCSTR G_GetThemeValue(LPCSTR filename) {
    LPCSTR skinned = NULL;
    if (!strstr(filename, "\\")) {
        skinned = gi.FindSheetCell(game.config.theme, "Default", filename);
    }
    return skinned ? skinned : filename;
}

LPEDICT G_GetPlayerEntityByNumber(DWORD number) {
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = g_edicts+i;
        if (ent->client && ent->client->ps.number == number) {
            return ent;
        }
    }
    return NULL;
}

LPGAMECLIENT G_GetPlayerClientByNumber(DWORD number) {
    FOR_LOOP(i, game.max_clients) {
        LPGAMECLIENT cl = game.clients+i;
        if (cl->ps.number == number) {
            return cl;
        }
    }
    return &game.clients[MAX_PLAYERS-1];
//    return NULL;
}

LPPLAYER G_GetPlayerByNumber(DWORD number) {
    FOR_LOOP(i, game.max_clients) {
        if (game.clients[i].ps.number == number) {
            return &game.clients[i].ps;
        }
    }
    return &game.clients[MAX_PLAYERS-1].ps;
//    return NULL;
}

GAMEEVENT *G_PublishEvent(LPEDICT edict, EVENTTYPE type) {
    GAMEEVENT *evt = &level.events.queue[level.events.write++ % MAX_EVENT_QUEUE];
    evt->type = type;
    evt->edict = edict;
    return evt;
}

LPCSTR G_LevelString(LPCSTR name) {
    DWORD string_id = 0;
    sscanf(name, "TRIGSTR_%d", &string_id);
    FOR_EACH_LIST(mapTrigStr_t, trigstr, level.mapinfo->strings) {
        if (trigstr->id == string_id) {
            return trigstr->text;
        }
    }
    return name;
}

/* Called when a client finishes the connection handshake and is ready to play.
 * Serializes the top-level ConsoleUI and CinematicPanel frame trees and sends
 * them to the client as svc_layout messages so the client can render the HUD.
 * Also counts the player's initial food supply from pre-placed buildings. */
static void G_ClientBegin(LPEDICT edict) {
    UI_FRAME(ConsoleUI);
    UI_FRAME(CinematicPanel);

    UI_WriteLayout(edict, ConsoleUI, LAYER_CONSOLE);
    UI_WriteLayout(edict, CinematicPanel, LAYER_CINEMATIC);
    
    FILTER_EDICTS(ent, edict->client->ps.number == ent->s.player) {
        edict->client->ps.stats[PLAYERSTATE_RESOURCE_FOOD_CAP] += UNIT_FOOD_MADE(ent->class_id);
        edict->client->ps.stats[PLAYERSTATE_RESOURCE_FOOD_USED] += UNIT_FOOD_USED(ent->class_id);
    }
    
    level.started = true;
}

/* Return the game API vtable to the server.
 * Called once at startup; after this point the server drives the game
 * exclusively through the returned function pointers. */
struct game_export *GetGameAPI(struct game_import *import) {
    gi = *import;
    globals.Init = G_InitGame;
    globals.Shutdown = G_ShutdownGame;
    globals.SpawnEntities = G_SpawnEntities;
    globals.RunFrame = G_RunFrame;
    globals.ClientCommand = G_ClientCommand;
    globals.ClientPanCamera = G_ClientPanCamera;
    globals.ClientBegin = G_ClientBegin;
    globals.GetThemeValue = G_GetThemeValue;
    globals.edict_size = sizeof(struct edict_s);
    return &globals;
}
