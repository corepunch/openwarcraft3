#include <string.h>

#include "test_framework.h"
#include "test_harness.h"

LPCUIFRAME SCR_Clear(HANDLE data);
LPCRECT SCR_LayoutRect(LPCUIFRAME frame);
VECTOR2 SCR_GetAxisBounds(LPCRECT rect, bool is_x_axis);
FLOAT SCR_NormalizeAnchorOffset(uiFramePoint_t const *p, bool is_x_axis);
VECTOR2 SCR_SolveAxisPosition(LPCUIFRAME frame,
                              uiFramePoints_t const points,
                              FLOAT width,
                              bool is_x_axis);

static LPUIFRAME setup_layout_root(void) {
    LPUIFRAME frames = SCR_Clear(NULL);
    ASSERT_NOT_NULL(frames);
    return frames;
}

static void test_axis_bounds_returns_x_interval(void) {
    RECT r = { 1.0f, 2.0f, 3.0f, 4.0f };
    VECTOR2 b = SCR_GetAxisBounds(&r, true);
    ASSERT_FLOAT_EQ(b.x, 1.0f);
    ASSERT_FLOAT_EQ(b.y, 4.0f);
}

static void test_axis_bounds_returns_y_interval(void) {
    RECT r = { 1.0f, 2.0f, 3.0f, 4.0f };
    VECTOR2 b = SCR_GetAxisBounds(&r, false);
    ASSERT_FLOAT_EQ(b.x, 2.0f);
    ASSERT_FLOAT_EQ(b.y, 6.0f);
}

static void test_normalize_anchor_offset_x_is_positive(void) {
    uiFramePoint_t p = { 0 };
    p.offset = 3277;
    ASSERT_EQ_FLOAT(SCR_NormalizeAnchorOffset(&p, true), 3277.0f / UI_FRAMEPOINT_SCALE, 0.0001f);
}

static void test_normalize_anchor_offset_y_is_positive(void) {
    uiFramePoint_t p = { 0 };
    p.offset = 3277;
    ASSERT_EQ_FLOAT(SCR_NormalizeAnchorOffset(&p, false), 3277.0f / UI_FRAMEPOINT_SCALE, 0.0001f);
}

static void test_solve_axis_center_anchor_x(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[1];
    VECTOR2 pos;

    memset(f, 0, sizeof(*f));
    f->number = 1;
    f->parent = 0;
    f->size.width = 0.2f;
    f->points.x[FPP_MID].used = 1;
    f->points.x[FPP_MID].targetPos = FPP_MID;
    f->points.x[FPP_MID].relativeTo = 0;
    f->points.x[FPP_MID].offset = 0;

    pos = SCR_SolveAxisPosition(f, f->points.x, f->size.width, true);
    ASSERT_FLOAT_EQ(pos.x, 0.3f);
    ASSERT_FLOAT_EQ(pos.y, 0.2f);
}

static void test_solve_axis_center_anchor_y_with_positive_offset(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[1];
    VECTOR2 pos;
    SHORT off = (SHORT)(0.05f * UI_FRAMEPOINT_SCALE);

    memset(f, 0, sizeof(*f));
    f->number = 1;
    f->parent = 0;
    f->size.height = 0.1f;
    f->points.y[FPP_MID].used = 1;
    f->points.y[FPP_MID].targetPos = FPP_MID;
    f->points.y[FPP_MID].relativeTo = 0;
    f->points.y[FPP_MID].offset = off;

    pos = SCR_SolveAxisPosition(f, f->points.y, f->size.height, false);
    ASSERT_EQ_FLOAT(pos.x, 0.3f, 0.001f);
    ASSERT_FLOAT_EQ(pos.y, 0.1f);
}

static void test_solve_axis_min_anchor_x(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[1];
    VECTOR2 pos;

    memset(f, 0, sizeof(*f));
    f->number = 1;
    f->parent = 0;
    f->size.width = 0.15f;
    f->points.x[FPP_MIN].used = 1;
    f->points.x[FPP_MIN].targetPos = FPP_MIN;
    f->points.x[FPP_MIN].relativeTo = 0;
    f->points.x[FPP_MIN].offset = (SHORT)(0.02f * UI_FRAMEPOINT_SCALE);

    pos = SCR_SolveAxisPosition(f, f->points.x, f->size.width, true);
    ASSERT_EQ_FLOAT(pos.x, 0.02f, 0.001f);
    ASSERT_FLOAT_EQ(pos.y, 0.15f);
}

