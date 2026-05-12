#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "../tools/tool_common.h"

static void test_normalize_slashes_to_backslashes(void) {
    char path[64] = "UI/FrameDef\\Glue/MainMenu.fdf";

    Tool_NormalizeSlashes(path, '\\');

    ASSERT_STR_EQ(path, "UI\\FrameDef\\Glue\\MainMenu.fdf");
}

static void test_trim_edge_slashes_removes_both_sides(void) {
    char path[64] = "\\UI/FrameDef/Glue/MainMenu.fdf/";

    Tool_TrimEdgeSlashes(path);

    ASSERT_STR_EQ(path, "UI/FrameDef/Glue/MainMenu.fdf");
}

static void test_path_join_uses_forward_slash(void) {
    char *joined = Tool_PathJoin("UI/FrameDef", "Glue/MainMenu.fdf");

    ASSERT_NOT_NULL(joined);
    if (joined) {
        ASSERT_STR_EQ(joined, "UI/FrameDef/Glue/MainMenu.fdf");
        free(joined);
    }
}

static void test_path_parent_and_basename_follow_archive_paths(void) {
    char *parent = Tool_PathParent("UI\\FrameDef\\Glue\\MainMenu.fdf");

    ASSERT_NOT_NULL(parent);
    if (parent) {
        ASSERT_STR_EQ(parent, "UI/FrameDef/Glue");
        free(parent);
    }

    ASSERT_STR_EQ(Tool_PathBasename("UI\\FrameDef\\Glue\\MainMenu.fdf"), "MainMenu.fdf");
    ASSERT_STR_EQ(Tool_PathExt("UI\\FrameDef\\Glue\\MainMenu.fdf"), "fdf");
}

void run_tool_common_tests(void) {
    printf("[tool common]\n");
    RUN_TEST(test_normalize_slashes_to_backslashes);
    RUN_TEST(test_trim_edge_slashes_removes_both_sides);
    RUN_TEST(test_path_join_uses_forward_slash);
    RUN_TEST(test_path_parent_and_basename_follow_archive_paths);
}
