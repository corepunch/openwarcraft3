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

    frame->points.y[FPP_MIN].used = 1;
    frame->points.y[FPP_MIN].targetPos = FPP_MIN;
    frame->points.y[FPP_MIN].relativeTo = 0;
    frame->points.y[FPP_MIN].offset = (SHORT)(y_offset_ui * UI_FRAMEPOINT_SCALE);
}

static void setup_sprite_bottom_left(uiFrame_t *frame) {
    memset(frame, 0, sizeof(*frame));
    frame->number = 1;
    frame->parent = 0;

    frame->points.x[FPP_MIN].used = 1;
    frame->points.x[FPP_MIN].targetPos = FPP_MIN;
    frame->points.x[FPP_MIN].relativeTo = 0;

    frame->points.y[FPP_MAX].used = 1;
    frame->points.y[FPP_MAX].targetPos = FPP_MAX;
    frame->points.y[FPP_MAX].relativeTo = 0;
}

static VECTOR3 sprite_vertex_to_ndc(float ui_x,
                                    float ui_y,
                                    float vertex_x,
                                    float vertex_y)
{
    MATRIX4 projection;
    MATRIX4 model;
    MATRIX4 mvp;

    Matrix4_ortho(&projection, 0.0f, 0.8f, 0.0f, 0.6f, 0.0f, 100.0f);
    Matrix4_identity(&model);
    Matrix4_translate(&model, &(VECTOR3) {
        ui_x,
        0.6f - ui_y,
        0.0f
    });
    Matrix4_multiply(&projection, &model, &mvp);

    return Matrix4_multiply_vector3(&mvp, &(VECTOR3) {
        vertex_x,
        vertex_y,
        0.0f
    });
}

static float ndc_to_top_pixel(float ndc_y) {
    return (1.0f - ndc_y) * 300.0f;
}

static void test_bottom_anchored_panel_sprite_extends_onto_screen(void) {
    LPUIFRAME frames = SCR_Clear(NULL);
    uiFrame_t *panel = &frames[1];
    VECTOR3 bottom_left;
    VECTOR3 top_right;

    setup_sprite_bottom_left(panel);

    VECTOR2 x_pos = SCR_SolveAxisPosition(panel, panel->points.x, 0.0f, true);
    VECTOR2 y_pos = SCR_SolveAxisPosition(panel, panel->points.y, 0.0f, false);
    bottom_left = sprite_vertex_to_ndc(x_pos.x, y_pos.x, 0.0f, 0.0f);
    top_right = sprite_vertex_to_ndc(x_pos.x, y_pos.x, 0.6f, 0.4f);

    ASSERT_FLOAT_EQ(bottom_left.x, -1.0f);
    ASSERT_FLOAT_EQ(bottom_left.y, -1.0f);
    ASSERT_FLOAT_EQ(top_right.x, 0.5f);
    ASSERT_EQ_FLOAT(top_right.y, 0.3333f, 0.01f);
    ASSERT(top_right.y <= 1.0f);
}

static void test_logo_top_anchor_projects_near_top_of_screen(void) {
    LPUIFRAME frames = SCR_Clear(NULL);
    uiFrame_t *logo = &frames[1];

    float const model_center_x_ui = (-0.112f + 0.111f) * 0.5f;
    float const model_center_y_ui = (-0.058f + 0.053f) * 0.5f;
    VECTOR3 center;
    float center_y_px_from_top;

    setup_logo_top_left(logo, 0.13f, -0.08f);

    VECTOR2 x_pos = SCR_SolveAxisPosition(logo, logo->points.x, 0.0f, true);
    VECTOR2 y_pos = SCR_SolveAxisPosition(logo, logo->points.y, 0.0f, false);
    center = sprite_vertex_to_ndc(x_pos.x, y_pos.x, model_center_x_ui, model_center_y_ui);
    center_y_px_from_top = ndc_to_top_pixel(center.y);

    ASSERT(center_y_px_from_top > 60.0f);
    ASSERT(center_y_px_from_top < 95.0f);
}

BEGIN_SUITE(ui_logo_layout)
    RUN_TEST(test_bottom_anchored_panel_sprite_extends_onto_screen);
    RUN_TEST(test_logo_top_anchor_projects_near_top_of_screen);
END_SUITE()
