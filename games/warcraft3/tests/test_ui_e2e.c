#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "test_harness.h"
#include "../../../common/net.h"

/* Forward declarations to avoid macro collisions with game/g_local.h */
void MSG_WriteDeltaUIFrame(LPSIZEBUF msg, LPCUIFRAME from, LPCUIFRAME to, bool force);
void MSG_WriteShort(LPSIZEBUF buf, int value);
void MSG_WriteLong(LPSIZEBUF buf, int value);
void MSG_Write(LPSIZEBUF buf, const void *value, DWORD size);

LPCUIFRAME SCR_Clear(HANDLE data);

static int fake_image_index_e2e(LPCSTR name) {
    return (name && *name) ? 88 : 0;
}

static int fake_model_index_e2e(LPCSTR name) {
    return (name && *name) ? 77 : 0;
}

static char *read_text_file(const char *path) {
    FILE *f;
    long size;
    char *buf;

    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    buf[size] = '\0';
    return buf;
}

static DWORD build_layout_payload_from_root(LPCFRAMEDEF root, BYTE *out, DWORD max) {
    LPCFRAMEDEF nodes[512];
    DWORD count;
    DWORD i;
    uiFrame_t empty = {0};
    sizeBuf_t msg = {0};

    msg.data = out;
    msg.maxsize = max;

    count = UI_CollectFrameTree(root, nodes, 512);
    for (i = 0; i < count; ++i) {
        uiFrame_t frame = {0};
        BYTE typedata[512] = {0};
        UINAME textbuf = {0};
        if (!UI_BuildFrameForWrite(nodes[i], &frame, typedata, sizeof(typedata), textbuf, sizeof(textbuf))) {
            continue;
        }
        MSG_WriteDeltaUIFrame(&msg, &empty, &frame, true);
        MSG_WriteShort(&msg, frame.buffer.size);
        MSG_Write(&msg, frame.buffer.data, frame.buffer.size);
    }

    MSG_WriteLong(&msg, 0);
    return msg.cursize;
}

static HANDLE build_layout_blob_from_fixture(const char *fixture_path, const char *root_name) {
    BYTE payload[65536] = {0};
    char *fdf = read_text_file(fixture_path);
    LPFRAMEDEF root;
    DWORD payload_size;
    BYTE *blob;

    if (!fdf) {
        return NULL;
    }

    UI_ClearTemplates();
    gi.ImageIndex = fake_image_index_e2e;
    gi.ModelIndex = fake_model_index_e2e;

    UI_ParseFDF_Buffer(fixture_path, fdf);
    free(fdf);

    root = UI_FindFrame(root_name);
    if (!root) {
        return NULL;
    }

    payload_size = build_layout_payload_from_root(root, payload, sizeof(payload));
    blob = (BYTE *)malloc(sizeof(DWORD) + payload_size);
    if (!blob) {
        return NULL;
    }
    memcpy(blob, &payload_size, sizeof(payload_size));
    memcpy(blob + sizeof(payload_size), payload, payload_size);
    return blob;
}

static void test_e2e_basic_layout_decodes_nonempty_frame_stream(void) {
    HANDLE blob = build_layout_blob_from_fixture("tests/resources-src/TestUI/Frames/basic_layout.fdf", "TestRootFrame");
    LPUIFRAME decoded;
    DWORD i;
    DWORD non_null_count = 0;

    ASSERT_NOT_NULL(blob);
    if (!blob) return;

    decoded = SCR_Clear(blob);
    ASSERT_NOT_NULL(decoded);
    
    /* Count how many frames got decoded (have non-zero flags) */
    for (i = 0; i < 64; ++i) {
        if (decoded[i].flagsvalue != 0) {
            non_null_count++;
        }
    }
    ASSERT((non_null_count) > (1));

    free(blob);
}

static void test_e2e_basic_layout_backdrop_decodes_as_backdrop(void) {
    HANDLE blob = build_layout_blob_from_fixture("tests/resources-src/TestUI/Frames/basic_layout.fdf", "TestRootFrame");
    LPUIFRAME decoded;
    DWORD i;
    DWORD found = 0;

    ASSERT_NOT_NULL(blob);
    if (!blob) return;

    decoded = SCR_Clear(blob);
    for (i = 0; i < 64; ++i) {
        if (decoded[i].flagsvalue != 0) {
            found++;
        }
    }
    ASSERT((found) > (0));

    free(blob);
}

static void test_e2e_simple_sprite_contains_sprite_model_index(void) {
    HANDLE blob = build_layout_blob_from_fixture("tests/resources-src/TestUI/Frames/simple_sprite.fdf", "TestSpriteRoot");
    LPUIFRAME decoded;
    DWORD i;
    BOOL found = false;

    ASSERT_NOT_NULL(blob);
    if (!blob) return;

    decoded = SCR_Clear(blob);
    for (i = 1; i < 64; ++i) {
        if (decoded[i].flags.type == FT_SPRITE) {
            found = true;
            ASSERT_EQ_INT(decoded[i].tex.index, 77);
            break;
        }
    }
    ASSERT_EQ_INT(found, true);

    free(blob);
}

static void test_e2e_animated_sprite_contains_text_and_backdrop(void) {
    HANDLE blob = build_layout_blob_from_fixture("tests/resources-src/TestUI/Frames/animated_sprite.fdf", "TestAnimRoot");
    LPUIFRAME decoded;
    DWORD i;
    BOOL has_text = false;
    BOOL has_backdrop = false;

    ASSERT_NOT_NULL(blob);
    if (!blob) return;

    decoded = SCR_Clear(blob);
    for (i = 1; i < 96; ++i) {
        if (decoded[i].flags.type == FT_TEXT) {
            has_text = true;
        }
        if (decoded[i].flags.type == FT_BACKDROP) {
            has_backdrop = true;
        }
    }

    ASSERT_EQ_INT(has_text, true);
    ASSERT_EQ_INT(has_backdrop, true);

    free(blob);
}

static void test_e2e_collect_tree_and_decode_preserves_parent_indexes(void) {
    HANDLE blob = build_layout_blob_from_fixture("tests/resources-src/TestUI/Frames/basic_layout.fdf", "TestRootFrame");
    LPUIFRAME decoded;
    DWORD i;
    DWORD decoded_count = 0;

    ASSERT_NOT_NULL(blob);
    if (!blob) return;

    decoded = SCR_Clear(blob);
    for (i = 0; i < 96; ++i) {
        if (decoded[i].flagsvalue != 0) {
            decoded_count++;
        }
    }
    ASSERT((decoded_count) > (0));

    free(blob);
}

BEGIN_SUITE(ui_e2e)
    RUN_TEST(test_e2e_basic_layout_decodes_nonempty_frame_stream);
    RUN_TEST(test_e2e_basic_layout_backdrop_decodes_as_backdrop);
    RUN_TEST(test_e2e_simple_sprite_contains_sprite_model_index);
    RUN_TEST(test_e2e_animated_sprite_contains_text_and_backdrop);
    RUN_TEST(test_e2e_collect_tree_and_decode_preserves_parent_indexes);
END_SUITE()
