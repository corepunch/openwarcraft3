#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "test_harness.h"

static const char *captured_image_path;
static const char *captured_model_path;

static int fake_image_index(LPCSTR name) {
    captured_image_path = name;
    return (name && *name) ? 123 : 0;
}

static int fake_model_index(LPCSTR name) {
    captured_model_path = name;
    return (name && *name) ? 456 : 0;
}

static void parse_fdf(const char *name, const char *src) {
    char *buf = strdup(src);
    ASSERT_NOT_NULL(buf);
    if (!buf) {
        return;
    }
    UI_ParseFDF_Buffer(name, buf);
    free(buf);
}

static int require_not_null(const void *ptr) {
    ASSERT_NOT_NULL(ptr);
    return ptr != NULL;
}

static void reset_ui_state(void) {
    UI_ClearTemplates();
    captured_image_path = NULL;
    captured_model_path = NULL;
    gi.ImageIndex = fake_image_index;
    gi.ModelIndex = fake_model_index;
}

static void test_parse_single_frame_definition(void) {
    LPFRAMEDEF root;

    reset_ui_state();
    parse_fdf("single.fdf",
              "Frame \"FRAME\" \"Root\" { Width 0.5, Height 0.25, }");

    root = UI_FindFrame("Root");
    if (!require_not_null(root)) return;
    ASSERT_EQ_INT(root->Type, FT_FRAME);
    ASSERT_FLOAT_EQ(root->Width, 0.5f);
    ASSERT_FLOAT_EQ(root->Height, 0.25f);
    ASSERT_NULL(root->Parent);
}

static void test_parse_nested_parent_child_relationship(void) {
    LPFRAMEDEF root;
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("nested.fdf",
              "Frame \"FRAME\" \"Root\" { "
              "Frame \"TEXT\" \"Child\" { Text \"Hello\", }"
              " }");

    root = UI_FindFrame("Root");
    child = UI_FindFrame("Child");
    if (!require_not_null(root)) return;
    if (!require_not_null(child)) return;
    ASSERT_EQ_INT(child->Type, FT_TEXT);
    ASSERT(child->Parent == root);
    ASSERT_STR_EQ(child->Text, "Hello");
}

static void test_inherits_copies_compatible_type_fields(void) {
    LPFRAMEDEF base;
    LPFRAMEDEF derived;

    reset_ui_state();
    parse_fdf("inherits_ok.fdf",
              "Frame \"FRAME\" \"Base\" { Width 0.33, Height 0.44, }"
              "Frame \"FRAME\" \"Derived\" INHERITS \"Base\" { }"
    );

    base = UI_FindFrame("Base");
    derived = UI_FindFrame("Derived");

    if (!require_not_null(base)) return;
    if (!require_not_null(derived)) return;
    ASSERT_FLOAT_EQ(derived->Width, base->Width);
    ASSERT_FLOAT_EQ(derived->Height, base->Height);
    ASSERT_EQ_INT(derived->Type, FT_FRAME);
}

static void test_inherits_rejects_incompatible_type(void) {
    LPFRAMEDEF base_text;
    LPFRAMEDEF derived_frame;

    reset_ui_state();
    parse_fdf("inherits_bad.fdf",
              "Frame \"TEXT\" \"BaseText\" { Width 0.77, }"
              "Frame \"FRAME\" \"DerivedFrame\" INHERITS \"BaseText\" { Height 0.25, }"
    );

    base_text = UI_FindFrame("BaseText");
    derived_frame = UI_FindFrame("DerivedFrame");

    if (!require_not_null(base_text)) return;
    if (!require_not_null(derived_frame)) return;
    ASSERT_FLOAT_EQ(base_text->Width, 0.77f);
    ASSERT_FLOAT_EQ(derived_frame->Height, 0.25f);
    ASSERT_FLOAT_EQ(derived_frame->Width, 0.0f);
}

static void test_setpoint_top_left_sets_top_y_anchor(void) {
    LPFRAMEDEF root;
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_tl.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint TOPLEFT, \"Root\", TOPLEFT, 0.01, -0.02,"
              " }"
              "}");

    root = UI_FindFrame("Root");
    child = UI_FindFrame("Child");
    if (!require_not_null(root)) return;
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MIN].offset, 0.01f);
    ASSERT(child->Points.x[FPP_MIN].relativeTo == root);

    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, -0.02f);
    ASSERT(child->Points.y[FPP_MAX].relativeTo == root);
}

