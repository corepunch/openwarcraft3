/*
 * test_commands.c — Quake-style command and map resolver coverage.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "test_framework.h"

static PATHSTR last_loading_map;
static PATHSTR last_sv_map;
static char last_forwarded[1024];
static bool command_tests_initialized;
static bool late_command_called;

void Key_Init(void) {
}

void Key_WriteBindings(FILE *file) {
    (void)file;
}

void Cmd_ForwardToServer(LPCSTR text) {
    snprintf(last_forwarded, sizeof(last_forwarded), "%s", text ? text : "");
}

void CL_SetGameplayBindings(void) {
}

void CL_BeginLoadingMap(LPCSTR mapName) {
    snprintf(last_loading_map, sizeof(last_loading_map), "%s", mapName ? mapName : "");
}

void CL_Shutdown(void) {
}

void SV_Map(LPCSTR pFilename) {
    snprintf(last_sv_map, sizeof(last_sv_map), "%s", pFilename ? pFilename : "");
}

void SV_Shutdown(void) {
}

void Sys_Quit(void) {
}

void PF_Sleep(DWORD msec) {
    (void)msec;
}

static void reset_map_handoff(void) {
    last_loading_map[0] = '\0';
    last_sv_map[0] = '\0';
    last_forwarded[0] = '\0';
    late_command_called = false;
}

static void Test_LateCommand_f(void) {
    late_command_called = true;
}

static void setup_command_tests(void) {
    if (command_tests_initialized) {
        return;
    }

    LPCSTR argv[] = { "test_commands", "-config", "" };

    Com_Init(3, argv);
    ASSERT(FS_AddArchive("build/tests/tests.mpq") != NULL);
    reset_map_handoff();
    command_tests_initialized = true;
}

static void test_command_registration(void) {
    setup_command_tests();

    ASSERT(Cmd_Exists("cmdlist"));
    ASSERT(Cmd_Exists("map"));
    ASSERT(Cmd_Exists("maps"));
    ASSERT(Cmd_Exists("dir"));
    ASSERT(Cmd_Exists("path"));
}

static void test_command_and_cvar_completion(void) {
    char out[128];

    setup_command_tests();

    ASSERT_EQ_INT(Cmd_CompleteCommand("cmdli", out, sizeof(out), false), 1);
    ASSERT_STR_EQ(out, "cmdlist");
    ASSERT_EQ_INT(Cmd_CompleteCommand("ma", out, sizeof(out), false), 2);
    ASSERT_STR_EQ(out, "map");
    ASSERT_EQ_INT(Cvar_CompleteVariable("scr_show", out, sizeof(out), false), 1);
    ASSERT_STR_EQ(out, "scr_showfps");
    ASSERT(Cvar_String("scr_showfps", NULL) != NULL);
}

static void test_data_command_line_sets_data_cvar(void) {
    LPCSTR argv[] = { "test_commands", "-data", "tests/data dir" };

    setup_command_tests();
    Cvar_ApplyCommandLine(3, argv);

    ASSERT_STR_EQ(Cvar_String("data", NULL), "tests/data dir");
}

static void test_tft_command_line_enables_expansion_archives(void) {
    LPCSTR argv[] = { "test_commands", "-tft" };

    setup_command_tests();
    Cvar_Set("fs_expansion", "0");
    Cvar_ApplyCommandLine(2, argv);

    ASSERT_STR_EQ(Cvar_String("fs_expansion", NULL), "1");
}

static void test_roc_command_line_disables_expansion_archives(void) {
    LPCSTR argv[] = { "test_commands", "-roc" };

    setup_command_tests();
    Cvar_Set("fs_expansion", "1");
    Cvar_ApplyCommandLine(2, argv);

    ASSERT_STR_EQ(Cvar_String("fs_expansion", NULL), "0");
}

static void test_dash_cvars_are_not_command_line_cvars(void) {
    LPCSTR argv[] = { "test_commands", "-r_module=stdout" };

    setup_command_tests();
    Cvar_Set("r_module", "renderer");
    Cvar_ApplyCommandLine(2, argv);

    ASSERT_STR_EQ(Cvar_String("r_module", NULL), "renderer");
}

static void test_plus_cvars_apply_immediately(void) {
    LPCSTR argv[] = { "test_commands", "+game_port", "28010", "+r_module", "stdout" };

    setup_command_tests();
    Cvar_Set("game_port", PORT_SERVER_STRING);
    Cvar_Set("r_module", "renderer");
    COM_InitArgv(5, argv);
    Cbuf_AddEarlyCommands(true);

    ASSERT_STR_EQ(Cvar_String("game_port", NULL), "28010");
    ASSERT_STR_EQ(Cvar_String("r_module", NULL), "stdout");
}

static void test_plus_map_is_early_launch_selector(void) {
    LPCSTR argv[] = { "test_commands", "+map", "Human02" };

    setup_command_tests();
    Cvar_Set("map", "");
    reset_map_handoff();
    COM_InitArgv(3, argv);
    Cbuf_AddEarlyCommands(true);
    Cbuf_AddLateCommands();
    Cbuf_Execute();

    ASSERT_STR_EQ(Cvar_String("map", NULL), "Human02");
    ASSERT_STR_EQ(last_loading_map, "");
    ASSERT_STR_EQ(last_sv_map, "");
}

static void test_remaining_plus_commands_run_late(void) {
    LPCSTR argv[] = { "test_commands", "+test_late_command" };

    setup_command_tests();
    if (!Cmd_Exists("test_late_command")) {
        Cmd_AddCommand("test_late_command", Test_LateCommand_f);
    }
    late_command_called = false;
    COM_InitArgv(2, argv);
    Cbuf_AddEarlyCommands(true);
    Cbuf_AddLateCommands();
    Cbuf_Execute();

    ASSERT(late_command_called);
}

typedef struct {
    DWORD count;
    bool human02;
    bool orc01;
    bool twin_w3m;
    bool twin_w3x;
} mapListState_t;

static void count_fixture_map(LPCSTR path, void *userData) {
    mapListState_t *state = userData;

    state->count++;
    if (!strcmp(path, "Maps\\Campaign\\Human02.w3m")) {
        state->human02 = true;
    } else if (!strcmp(path, "Maps\\Campaign\\Orc01.w3m")) {
        state->orc01 = true;
    } else if (!strcmp(path, "Maps\\Melee\\TwinRivers.w3m")) {
        state->twin_w3m = true;
    } else if (!strcmp(path, "Maps\\FrozenThrone\\TwinRivers.w3x")) {
        state->twin_w3x = true;
    }
}

static void test_fixture_maps_are_listed_from_mpq(void) {
    mapListState_t state = { 0 };

    setup_command_tests();

    ASSERT_EQ_INT(FS_ListMaps(count_fixture_map, &state), 4);
    ASSERT_EQ_INT(state.count, 4);
    ASSERT(state.human02);
    ASSERT(state.orc01);
    ASSERT(state.twin_w3m);
    ASSERT(state.twin_w3x);
}

static void test_short_map_name_resolves_from_fixture_mpq(void) {
    PATHSTR path;

    setup_command_tests();

    ASSERT_EQ_INT(FS_ResolveMapPath("Human02", path, sizeof(path)), FS_MAP_RESOLVE_OK);
    ASSERT_STR_EQ(path, "Maps\\Campaign\\Human02.w3m");
    ASSERT_EQ_INT(FS_ResolveMapPath("orc01", path, sizeof(path)), FS_MAP_RESOLVE_OK);
    ASSERT_STR_EQ(path, "Maps\\Campaign\\Orc01.w3m");
}

static void test_explicit_map_path_still_resolves(void) {
    PATHSTR path;

    setup_command_tests();

    ASSERT_EQ_INT(FS_ResolveMapPath("Maps/Campaign/Human02.w3m", path, sizeof(path)), FS_MAP_RESOLVE_OK);
    ASSERT_STR_EQ(path, "Maps\\Campaign\\Human02.w3m");
}

static void test_ambiguous_short_map_name_is_rejected(void) {
    PATHSTR path;

    setup_command_tests();

    ASSERT_EQ_INT(FS_ResolveMapPath("TwinRivers", path, sizeof(path)), FS_MAP_RESOLVE_AMBIGUOUS);
}

static void test_map_command_uses_resolver(void) {
    setup_command_tests();
    reset_map_handoff();

    Cmd_ExecuteString("map Human02");

    ASSERT_STR_EQ(last_loading_map, "Maps\\Campaign\\Human02.w3m");
    ASSERT_STR_EQ(last_sv_map, "Maps\\Campaign\\Human02.w3m");
}

static void test_map_command_rejects_ambiguous_short_name(void) {
    setup_command_tests();
    reset_map_handoff();

    Cmd_ExecuteString("map TwinRivers");

    ASSERT_STR_EQ(last_loading_map, "");
    ASSERT_STR_EQ(last_sv_map, "");
}

void run_command_tests(void) {
    RUN_TEST(test_command_registration);
    RUN_TEST(test_command_and_cvar_completion);
    RUN_TEST(test_data_command_line_sets_data_cvar);
    RUN_TEST(test_tft_command_line_enables_expansion_archives);
    RUN_TEST(test_roc_command_line_disables_expansion_archives);
    RUN_TEST(test_dash_cvars_are_not_command_line_cvars);
    RUN_TEST(test_plus_cvars_apply_immediately);
    RUN_TEST(test_plus_map_is_early_launch_selector);
    RUN_TEST(test_remaining_plus_commands_run_late);
    RUN_TEST(test_fixture_maps_are_listed_from_mpq);
    RUN_TEST(test_short_map_name_resolves_from_fixture_mpq);
    RUN_TEST(test_explicit_map_path_still_resolves);
    RUN_TEST(test_ambiguous_short_map_name_is_rejected);
    RUN_TEST(test_map_command_uses_resolver);
    RUN_TEST(test_map_command_rejects_ambiguous_short_name);
}