static void test_solve_axis_max_anchor_x(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[1];
    VECTOR2 pos;

    memset(f, 0, sizeof(*f));
    f->number = 1;
    f->parent = 0;
    f->size.width = 0.10f;
    f->points.x[FPP_MAX].used = 1;
    f->points.x[FPP_MAX].targetPos = FPP_MAX;
    f->points.x[FPP_MAX].relativeTo = 0;
    f->points.x[FPP_MAX].offset = (SHORT)(-0.02f * UI_FRAMEPOINT_SCALE);

    pos = SCR_SolveAxisPosition(f, f->points.x, f->size.width, true);
    ASSERT_EQ_FLOAT(pos.x, 0.68f, 0.002f);
    ASSERT_FLOAT_EQ(pos.y, 0.10f);
}

static void test_solve_axis_dual_anchor_stretch_x(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[1];
    VECTOR2 pos;

    memset(f, 0, sizeof(*f));
    f->number = 1;
    f->parent = 0;
    f->points.x[FPP_MIN].used = 1;
    f->points.x[FPP_MIN].targetPos = FPP_MIN;
    f->points.x[FPP_MIN].relativeTo = 0;
    f->points.x[FPP_MIN].offset = (SHORT)(0.10f * UI_FRAMEPOINT_SCALE);
    f->points.x[FPP_MAX].used = 1;
    f->points.x[FPP_MAX].targetPos = FPP_MAX;
    f->points.x[FPP_MAX].relativeTo = 0;
    f->points.x[FPP_MAX].offset = (SHORT)(-0.10f * UI_FRAMEPOINT_SCALE);

    pos = SCR_SolveAxisPosition(f, f->points.x, 0.0f, true);
    ASSERT_EQ_FLOAT(pos.x, 0.10f, 0.001f);
    ASSERT_EQ_FLOAT(pos.y, 0.60f, 0.002f);
}

static void test_layout_rect_uses_solver_for_screen_relative_points(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[2];
    LPCRECT r;

    memset(f, 0, sizeof(*f));
    f->number = 2;
    f->parent = 0;
    f->size.width = 0.2f;
    f->size.height = 0.1f;

    f->points.x[FPP_MID].used = 1;
    f->points.x[FPP_MID].targetPos = FPP_MID;
    f->points.x[FPP_MID].relativeTo = 0;

    f->points.y[FPP_MAX].used = 1;
    f->points.y[FPP_MAX].targetPos = FPP_MAX;
    f->points.y[FPP_MAX].relativeTo = 0;

    r = SCR_LayoutRect(f);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ_FLOAT(r->x, 0.3f, 0.001f);
    ASSERT_EQ_FLOAT(r->y, 0.5f, 0.001f);
    ASSERT_EQ_FLOAT(r->w, 0.2f, 0.001f);
    ASSERT_EQ_FLOAT(r->h, 0.1f, 0.001f);
}

static void test_layout_rect_dual_anchor_overrides_explicit_width(void) {
    LPUIFRAME frames = setup_layout_root();
    uiFrame_t *f = &frames[3];
    LPCRECT r;

    memset(f, 0, sizeof(*f));
    f->number = 3;
    f->parent = 0;
    f->size.width = 0.05f;
    f->size.height = 0.05f;

    f->points.x[FPP_MIN].used = 1;
    f->points.x[FPP_MIN].targetPos = FPP_MIN;
    f->points.x[FPP_MIN].relativeTo = 0;
    f->points.x[FPP_MAX].used = 1;
    f->points.x[FPP_MAX].targetPos = FPP_MAX;
    f->points.x[FPP_MAX].relativeTo = 0;

    f->points.y[FPP_MIN].used = 1;
    f->points.y[FPP_MIN].targetPos = FPP_MIN;
    f->points.y[FPP_MIN].relativeTo = 0;

    r = SCR_LayoutRect(f);
    ASSERT_NOT_NULL(r);
    ASSERT_EQ_FLOAT(r->x, 0.0f, 0.001f);
    ASSERT_EQ_FLOAT(r->w, 0.8f, 0.001f);
}

BEGIN_SUITE(ui_layout)
    RUN_TEST(test_axis_bounds_returns_x_interval);
    RUN_TEST(test_axis_bounds_returns_y_interval);
    RUN_TEST(test_normalize_anchor_offset_x_is_positive);
    RUN_TEST(test_normalize_anchor_offset_y_is_positive);
    RUN_TEST(test_solve_axis_center_anchor_x);
    RUN_TEST(test_solve_axis_center_anchor_y_with_positive_offset);
    RUN_TEST(test_solve_axis_min_anchor_x);
    RUN_TEST(test_solve_axis_max_anchor_x);
    RUN_TEST(test_solve_axis_dual_anchor_stretch_x);
    RUN_TEST(test_layout_rect_uses_solver_for_screen_relative_points);
    RUN_TEST(test_layout_rect_dual_anchor_overrides_explicit_width);
END_SUITE()