static void test_setallpoints_sets_min_and_max(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setall.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" { SetAllPoints, }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
}

static void test_anchor_translates_to_setpoint_state(void) {
    LPFRAMEDEF frame;

    reset_ui_state();
    parse_fdf("anchor.fdf",
              "Frame \"FRAME\" \"Root\" { Anchor BOTTOMRIGHT, -0.03, 0.04, }");

    frame = UI_FindFrame("Root");
    if (!require_not_null(frame)) return;
    ASSERT_EQ_INT(frame->Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(frame->Points.y[FPP_MIN].used, 1);
    ASSERT_FLOAT_EQ(frame->Points.x[FPP_MAX].offset, -0.03f);
    ASSERT_FLOAT_EQ(frame->Points.y[FPP_MIN].offset, 0.04f);
}

static void test_backdrop_flags_and_insets_are_parsed(void) {
    LPFRAMEDEF frame;

    reset_ui_state();
    parse_fdf("backdrop_flags.fdf",
              "Frame \"BACKDROP\" \"BD\" {"
              " BackdropTileBackground,"
              " BackdropBlendAll,"
              " BackdropBackgroundInsets 0.1 0.2 0.3 0.4,"
              "}"
    );

    frame = UI_FindFrame("BD");
    if (!require_not_null(frame)) return;
    ASSERT_EQ_INT(frame->Type, FT_BACKDROP);
    ASSERT_EQ_INT(frame->Backdrop.TileBackground, 1);
    ASSERT_EQ_INT(frame->Backdrop.BlendAll, 1);
    ASSERT_FLOAT_EQ(frame->Backdrop.BackgroundInsets[0], 0.1f);
    ASSERT_FLOAT_EQ(frame->Backdrop.BackgroundInsets[1], 0.2f);
    ASSERT_FLOAT_EQ(frame->Backdrop.BackgroundInsets[2], 0.3f);
    ASSERT_FLOAT_EQ(frame->Backdrop.BackgroundInsets[3], 0.4f);
}

static void test_vector_parser_accepts_f_suffixes(void) {
    LPFRAMEDEF frame;

    reset_ui_state();
    parse_fdf("vector_f_suffix.fdf",
              "Frame \"TEXTBUTTON\" \"Button\" {"
              " ButtonPushedTextOffset -0.002f -0.003f,"
              "}");

    frame = UI_FindFrame("Button");
    if (!require_not_null(frame)) return;
    ASSERT_FLOAT_EQ(frame->Button.PushedTextOffset.x, -0.002f);
    ASSERT_FLOAT_EQ(frame->Button.PushedTextOffset.y, -0.003f);
}

static void test_backdrop_background_adds_blp_extension(void) {
    LPFRAMEDEF frame;

    reset_ui_state();
    parse_fdf("backdrop_bg_path.fdf",
              "Frame \"BACKDROP\" \"BD\" {"
              " BackdropBackground \"TestUI/Textures/checker_8x8\","
              "}");

    frame = UI_FindFrame("BD");
    if (!require_not_null(frame)) return;
    ASSERT_EQ_INT(frame->Backdrop.Background, 123);
    ASSERT_NOT_NULL(captured_image_path);
    ASSERT_STR_EQ(captured_image_path, "TestUI/Textures/checker_8x8.blp");
}

static void test_background_art_uses_model_index(void) {
    LPFRAMEDEF sprite;

    reset_ui_state();
    parse_fdf("sprite_path.fdf",
              "Frame \"SPRITE\" \"SpriteA\" {"
              " BackgroundArt \"TestUI/Models/quad_sprite.mdx\","
              "}");

    sprite = UI_FindFrame("SpriteA");
    if (!require_not_null(sprite)) return;
    ASSERT_EQ_INT(sprite->Type, FT_SPRITE);
    ASSERT_EQ_INT(sprite->Portrait.model, 456);
    ASSERT_NOT_NULL(captured_model_path);
    ASSERT_STR_EQ(captured_model_path, "TestUI/Models/quad_sprite.mdx");
}

static void test_collect_frame_tree_preorder_matches_writer_traversal(void) {
    LPCFRAMEDEF out[8];
    DWORD count;
    LPFRAMEDEF root;
    LPFRAMEDEF child_a;
    LPFRAMEDEF child_b;
    LPFRAMEDEF grand;

    reset_ui_state();
    parse_fdf("collect_tree.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"A\" {"
              "  Frame \"TEXT\" \"A1\" { Text \"x\", }"
              " }"
              " Frame \"FRAME\" \"B\" { }"
              "}");

    root = UI_FindFrame("Root");
    child_a = UI_FindFrame("A");
    child_b = UI_FindFrame("B");
    grand = UI_FindFrame("A1");
    if (!require_not_null(root)) return;
    if (!require_not_null(child_a)) return;
    if (!require_not_null(child_b)) return;
    if (!require_not_null(grand)) return;

    memset(out, 0, sizeof(out));
    count = UI_CollectFrameTree(root, out, 8);

    ASSERT_EQ_INT((int)count, 4);
    ASSERT(out[0] == root);
    ASSERT(out[1] == child_a);
    ASSERT(out[2] == grand);
    ASSERT(out[3] == child_b);
}

static void test_collect_frame_tree_skips_hidden_children(void) {
    LPCFRAMEDEF out[4];
    DWORD count;
    LPFRAMEDEF root;
    LPFRAMEDEF visible;
    LPFRAMEDEF hidden;

    reset_ui_state();
    parse_fdf("collect_hidden.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Visible\" { }"
              " Frame \"FRAME\" \"Hidden\" { }"
              "}");

    root = UI_FindFrame("Root");
    visible = UI_FindFrame("Visible");
    hidden = UI_FindFrame("Hidden");
    if (!require_not_null(root)) return;
    if (!require_not_null(visible)) return;
    if (!require_not_null(hidden)) return;

    UI_SetHidden(hidden, true);

    memset(out, 0, sizeof(out));
    count = UI_CollectFrameTree(root, out, 4);

    ASSERT_EQ_INT((int)count, 2);
    ASSERT(out[0] == root);
    ASSERT(out[1] == visible);
}

static void test_collect_frame_tree_skips_button_control_art(void) {
    LPCFRAMEDEF out[4];
    DWORD count;
    LPFRAMEDEF button;
    LPFRAMEDEF text;

    reset_ui_state();
    parse_fdf("collect_button_art.fdf",
              "Frame \"GLUETEXTBUTTON\" \"Button\" {"
              " ControlBackdrop \"ButtonBackdrop\","
              " ControlPushedBackdrop \"ButtonPushedBackdrop\","
              " ControlDisabledBackdrop \"ButtonDisabledBackdrop\","
              " ControlMouseOverHighlight \"ButtonHighlight\","
              " Frame \"BACKDROP\" \"ButtonBackdrop\" { }"
              " Frame \"BACKDROP\" \"ButtonPushedBackdrop\" { }"
              " Frame \"BACKDROP\" \"ButtonDisabledBackdrop\" { }"
              " Frame \"HIGHLIGHT\" \"ButtonHighlight\" { }"
              " Frame \"TEXT\" \"ButtonText\" { Text \"x\", }"
              "}");

    button = UI_FindFrame("Button");
    text = UI_FindFrame("ButtonText");
    if (!require_not_null(button)) return;
    if (!require_not_null(text)) return;

    memset(out, 0, sizeof(out));
    count = UI_CollectFrameTree(button, out, 4);

    ASSERT_EQ_INT((int)count, 2);
    ASSERT(out[0] == button);
    ASSERT(out[1] == text);
}

static void test_collect_frame_tree_returns_total_when_truncated(void) {
    LPCFRAMEDEF out[2];
    DWORD count;
    LPFRAMEDEF root;

    reset_ui_state();
    parse_fdf("collect_truncated.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"A\" { }"
              " Frame \"FRAME\" \"B\" { }"
              " Frame \"FRAME\" \"C\" { }"
              "}");

    root = UI_FindFrame("Root");
    if (!require_not_null(root)) return;

    memset(out, 0, sizeof(out));
    count = UI_CollectFrameTree(root, out, 2);

    ASSERT_EQ_INT((int)count, 4);
    ASSERT(out[0] == root);
    ASSERT_NOT_NULL(out[1]);
}

static void test_find_child_frame_descends_recursively(void) {
    LPFRAMEDEF root;
    LPFRAMEDEF found;

    reset_ui_state();
    parse_fdf("find_child.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"L1\" {"
              "  Frame \"FRAME\" \"L2\" { }"
              " }"
              "}");

    root = UI_FindFrame("Root");
    if (!require_not_null(root)) return;

    found = UI_FindChildFrame(root, "L2");
    if (!require_not_null(found)) return;
    ASSERT_STR_EQ(found->Name, "L2");
}

static void test_programmatic_setpoint_maps_to_points(void) {
    FRAMEDEF root;
    FRAMEDEF child;

    reset_ui_state();

    UI_InitFrame(&root, FT_FRAME);
    strcpy(root.Name, "Root");

    UI_InitFrame(&child, FT_FRAME);
    strcpy(child.Name, "Child");

    UI_SetPoint(&child, FRAMEPOINT_CENTER, &root, FRAMEPOINT_TOPLEFT, 0.11f, -0.22f);

    ASSERT_EQ_INT(child.Points.x[FPP_MID].used, 1);
    ASSERT_EQ_INT(child.Points.y[FPP_MID].used, 1);
    ASSERT_EQ_INT(child.Points.x[FPP_MID].targetPos, FPP_MIN);
    ASSERT_EQ_INT(child.Points.y[FPP_MID].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child.Points.x[FPP_MID].offset, 0.11f);
    ASSERT_FLOAT_EQ(child.Points.y[FPP_MID].offset, -0.22f);
    ASSERT(child.Points.x[FPP_MID].relativeTo == &root);
    ASSERT(child.Points.y[FPP_MID].relativeTo == &root);
}

static void test_programmatic_setallpoints_sets_both_axes(void) {
    FRAMEDEF frame;

    reset_ui_state();
    UI_InitFrame(&frame, FT_FRAME);
    UI_SetAllPoints(&frame);

    ASSERT_EQ_INT(frame.Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(frame.Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MAX].used, 1);
}

/* --- SetPoint: coverage for each FRAMEPOINT position --- */

static void test_setpoint_top_maps_mid_x_max_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_top.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint TOP, \"Root\", TOP, 0.03, -0.07,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MID].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MID].targetPos, FPP_MID);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MID].offset, 0.03f);

    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, -0.07f);
}

