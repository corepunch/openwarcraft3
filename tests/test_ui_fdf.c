#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "test_framework.h"
#include "../ui/ui_local.h"
#include "../ui/ui_dialog.h"
#include "../ui/ui_screen.h"

static void setup_game(void) {}
static void teardown_game(void) {}

static const char *captured_image_path;
static const char *captured_model_path;
static char captured_command[128];

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

static char *normalize_ui_path(LPCSTR path) {
    size_t len;
    char *mapped;
    size_t prefix_len = strlen("data/fdf/");

    if (!path) {
        return NULL;
    }

    len = strlen(path);
    mapped = malloc(prefix_len + len + 1);
    if (!mapped) {
        return NULL;
    }

    memcpy(mapped, "data/fdf/", prefix_len);
    for (size_t i = 0; i < len; i++) {
        mapped[prefix_len + i] = (path[i] == '\\') ? '/' : path[i];
    }
    mapped[prefix_len + len] = '\0';
    return mapped;
}

static int test_fs_read_file(LPCSTR file_name, void **buf) {
    char *mapped;
    FILE *file;
    long size;
    void *data;

    if (!buf) {
        return -1;
    }
    *buf = NULL;

    mapped = normalize_ui_path(file_name);
    if (!mapped) {
        return -1;
    }

    file = fopen(mapped, "rb");
    free(mapped);
    if (!file) {
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    data = malloc((size_t)size + 1);
    if (!data) {
        fclose(file);
        return -1;
    }
    if (fread(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return -1;
    }
    fclose(file);
    ((char *)data)[size] = '\0';
    *buf = data;
    return (int)size;
}

static void test_fs_free_file(void *buf) {
    free(buf);
}

static int test_image_index(LPCSTR name) {
    return (name && *name) ? 123 : 0;
}

static int test_model_index(LPCSTR name) {
    return (name && *name) ? 456 : 0;
}

static LPTEXTURE test_load_texture(LPCSTR name) {
    captured_image_path = name;
    return (LPTEXTURE)1;
}

static LPMODEL test_load_model(LPCSTR name) {
    captured_model_path = name;
    return (LPMODEL)1;
}

static LPRENDERER test_get_renderer(void) {
    static refExport_t renderer = {
        .LoadTexture = test_load_texture,
        .LoadModel = test_load_model,
    };
    return &renderer;
}

static int test_font_index(LPCSTR name, DWORD size) {
    (void)name;
    (void)size;
    return 1;
}

static HANDLE test_ui_mem_alloc(long size) {
    void *ptr = calloc(1u, (size_t)size);
    return ptr;
}

static void test_ui_mem_free(HANDLE ptr) {
    free(ptr);
}

static void test_ui_printf(LPCSTR fmt, ...) {
    (void)fmt;
}

static void test_cmd_execute_text(LPCSTR text) {
    snprintf(captured_command, sizeof(captured_command), "%s", text ? text : "");
}

static void load_ui_file(LPCSTR file_name) {
    uiImport_t saved = uiimport;

    UI_ClearTemplates();
    memset(&uiimport, 0, sizeof(uiimport));
    uiimport.FS_ReadFile = test_fs_read_file;
    uiimport.FS_FreeFile = test_fs_free_file;
    uiimport.MemAlloc = test_ui_mem_alloc;
    uiimport.MemFree = test_ui_mem_free;
    uiimport.ImageIndex = test_image_index;
    uiimport.ModelIndex = test_model_index;
    uiimport.FontIndex = test_font_index;
    uiimport.Printf = test_ui_printf;
    uiimport.Error = test_ui_printf;
    UI_ParseFDF(file_name);
    uiimport = saved;
}

static void load_ui_files(LPCSTR const *file_names, size_t count) {
    uiImport_t saved = uiimport;

    UI_ClearTemplates();
    memset(&uiimport, 0, sizeof(uiimport));
    uiimport.FS_ReadFile = test_fs_read_file;
    uiimport.FS_FreeFile = test_fs_free_file;
    uiimport.MemAlloc = test_ui_mem_alloc;
    uiimport.MemFree = test_ui_mem_free;
    uiimport.ImageIndex = test_image_index;
    uiimport.ModelIndex = test_model_index;
    uiimport.FontIndex = test_font_index;
    uiimport.Printf = test_ui_printf;
    uiimport.Error = test_ui_printf;
    for (size_t i = 0; i < count; i++) {
        UI_ParseFDF(file_names[i]);
    }
    uiimport = saved;
}

static void reset_ui_state(void) {
    UI_ClearTemplates();
    captured_image_path = NULL;
    captured_model_path = NULL;
    captured_command[0] = '\0';
    uiimport.MemAlloc = test_ui_mem_alloc;
    uiimport.MemFree = test_ui_mem_free;
    uiimport.ImageIndex = fake_image_index;
    uiimport.ModelIndex = fake_model_index;
    uiimport.GetRenderer = test_get_renderer;
    uiimport.Printf = test_ui_printf;
    uiimport.Error = test_ui_printf;
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

    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, -0.02f);
    ASSERT(child->Points.y[FPP_MIN].relativeTo == root);
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
    ASSERT_EQ_INT(frame->Points.y[FPP_MAX].used, 1);
    ASSERT_FLOAT_EQ(frame->Points.x[FPP_MAX].offset, -0.03f);
    ASSERT_FLOAT_EQ(frame->Points.y[FPP_MAX].offset, 0.04f);
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

static void test_collect_frame_tree_skips_editbox_text_frame(void) {
    LPCFRAMEDEF out[4];
    DWORD count;
    LPFRAMEDEF editbox;

    reset_ui_state();
    parse_fdf("collect_editbox_text.fdf",
              "Frame \"EDITBOX\" \"Edit\" {"
              " EditTextFrame \"EditText\","
              " Frame \"TEXT\" \"EditText\" { Text \"x\", }"
              "}");

    editbox = UI_FindFrame("Edit");
    if (!require_not_null(editbox)) return;

    memset(out, 0, sizeof(out));
    count = UI_CollectFrameTree(editbox, out, 4);

    ASSERT_EQ_INT((int)count, 1);
    ASSERT(out[0] == editbox);
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
    ASSERT_EQ_INT(child.Points.y[FPP_MID].targetPos, FPP_MIN);
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

    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, -0.07f);
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

    ASSERT_EQ_INT(child->Points.y[FPP_MIN].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MIN].offset, -0.10f);
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

    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, 0.02f);
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

    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, 0.08f);
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

    ASSERT_EQ_INT(child->Points.y[FPP_MAX].used, 1);
    ASSERT_EQ_INT(child->Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_FLOAT_EQ(child->Points.y[FPP_MAX].offset, 0.05f);
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

    /* Set TOPLEFT — should clear x-mid and y-mid, then fill x-min and y-min */
    UI_SetPoint(&frame, FRAMEPOINT_TOPLEFT, NULL, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MID].used, 0);
    ASSERT_EQ_INT(frame.Points.x[FPP_MIN].used, 1);
    ASSERT_EQ_INT(frame.Points.y[FPP_MID].used, 0);
    ASSERT_EQ_INT(frame.Points.y[FPP_MIN].used, 1);
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

    /* TOPLEFT anchor: x[MIN] left->left, y[MIN] top->top */
    ASSERT_FLOAT_EQ(frame.Points.x[FPP_MIN].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_NULL(frame.Points.x[FPP_MIN].relativeTo);

    ASSERT_FLOAT_EQ(frame.Points.y[FPP_MIN].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.y[FPP_MIN].targetPos, FPP_MIN);
    ASSERT_NULL(frame.Points.y[FPP_MIN].relativeTo);

    /* BOTTOMRIGHT anchor: x[MAX] right->right, y[MAX] bottom->bottom */
    ASSERT_FLOAT_EQ(frame.Points.x[FPP_MAX].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.x[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_NULL(frame.Points.x[FPP_MAX].relativeTo);

    ASSERT_FLOAT_EQ(frame.Points.y[FPP_MAX].offset, 0.0f);
    ASSERT_EQ_INT(frame.Points.y[FPP_MAX].targetPos, FPP_MAX);
    ASSERT_NULL(frame.Points.y[FPP_MAX].relativeTo);

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

static void test_esc_menu_confirm_quit_panel_is_available(void) {
    LPFRAMEDEF panel;
    LPFRAMEDEF quit_button;
    LPFRAMEDEF cancel_button;
    LPFRAMEDEF message;

    load_ui_file("UI\\FrameDef\\UI\\EscMenuMainPanel.fdf");

    panel = UI_FindFrame("ConfirmQuitPanel");
    quit_button = UI_FindFrame("ConfirmQuitQuitButton");
    cancel_button = UI_FindFrame("ConfirmQuitCancelButton");
    message = UI_FindFrame("ConfirmQuitMessageText");

    if (!require_not_null(panel)) return;
    if (!require_not_null(quit_button)) return;
    if (!require_not_null(cancel_button)) return;
    if (!require_not_null(message)) return;

    ASSERT_STR_EQ(quit_button->Text, "ConfirmQuitQuitButtonText");
    ASSERT_STR_EQ(cancel_button->Text, "ConfirmQuitCancelButtonText");
    ASSERT_STR_EQ(message->Text, "CONFIRM_EXIT_MESSAGE");
}

static void test_dialog_war3_supports_configurable_button_modes(void) {
    LPCSTR files[] = {
        "UI\\FrameDef\\GlobalStrings.fdf",
        "UI\\FrameDef\\Glue\\StandardTemplates.fdf",
        "UI\\FrameDef\\Glue\\DialogWar3.fdf",
    };
    LPFRAMEDEF root;
    uiDialogWar3_t dialog;
    uiDialogWar3Init_t init = {
        .modal_name = "TestDialogModal",
        .cover_name = "TestDialogModalCover",
        .template_name = "DialogWar3",
    };
    uiDialogWar3Config_t config = {
        .message = "CONFIRM_EXIT_MESSAGE",
        .icon = UI_DIALOG_WAR3_ICON_ERROR,
        .buttons = UI_DIALOG_WAR3_BUTTONS_OK,
        .ok_route = "menu /main/main",
    };

    load_ui_files(files, sizeof(files) / sizeof(files[0]));
    uiimport.GetRenderer = test_get_renderer;
    uiimport.Printf = test_ui_printf;

    root = UI_Spawn(FT_FRAME, NULL);
    if (!require_not_null(root)) return;
    snprintf(root->Name, sizeof(root->Name), "%s", "TestDialogRoot");
    UI_SetSize(root, UI_BASE_WIDTH, UI_BASE_HEIGHT);

    ASSERT(UI_DialogWar3Init(&dialog, root, &init));
    ASSERT(!UI_DialogWar3Visible(&dialog));

    UI_DialogWar3Show(&dialog, &config);
    ASSERT(UI_DialogWar3Visible(&dialog));
    ASSERT_STR_EQ(dialog.dialog->DialogBackdropName, "DialogBackdrop");
    ASSERT_NOT_NULL(UI_FindChildFrame(dialog.dialog, dialog.dialog->DialogBackdropName));
    ASSERT_STR_EQ(dialog.text->Text, "Are you sure you want to exit?");
    ASSERT(dialog.cover->Texture.Image != 0);
    ASSERT_EQ_INT(dialog.cover->Color.a, 128);
    ASSERT(dialog.icon->Backdrop.Background != 0);
    ASSERT(!dialog.ok_backdrop->hidden);
    ASSERT(dialog.no_backdrop->hidden);
    ASSERT(dialog.yes_backdrop->hidden);
    ASSERT_STR_EQ(dialog.ok_button->OnClick, "menu /main/main");

    UI_DialogWar3Hide(&dialog);
    ASSERT(!UI_DialogWar3Visible(&dialog));
}

static void test_main_menu_quit_dialog_routes_to_quit(void) {
    LPCSTR files[] = {
        "UI\\FrameDef\\GlobalStrings.fdf",
        "UI\\FrameDef\\UI\\EscMenuTemplates.fdf",
        "UI\\FrameDef\\UI\\EscMenuMainPanel.fdf",
        "UI\\FrameDef\\Glue\\StandardTemplates.fdf",
        "UI\\FrameDef\\Glue\\MainMenu.fdf",
        "UI\\FrameDef\\Glue\\DialogWar3.fdf",
    };
    LPFRAMEDEF global_exit_button;
    LPFRAMEDEF exit_button;
    LPFRAMEDEF modal;
    LPFRAMEDEF cover;
    LPFRAMEDEF dialog;
    LPFRAMEDEF message;
    LPFRAMEDEF icon;
    LPFRAMEDEF ok_backdrop;
    LPFRAMEDEF no_backdrop;
    LPFRAMEDEF yes_backdrop;
    LPFRAMEDEF no_button;
    LPFRAMEDEF yes_button;
    uiImport_t saved = uiimport;

    load_ui_files(files, sizeof(files) / sizeof(files[0]));

    memset(&uiimport, 0, sizeof(uiimport));
    uiimport.Printf = test_ui_printf;
    uiimport.GetRenderer = test_get_renderer;
    uiimport.Cmd_ExecuteText = test_cmd_execute_text;
    captured_command[0] = '\0';

    mainMenuScreen.init();

    global_exit_button = UI_FindFrame("ExitButton");
    exit_button = UI_FindChildFrame(UI_FindFrame("MainMenuFrame"), "ExitButton");
    if (!require_not_null(exit_button)) {
        uiimport = saved;
        return;
    }
    ASSERT(global_exit_button != exit_button);
    ASSERT(!exit_button->hidden);
    ASSERT_STR_EQ(exit_button->OnClick, "menu /main/quit-confirm");

    modal = UI_FindChildFrame(UI_FindFrame("MainMenuFrame"), "MainMenuQuitModal");
    if (!require_not_null(modal)) {
        uiimport = saved;
        return;
    }
    ASSERT_EQ_INT(modal->Type, FT_DIALOG);
    ASSERT(modal->hidden);

    cover = UI_FindChildFrame(modal, "MainMenuQuitModalCover");
    if (!require_not_null(cover)) {
        uiimport = saved;
        return;
    }
    ASSERT_EQ_INT(cover->Type, FT_TEXTURE);
    ASSERT(cover->Texture.Image != 0);
    ASSERT_EQ_INT(cover->Color.a, 128);

    dialog = UI_FindChildFrame(modal, "DialogWar3");
    if (!require_not_null(dialog)) {
        uiimport = saved;
        return;
    }
    ASSERT_EQ_INT(dialog->Type, FT_DIALOG);
    ASSERT(dialog->hidden);

    message = UI_FindChildFrame(dialog, "DialogText");
    icon = UI_FindChildFrame(dialog, "DialogIcon");
    ok_backdrop = UI_FindChildFrame(dialog, "DialogButtonOKBackdrop");
    no_backdrop = UI_FindChildFrame(dialog, "DialogButtonNoBackdrop");
    yes_backdrop = UI_FindChildFrame(dialog, "DialogButtonYesBackdrop");
    no_button = UI_FindChildFrame(dialog, "DialogButtonNo");
    yes_button = UI_FindChildFrame(dialog, "DialogButtonYes");

    if (!require_not_null(message)) {
        uiimport = saved;
        return;
    }
    if (!require_not_null(icon)) {
        uiimport = saved;
        return;
    }
    if (!require_not_null(ok_backdrop)) {
        uiimport = saved;
        return;
    }
    if (!require_not_null(no_backdrop)) {
        uiimport = saved;
        return;
    }
    if (!require_not_null(yes_backdrop)) {
        uiimport = saved;
        return;
    }
    if (!require_not_null(no_button)) {
        uiimport = saved;
        return;
    }
    if (!require_not_null(yes_button)) {
        uiimport = saved;
        return;
    }

    ASSERT_STR_EQ(message->Text, "Do you want to Quit?");
    ASSERT(icon->Backdrop.Background != 0);
    ASSERT(ok_backdrop->hidden);
    ASSERT(!no_backdrop->hidden);
    ASSERT(!yes_backdrop->hidden);
    ASSERT_STR_EQ(no_button->OnClick, "menu /main/main");
    ASSERT_STR_EQ(yes_button->OnClick, "menu /quit");

    mainMenuScreen.route("/quit-confirm");
    ASSERT(!modal->hidden);
    ASSERT(!dialog->hidden);

    UI_MenuCommandLocal(no_button->OnClick);
    ASSERT(modal->hidden);
    ASSERT(dialog->hidden);

    UI_MenuCommandLocal(yes_button->OnClick);
    ASSERT_STR_EQ(captured_command, "quit\n");

    uiimport = saved;
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
    RUN_TEST(test_collect_frame_tree_skips_editbox_text_frame);
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
    RUN_TEST(test_esc_menu_confirm_quit_panel_is_available);
    RUN_TEST(test_dialog_war3_supports_configurable_button_modes);
    RUN_TEST(test_main_menu_quit_dialog_routes_to_quit);
END_SUITE()
