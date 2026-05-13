#include <string.h>

#include "test_framework.h"
#include "test_harness.h"

LPCUIFRAME SCR_Clear(HANDLE data);
VECTOR2 SCR_SolveAxisPosition(LPCUIFRAME frame,
                              uiFramePoints_t const points,
                              FLOAT width,
                              bool is_x_axis);

static void setup_logo_top_left(uiFrame_t *frame, float x_offset_ui, float y_offset_ui) {
    memset(frame, 0, sizeof(*frame));
    frame->number = 1;
    frame->parent = 0;

    frame->points.x[FPP_MIN].used = 1;
    frame->points.x[FPP_MIN].targetPos = FPP_MIN;
    frame->points.x[FPP_MIN].relativeTo = 0;
    frame->points.x[FPP_MIN].offset = (SHORT)(x_offset_ui * UI_FRAMEPOINT_SCALE);

    frame->points.y[FPP_MAX].used = 1;
    frame->points.y[FPP_MAX].targetPos = FPP_MAX;
    frame->points.y[FPP_MAX].relativeTo = 0;
    frame->points.y[FPP_MAX].offset = (SHORT)(y_offset_ui * UI_FRAMEPOINT_SCALE);
}

static float ui_to_px_x(float x_ui) {
    return x_ui * 1000.0f;
}

static float ui_to_px_y_from_top(float y_ui) {
    return (0.6f - y_ui) * 1000.0f;
}

static void test_logo_center_from_fdf_offset_is_not_warcraft_position(void) {
    LPUIFRAME frames = SCR_Clear(NULL);
    uiFrame_t *logo = &frames[1];

    float const model_center_x_ui = (-0.112f + 0.111f) * 0.5f;
    float const model_center_y_ui = (-0.058f + 0.053f) * 0.5f;

    setup_logo_top_left(logo, 0.13f, 0.04f);

    VECTOR2 x_pos = SCR_SolveAxisPosition(logo, logo->points.x, 0.0f, true);
    VECTOR2 y_pos = SCR_SolveAxisPosition(logo, logo->points.y, 0.0f, false);
    float center_x_px = ui_to_px_x(x_pos.x + model_center_x_ui);
    float center_y_px_from_top = ui_to_px_y_from_top(y_pos.x + model_center_y_ui);

    ASSERT_EQ_FLOAT(center_x_px, 129.5f, 1.0f);
    ASSERT_EQ_FLOAT(center_y_px_from_top, -37.5f, 2.0f);
}

static void test_logo_center_with_warsmash_menu_offset_is_near_expected_region(void) {
    LPUIFRAME frames = SCR_Clear(NULL);
    uiFrame_t *logo = &frames[1];

    float const model_center_x_ui = (-0.112f + 0.111f) * 0.5f;
    float const model_center_y_ui = (-0.058f + 0.053f) * 0.5f;

    setup_logo_top_left(logo, 0.13f, -0.08f);

    VECTOR2 x_pos = SCR_SolveAxisPosition(logo, logo->points.x, 0.0f, true);
    VECTOR2 y_pos = SCR_SolveAxisPosition(logo, logo->points.y, 0.0f, false);
    float center_x_px = ui_to_px_x(x_pos.x + model_center_x_ui);
    float center_y_px_from_top = ui_to_px_y_from_top(y_pos.x + model_center_y_ui);

    ASSERT_EQ_FLOAT(center_x_px, 129.5f, 1.0f);
    ASSERT(center_y_px_from_top > 60.0f);
    ASSERT(center_y_px_from_top < 95.0f);
}

BEGIN_SUITE(ui_logo_layout)
    RUN_TEST(test_logo_center_from_fdf_offset_is_not_warcraft_position);
    RUN_TEST(test_logo_center_with_warsmash_menu_offset_is_near_expected_region);
END_SUITE()