static void test_setpoint_topright_maps_max_x_max_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_topright.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint TOPRIGHT, \"Root\", TOPRIGHT, 0.05, -0.10,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MAX].offset, 0.05f);

    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, -0.10f);
}

static void test_setpoint_left_maps_min_x_mid_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_left.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint LEFT, \"Root\", LEFT, 0.02, 0.04,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MIN].offset, 0.02f);

    ASSERT_EQ_INT(child->Points.y[FPP_MID].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MID].targetPos, FPP_MID);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MID].offset, 0.04f);
}

static void test_setpoint_center_maps_mid_x_mid_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_center.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint CENTER, \"Root\", CENTER, 0.0, 0.0,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MID].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MID].targetPos, FPP_MID);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MID].offset, 0.0f);

    ASSERT_EQ_INT(child->Points.y[FPP_MID].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MID].targetPos, FPP_MID);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MID].offset, 0.0f);
}

static void test_setpoint_right_maps_max_x_mid_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_right.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint RIGHT, \"Root\", RIGHT, -0.06, 0.0,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MAX].offset, -0.06f);

    ASSERT_EQ_INT(child->Points.y[FPP_MID].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MID].targetPos, FPP_MID);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MID].offset, 0.0f);
}

static void test_setpoint_bottomleft_maps_min_x_min_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_bottomleft.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint BOTTOMLEFT, \"Root\", BOTTOMLEFT, 0.01, 0.02,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MIN].offset, 0.01f);

    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, 0.02f);
}

