/*
 * test_sc2_map.c - StarCraft II map fixture coverage.
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "games/starcraft-2/common/sc2_map.h"
#include "test_framework.h"

#ifndef TEST_SC2_MPQ
#define TEST_SC2_MPQ "build/tests/test-sc2.SC2Maps"
#endif

static BOOL sc2_tests_initialized;
static DWORD listed_count;
static PATHSTR listed_map;

void Key_Init(void) {
}

void Key_WriteBindings(FILE *file) {
    (void)file;
}

void Cmd_ForwardToServer(LPCSTR text) {
    (void)text;
}

void CL_SetGameplayBindings(void) {
}

void CL_BeginLoadingMap(LPCSTR mapName) {
    (void)mapName;
}

void CL_Shutdown(void) {
}

void SV_Map(LPCSTR pFilename) {
    (void)pFilename;
}

void SV_Shutdown(void) {
}

void Sys_Quit(void) {
}

void PF_Sleep(DWORD msec) {
    (void)msec;
}

static void setup_sc2_tests(void) {
    if (sc2_tests_initialized) {
        return;
    }

    LPCSTR argv[] = { "test_sc2", "-config", "" };
    Com_Init(3, argv);
    ASSERT(FS_AddArchive(TEST_SC2_MPQ) != NULL);
    SC2_MapSetHost(&(sc2MapHost_t){
        .read_file = FS_ReadFile,
        .free_file = FS_FreeFile,
        .mem_alloc = MemAlloc,
        .mem_free = MemFree,
    });
    sc2_tests_initialized = true;
}

static void collect_map(LPCSTR path, void *userData) {
    (void)userData;
    listed_count++;
    if (!listed_map[0]) {
        snprintf(listed_map, sizeof(listed_map), "%s", path ? path : "");
    }
}

static void test_sc2_fixture_archive_lists_map_root(void) {
    setup_sc2_tests();
    listed_count = 0;
    listed_map[0] = '\0';

    ASSERT_EQ_INT(FS_ListMaps(collect_map, NULL), 1);
    ASSERT_EQ_INT(listed_count, 1);
    ASSERT_STR_EQ(listed_map, "Maps\\Test\\Tiny.SC2Map");
}

static void test_sc2_fixture_short_name_resolves(void) {
    PATHSTR path;

    setup_sc2_tests();
    ASSERT_EQ_INT(FS_ResolveMapPath("Tiny", path, sizeof(path)), FS_MAP_RESOLVE_OK);
    ASSERT_STR_EQ(path, "Maps\\Test\\Tiny.SC2Map");
}

static void test_sc2_map_loads_xml_objects_and_terrain(void) {
    sc2Map_t *map;

    setup_sc2_tests();
    ASSERT(SC2_MapLoad("Maps\\Test\\Tiny.SC2Map"));
    map = SC2_MapCurrent();

    ASSERT_STR_EQ(map->map_name, "SC2 Tiny Fixture");
    ASSERT_EQ_INT(map->width, 4);
    ASSERT_EQ_INT(map->height, 3);
    ASSERT_EQ_INT(map->num_objects, 3);
    ASSERT_EQ_INT(map->generated, false);
    ASSERT_EQ_INT(map->has_camera, true);
    ASSERT_EQ_FLOAT(map->camera_target.x, 8.0f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_target.y, 9.0f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_distance, 34.0f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_pitch, 56.0f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_yaw, 179.9584f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_fov, 27.7998f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_znear, 0.0998f, 0.001f);
    ASSERT_EQ_FLOAT(map->camera_zfar, 400.0f, 0.001f);

    ASSERT_STR_EQ(map->objects[0].name, "Marine");
    ASSERT_STR_EQ(map->objects[0].model, "Assets\\Units\\Terran\\Marine\\Marine.m3");
    ASSERT_EQ_INT(map->objects[0].type, SC2_OBJECT_UNIT);
    ASSERT_EQ_FLOAT(map->objects[0].position.x, 1.5f, 0.001f);
    ASSERT_EQ_FLOAT(map->objects[0].position.y, 2.5f, 0.001f);
    ASSERT_EQ_FLOAT(map->objects[0].position.z, 0.25f, 0.001f);
    ASSERT_EQ_FLOAT(map->objects[0].angle, 0.75f, 0.001f);
    ASSERT_EQ_INT(map->objects[0].player, 2);

    ASSERT_STR_EQ(map->objects[2].name, "BillboardTall");
    ASSERT_EQ_INT(map->objects[2].type, SC2_OBJECT_DOODAD);
    ASSERT_STR_EQ(map->objects[2].model, "");

    ASSERT_EQ_INT(map->num_terrain_textures, 2);
    ASSERT_STR_EQ(map->terrain_textures[0].diffuse, "Assets\\Textures\\Terrain\\FixtureGrass_Diffuse.dds");
    ASSERT_STR_EQ(map->terrain_textures[0].normal, "Assets\\Textures\\Terrain\\FixtureGrass_Diffuse_normal.dds");
    ASSERT_STR_EQ(map->terrain_textures[1].diffuse, "Assets\\Textures\\Terrain\\FixtureDirt_Diffuse.dds");
}

static void test_sc2_map_loads_binary_terrain_layers(void) {
    sc2Map_t *map;

    setup_sc2_tests();
    ASSERT(SC2_MapLoad("Maps\\Test\\Tiny.SC2Map"));
    map = SC2_MapCurrent();

    ASSERT_EQ_INT(map->cell_flags_width, 4);
    ASSERT_EQ_INT(map->cell_flags_height, 3);
    ASSERT_NOT_NULL(map->cell_flags);
    if (map->cell_flags) {
        ASSERT_EQ_INT(map->cell_flags[0], 0x10);
        ASSERT_EQ_INT(map->cell_flags[11], 0x1b);
    }

    ASSERT_EQ_INT(map->cliff_level_width, 4);
    ASSERT_EQ_INT(map->cliff_level_height, 3);
    ASSERT_NOT_NULL(map->cliff_levels);
    if (map->cliff_levels) {
        ASSERT_EQ_INT(map->cliff_levels[0], 1);
        ASSERT_EQ_INT(map->cliff_levels[11], 12);
    }

    ASSERT_EQ_INT(map->height_map_width, 5);
    ASSERT_EQ_INT(map->height_map_height, 4);
    ASSERT_NOT_NULL(map->height_map);
    if (map->height_map) {
        ASSERT_EQ_FLOAT(map->height_map[0], 3.0f, 0.001f);
        ASSERT_EQ_FLOAT(map->height_map[19], 12.5f, 0.001f);
        ASSERT_EQ_FLOAT(SC2_MapHeightAtPoint(0.0f, 0.0f), 3.0f, 0.001f);
        ASSERT_EQ_FLOAT(SC2_MapHeightAtPoint(4.0f, 3.0f), 12.5f, 0.001f);
    }

    ASSERT_EQ_INT(map->texture_mask_width, 4);
    ASSERT_EQ_INT(map->texture_mask_height, 4);
    ASSERT_EQ_INT(map->num_texture_masks, 2);
    ASSERT_NOT_NULL(map->texture_masks[0]);
    ASSERT_NOT_NULL(map->texture_masks[1]);
    if (map->texture_masks[0]) {
        ASSERT_EQ_INT(map->texture_masks[0][0], 1);
        ASSERT_EQ_INT(map->texture_masks[0][1], 2);
    }
    if (map->texture_masks[1]) {
        ASSERT_EQ_INT(map->texture_masks[1][0], 0x0a);
        ASSERT_EQ_INT(map->texture_masks[1][1], 0x0b);
    }
}

void run_sc2_map_tests(void) {
    RUN_TEST(test_sc2_fixture_archive_lists_map_root);
    RUN_TEST(test_sc2_fixture_short_name_resolves);
    RUN_TEST(test_sc2_map_loads_xml_objects_and_terrain);
    RUN_TEST(test_sc2_map_loads_binary_terrain_layers);
    SC2_MapShutdown();
    FS_Shutdown();
}
