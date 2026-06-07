/*
 * test_sc2_cliff_fit.c - StarCraft II 2x2 cliff block weld coverage.
 */

#include "common.h"
#include "games/starcraft-2/common/sc2_cliff_fit.h"
#include "test_framework.h"

static sc2CliffHeightFit_t test_make_edge_fit(FLOAT edge_mid_height) {
    sc2CliffHeightFit_t fit;

    memset(&fit, 0, sizeof(fit));
    fit.footprint = (BOX2){ .min = { -1.0f, -1.0f }, .max = { 1.0f, 1.0f } };
    fit.model_span = 8.0f;
    fit.terrain_span = 1.0f;
    fit.height[2] = 10.0f;
    fit.height[5] = edge_mid_height;
    fit.height[8] = 30.0f;
    return fit;
}

static void test_cliff_fit_samples_middle_side_vertex(void) {
    sc2CliffHeightFit_t fit = test_make_edge_fit(77.0f);

    ASSERT_EQ_FLOAT(SC2_CliffFitSample(fit.height, 1.0f, 0.0f), 10.0f, 0.001f);
    ASSERT_EQ_FLOAT(SC2_CliffFitSample(fit.height, 1.0f, 0.5f), 77.0f, 0.001f);
    ASSERT_EQ_FLOAT(SC2_CliffFitSample(fit.height, 1.0f, 1.0f), 30.0f, 0.001f);
}

static void test_cliff_fit_welds_adjacent_blocks_at_middle_side_vertex(void) {
    sc2CliffHeightFit_t left = test_make_edge_fit(77.0f);
    sc2CliffHeightFit_t right;
    BOX3 model_bounds = { .min = { -1.0f, -1.0f, 0.0f }, .max = { 1.0f, 1.0f, 8.0f } };
    VECTOR2 left_offset = { 1.0f, 1.0f };
    VECTOR2 right_offset = { 3.0f, 1.0f };
    VECTOR3 left_edge = { 1.0f, 0.0f, 0.0f };
    VECTOR3 right_edge = { -1.0f, 0.0f, 0.0f };
    VECTOR3 left_local = left_edge;
    VECTOR3 right_local = right_edge;
    VECTOR2 left_xy;
    VECTOR2 right_xy;

    memset(&right, 0, sizeof(right));
    right.footprint = (BOX2){ .min = { -1.0f, -1.0f }, .max = { 1.0f, 1.0f } };
    right.model_span = 8.0f;
    right.terrain_span = 1.0f;
    right.height[0] = 10.0f;
    right.height[3] = 77.0f;
    right.height[6] = 30.0f;

    left_xy = SC2_CliffFitVertexXY(&left, 1.0f, &left_offset, &left_edge);
    right_xy = SC2_CliffFitVertexXY(&right, 1.0f, &right_offset, &right_edge);
    ASSERT_EQ_FLOAT(left_xy.x, 2.0f, 0.001f);
    ASSERT_EQ_FLOAT(left_xy.y, 1.0f, 0.001f);
    ASSERT_EQ_FLOAT(left_xy.x, right_xy.x, 0.001f);
    ASSERT_EQ_FLOAT(left_xy.y, right_xy.y, 0.001f);
    ASSERT_EQ_FLOAT(SC2_CliffFitVertexZ(&left, &model_bounds, &left_local, &left_edge), 77.0f, 0.001f);
    ASSERT_EQ_FLOAT(SC2_CliffFitVertexZ(&right, &model_bounds, &right_local, &right_edge), 77.0f, 0.001f);
}

static void test_cliff_fit_keeps_middle_height_separate_from_corner_tier(void) {
    sc2CliffHeightFit_t fit = test_make_edge_fit(77.0f);
    BOX3 model_bounds = { .min = { -1.0f, -1.0f, 0.0f }, .max = { 1.0f, 1.0f, 8.0f } };
    VECTOR3 edge_mid = { 1.0f, 0.0f, 0.0f };
    VECTOR3 local = edge_mid;

    fit.terrain_span = 100.0f;
    fit.level[0] = 0.0f;
    fit.level[1] = 0.0f;
    fit.level[2] = 0.0f;
    fit.level[3] = 0.0f;
    ASSERT_EQ_FLOAT(SC2_CliffFitVertexZ(&fit, &model_bounds, &local, &edge_mid), 77.0f, 0.001f);

    fit.level[1] = 1.0f;
    fit.level[2] = 1.0f;
    ASSERT_EQ_FLOAT(SC2_CliffFitSampleCorners(fit.level, 1.0f, 0.5f), 1.0f, 0.001f);
}

void run_sc2_cliff_fit_tests(void) {
    RUN_TEST(test_cliff_fit_samples_middle_side_vertex);
    RUN_TEST(test_cliff_fit_welds_adjacent_blocks_at_middle_side_vertex);
    RUN_TEST(test_cliff_fit_keeps_middle_height_separate_from_corner_tier);
}