static void test_setpoint_bottom_maps_mid_x_min_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_bottom.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint BOTTOM, \"Root\", BOTTOM, 0.0, 0.08,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MID].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MID].targetPos, FPP_MID);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MID].offset, 0.0f);

    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, 0.08f);
}

static void test_setpoint_bottomright_maps_max_x_min_y(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setpoint_bottomright.fdf",
              "Frame \"FRAME\" \"Root\" {"
              " Frame \"FRAME\" \"Child\" {"
              "  SetPoint BOTTOMRIGHT, \"Root\", BOTTOMRIGHT, -0.04, 0.05,"
              " }"
              "}");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    ASSERT_EQ_INT(child->Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MAX].offset, -0.04f);

    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, 0.05f);
}

/* --- SetPoint: center-mutex behavior --- */

static void test_setpoint_edge_overrides_and_clears_center(void) {
    FRAMEDEF frame;

    reset_ui_state();
    UI_InitFrame(&frame, FT_FRAME);

    /* Set CENTER first — both mid slots occupied */
    UI_SetPoint(&frame, FRAMEPOINT_CENTER, NULL, FRAMEPOINT_CENTER, 0.5f, 0.5f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MID].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MID].used, 1);

    /* Set TOPLEFT — should clear x-mid and y-mid, then fill x-min and y-max */
    UI_SetPoint(&frame, FRAMEPOINT_TOPLEFT, NULL, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MID].used, 0);
    ASSERT_EQ_INT(frame.Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MID].used, 0);
    ASSERT_EQ_INT(frame.Points.y[FPP_MAX].used, 1);
}

