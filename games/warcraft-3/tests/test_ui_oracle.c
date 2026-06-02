#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_framework.h"
#include "test_harness.h"

#define ORACLE_OUT_MAX (256 * 1024)

static void ensure_test_assets(void) {
    static BOOL ready = false;
    FILE *f;

    if (ready) {
        return;
    }

    f = fopen("build/tests/tests.mpq", "rb");
    if (!f) {
        int rc = system("make -s test-assets > /tmp/openwarcraft3-test-assets.log 2>&1");
        ASSERT_EQ_INT(rc, 0);
        f = fopen("build/tests/tests.mpq", "rb");
        ASSERT_NOT_NULL(f);
        if (!f) {
            return;
        }
    }

    fclose(f);
    ready = true;
}

static char *run_capture(const char *cmd) {
    FILE *pipe;
    char *out;
    size_t used = 0;

    out = (char *)malloc(ORACLE_OUT_MAX);
    if (!out) {
        return NULL;
    }
    out[0] = '\0';

    pipe = popen(cmd, "r");
    if (!pipe) {
        free(out);
        return NULL;
    }

    while (used + 1 < ORACLE_OUT_MAX) {
        size_t n = fread(out + used, 1, ORACLE_OUT_MAX - used - 1, pipe);
        if (n == 0) {
            break;
        }
        used += n;
    }
    out[used] = '\0';

    if (pclose(pipe) != 0) {
        free(out);
        return NULL;
    }

    return out;
}

static void assert_contains(const char *haystack, const char *needle) {
    ASSERT_NOT_NULL(haystack);
    ASSERT_NOT_NULL(needle);
    ASSERT(strstr(haystack, needle) != NULL);
}

static void test_oracle_fdftool_basic_layout_summary(void) {
    char *out;

    ensure_test_assets();
    out = run_capture("build/bin/fdftool -mpq build/tests/tests.mpq -fdf TestUI/Frames/basic_layout.fdf --info 2>&1");
    ASSERT_NOT_NULL(out);
    if (!out) {
        return;
    }

    assert_contains(out, "fdf.files=1");
    assert_contains(out, "fdf.templates=9");
    assert_contains(out, "- TestBackdropBasic [BACKDROP]");
    assert_contains(out, "- TestLabel [TEXT]");

    free(out);
}

static void test_oracle_fdftool_simple_sprite_summary(void) {
    char *out;

    ensure_test_assets();
    out = run_capture("build/bin/fdftool -mpq build/tests/tests.mpq -fdf TestUI/Frames/simple_sprite.fdf --info 2>&1");
    ASSERT_NOT_NULL(out);
    if (!out) {
        return;
    }

    assert_contains(out, "fdf.files=1");
    assert_contains(out, "fdf.templates=5");
    assert_contains(out, "- TestSpriteRoot [FRAME]");
    assert_contains(out, "- TestQuadSprite [SPRITE]");

    free(out);
}

static void test_oracle_mdxtool_quad_info_counts(void) {
    char *out;

    ensure_test_assets();
    out = run_capture("build/bin/mdxtool -mpq build/tests/tests.mpq -model TestUI/Models/quad_sprite.mdx --info 2>&1");
    ASSERT_NOT_NULL(out);
    if (!out) {
        return;
    }

    assert_contains(out, "mdxtool --info: model=TestUI/Models/quad_sprite.mdx");
    assert_contains(out, "MODL: name=QuadSprite");
    assert_contains(out, "SEQS: count=1");
    assert_contains(out, "TEXS: count=1");
    assert_contains(out, "GEOS: count=1");
    assert_contains(out, "BONE: count=1");
    assert_contains(out, "PIVT: count=1");

    free(out);
}

static void test_oracle_mdxtool_anim_info_counts(void) {
    char *out;

    ensure_test_assets();
    out = run_capture("build/bin/mdxtool -mpq build/tests/tests.mpq -model TestUI/Models/anim_pulse.mdx --info 2>&1");
    ASSERT_NOT_NULL(out);
    if (!out) {
        return;
    }

    assert_contains(out, "mdxtool --info: model=TestUI/Models/anim_pulse.mdx");
    assert_contains(out, "MODL: name=AnimPulse");
    assert_contains(out, "SEQS: count=2");
    assert_contains(out, "name=Stand");
    assert_contains(out, "name=Birth");
    assert_contains(out, "TEXS: count=1");
    assert_contains(out, "GEOS: count=1");
    assert_contains(out, "BONE: count=1");
    assert_contains(out, "PIVT: count=1");

    free(out);
}

BEGIN_SUITE(ui_oracle)
    RUN_TEST(test_oracle_fdftool_basic_layout_summary);
    RUN_TEST(test_oracle_fdftool_simple_sprite_summary);
    RUN_TEST(test_oracle_mdxtool_quad_info_counts);
    RUN_TEST(test_oracle_mdxtool_anim_info_counts);
END_SUITE()