static void test_setpoint_center_ignored_when_edges_set(void) {
    FRAMEDEF frame;

    reset_ui_state();
    UI_InitFrame(&frame, FT_FRAME);

    /* Anchor x and y edges via TOPLEFT + BOTTOMRIGHT */
    UI_SetPoint(&frame, FRAMEPOINT_TOPLEFT, NULL, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    UI_SetPoint(&frame, FRAMEPOINT_BOTTOMRIGHT, NULL, FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f);

    /* Attempt to set CENTER — should be ignored on both axes */
    UI_SetPoint(&frame, FRAMEPOINT_CENTER, NULL, FRAMEPOINT_CENTER, 0.5f, 0.5f);

    ASSERT_EQ_INT(frame.Points.x[FPP_MID].used, 0);
    ASSERT_EQ_INT(frame.Points.y[FPP_MID].used, 0);
    /* Edges must still be intact */
    ASSERT_EQ_INT(frame.Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(frame.Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MAX].used, 1);
}

/* --- SetAllPoints: detailed field verification --- */

static void test_setallpoints_zero_offsets_and_target_positions(void) {
    FRAMEDEF frame;

    reset_ui_state();
    UI_InitFrame(&frame, FT_FRAME);
    UI_SetAllPoints(&frame);

    /* TOPLEFT anchor: x[MIN] left->left, y[MAX] top->top */
    ASSERT_FLOAT_EQ(frame.Points.x[FPP_MIN].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_NULL(frame.Points.x[FPP_MIN].relativeTo);

    ASSERT_FLOAT_EQ(frame.Points.y[FPP_MAX].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_NULL(frame.Points.y[FPP_MAX].relativeTo);

    /* BOTTOMRIGHT anchor: x[MAX] right->right, y[MIN] bottom->bottom */
    ASSERT_FLOAT_EQ(frame.Points.x[FPP_MAX].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_NULL(frame.Points.x[FPP_MAX].relativeTo);

    ASSERT_FLOAT_EQ(frame.Points.y[FPP_MIN].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_NULL(frame.Points.y[FPP_MIN].relativeTo);

    /* Center slot must remain unused */
    ASSERT_EQ_INT(frame.Points.x[FPP_MID].used, 0);
    ASSERT_EQ_INT(frame.Points.y[FPP_MID].used, 0);
}

static void test_setallpoints_with_relative_frame_propagates_to_both_anchors(void) {
    LPFRAMEDEF child;

    reset_ui_state();
    parse_fdf("setallpoints_rel.fdf",
              "Frame \"FRAME\" \"Anchor\" { Width 0.8, Height 0.6, }"
              "Frame \"FRAME\" \"Child\" { SetAllPoints, }");

    child = UI_FindFrame("Child");
    if (!require_not_null(child)) return;

    /* SetAllPoints via FDF uses NULL relativeTo (resolved at layout time) */
    ASSERT_EQ_INT(child->Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.x[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MIN].offset, 0.0f);
    ASSERT_FLOAT_EQ(child->Points.x[FPP_MAX].offset, 0.0f);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, 0.0f);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, 0.0f);
}

static void test_text_uses_key_when_no_stringlist_entry_exists(void) {
    LPFRAMEDEF text;

    reset_ui_state();
    parse_fdf("text_key_passthrough.fdf",
              "Frame \"TEXT\" \"TextA\" {"
              " Text \"TRIGSTR_999\","
              "}");

    text = UI_FindFrame("TextA");
    if (!require_not_null(text)) return;
    ASSERT_STR_EQ(text->Text, "TRIGSTR_999");
}

static void test_long_stringlist_text_is_bounded_to_frame_storage(void) {
    LPFRAMEDEF text;

    reset_ui_state();
    parse_fdf("long_text.fdf",
              "StringList { LONG_TEXT "
              "\"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
              "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\","
              "}"
              "Frame \"TEXT\" \"TextA\" {"
              " Text \"LONG_TEXT\","
              "}");

    text = UI_FindFrame("TextA");
    if (!require_not_null(text)) return;
    ASSERT_EQ_INT(strlen(text->Text), sizeof(text->TextStorage) - 1);
    ASSERT_NULL(text->Tip);
    ASSERT_NULL(text->Ubertip);
}

static void test_duplicate_name_prefers_first_template(void) {
    LPFRAMEDEF found;

    reset_ui_state();
    parse_fdf("dup_name.fdf",
              "Frame \"FRAME\" \"Dup\" { Width 0.10, }"
              "Frame \"FRAME\" \"Dup\" { Width 0.20, }");

    found = UI_FindFrame("Dup");
    if (!require_not_null(found)) return;
    ASSERT_FLOAT_EQ(found->Width, 0.10f);
}

static void test_unknown_token_does_not_crash_existing_definitions(void) {
    LPFRAMEDEF good;

    reset_ui_state();
    parse_fdf("unknown_token.fdf",
              "Frame \"FRAME\" \"Good\" { Width 0.5, }"
              "Frame \"FRAME\" \"Bad\" { UnknownThing 1, }");

    good = UI_FindFrame("Good");
    if (!require_not_null(good)) return;
    ASSERT_FLOAT_EQ(good->Width, 0.5f);
}

BEGIN_SUITE(ui_fdf)
    RUN_TEST(test_parse_single_frame_definition);
    RUN_TEST(test_parse_nested_parent_child_relationship);
    RUN_TEST(test_inherits_copies_compatible_type_fields);
    RUN_TEST(test_inherits_rejects_incompatible_type);
    RUN_TEST(test_setpoint_top_left_sets_top_y_anchor);
    RUN_TEST(test_setallpoints_sets_min_and_max);
    RUN_TEST(test_anchor_translates_to_setpoint_state);
    RUN_TEST(test_backdrop_flags_and_insets_are_parsed);
    RUN_TEST(test_vector_parser_accepts_f_suffixes);
    RUN_TEST(test_backdrop_background_adds_blp_extension);
    RUN_TEST(test_background_art_uses_model_index);
    RUN_TEST(test_collect_frame_tree_preorder_matches_writer_traversal);
    RUN_TEST(test_collect_frame_tree_skips_hidden_children);
    RUN_TEST(test_collect_frame_tree_skips_button_control_art);
    RUN_TEST(test_collect_frame_tree_returns_total_when_truncated);
    RUN_TEST(test_find_child_frame_descends_recursively);
    RUN_TEST(test_programmatic_setpoint_maps_to_points);
    RUN_TEST(test_programmatic_setallpoints_sets_both_axes);
    RUN_TEST(test_setpoint_top_maps_mid_x_max_y);
    RUN_TEST(test_setpoint_topright_maps_max_x_max_y);
    RUN_TEST(test_setpoint_left_maps_min_x_mid_y);
    RUN_TEST(test_setpoint_center_maps_mid_x_mid_y);
    RUN_TEST(test_setpoint_right_maps_max_x_mid_y);
    RUN_TEST(test_setpoint_bottomleft_maps_min_x_min_y);
    RUN_TEST(test_setpoint_bottom_maps_mid_x_min_y);
    RUN_TEST(test_setpoint_bottomright_maps_max_x_min_y);
    RUN_TEST(test_setpoint_edge_overrides_and_clears_center);
    RUN_TEST(test_setpoint_center_ignored_when_edges_set);
    RUN_TEST(test_setallpoints_zero_offsets_and_target_positions);
    RUN_TEST(test_setallpoints_with_relative_frame_propagates_to_both_anchors);
    RUN_TEST(test_text_uses_key_when_no_stringlist_entry_exists);
    RUN_TEST(test_long_stringlist_text_is_bounded_to_frame_storage);
    RUN_TEST(test_duplicate_name_prefers_first_template);
    RUN_TEST(test_unknown_token_does_not_crash_existing_definitions);
END_SUITE()
