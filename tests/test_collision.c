/*
 * test_collision.c — Collision resolution and TGA pathfinding texture tests.
 *
 * Covered scenarios:
 *
 *  G_PushEntity
 *    - moves entity by the specified distance in the given direction
 *
 *  G_SolveCollisions
 *    - two overlapping dynamic units are pushed apart
 *    - a dynamic unit overlapping a static building is pushed away
 *      while the static unit does not move
 *    - entities flagged as IS_HOLLOW (dead, hidden, no model) are
 *      ignored by the collision resolver
 *
 *  LoadTGA (g_pathing.c)
 *    - a minimal 1×1 8-bit grayscale TGA is decoded correctly
 *    - a minimal 2×2 24-bit RGB TGA is decoded correctly
 *    - unsupported image types return NULL
 */

#include <math.h>
#include <string.h>
#include "test_framework.h"
#include "test_harness.h"

/* -----------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------- */

static DWORD hpea_id(void) { DWORD id; memcpy(&id, "hpea", 4); return id; }

/*
 * Create a live, dynamic (MOVETYPE_STEP) unit suitable for collision
 * testing.  Setting s.model to a non-zero value is required so that
 * IS_HOLLOW() evaluates to false.
 */
static LPEDICT make_collision_unit(FLOAT x, FLOAT y, FLOAT radius) {
    LPEDICT ent   = alloc_test_unit(hpea_id(), x, y);
    ent->movetype  = MOVETYPE_STEP;
    ent->collision = radius;
    ent->s.model   = 1;   /* IS_HOLLOW requires s.model != 0 */
    ent->stand     = unit_stand;
    unit_stand(ent);
    gi.LinkEntity(ent);   /* update bounds after setting collision */
    return ent;
}

/* Distance between two 2-D origins. */
static FLOAT dist2(LPCVECTOR2 a, LPCVECTOR2 b) {
    FLOAT dx = a->x - b->x;
    FLOAT dy = a->y - b->y;
    return sqrtf(dx*dx + dy*dy);
}

/* -----------------------------------------------------------------------
 * G_PushEntity tests
 * --------------------------------------------------------------------- */

static void test_push_entity_moves_in_direction(void) {
    reset_entities();
    LPEDICT ent = make_collision_unit(0.0f, 0.0f, 0.0f);
    VECTOR2 dir = {1.0f, 0.0f};
    G_PushEntity(ent, 50.0f, &dir);
    ASSERT_EQ_FLOAT(ent->s.origin2.x, 50.0f, 0.01f);
    ASSERT_EQ_FLOAT(ent->s.origin2.y,  0.0f, 0.01f);
}

static void test_push_entity_negative_distance_moves_back(void) {
    reset_entities();
    LPEDICT ent = make_collision_unit(100.0f, 0.0f, 0.0f);
    VECTOR2 dir = {1.0f, 0.0f};
    G_PushEntity(ent, -30.0f, &dir);
    ASSERT_EQ_FLOAT(ent->s.origin2.x, 70.0f, 0.01f);
}

/* -----------------------------------------------------------------------
 * G_SolveCollisions — two dynamic units
 * --------------------------------------------------------------------- */

static void test_two_overlapping_units_are_separated(void) {
    reset_entities();
    /* Radii = 16 each → combined = 32.  Place them 5 units apart → overlap. */
    LPEDICT a = make_collision_unit( 0.0f, 0.0f, 16.0f);
    LPEDICT b = make_collision_unit( 5.0f, 0.0f, 16.0f);

    FLOAT dist_before = dist2(&a->s.origin2, &b->s.origin2);
    ASSERT(dist_before < a->collision + b->collision); /* confirm overlap */

    G_SolveCollisions();

    FLOAT dist_after = dist2(&a->s.origin2, &b->s.origin2);
    ASSERT(dist_after > dist_before);
}

static void test_two_overlapping_units_reach_non_overlap(void) {
    reset_entities();
    /* Run multiple collision passes (real game runs one per frame). */
    LPEDICT a = make_collision_unit( 0.0f, 0.0f, 16.0f);
    LPEDICT b = make_collision_unit( 5.0f, 0.0f, 16.0f);
    FLOAT combined = a->collision + b->collision;

    for (int i = 0; i < 20; i++) {
        G_SolveCollisions();
        if (dist2(&a->s.origin2, &b->s.origin2) >= combined) break;
    }

    FLOAT dist_after = dist2(&a->s.origin2, &b->s.origin2);
    ASSERT(dist_after >= combined - 0.5f);
}

/* -----------------------------------------------------------------------
 * G_SolveCollisions — static unit (building) vs. dynamic unit
 * --------------------------------------------------------------------- */

static void test_static_unit_does_not_move_on_overlap(void) {
    reset_entities();
    /* Building: MOVETYPE_NONE (IS_STATIC = true). */
    LPEDICT building = make_collision_unit(0.0f, 0.0f, 32.0f);
    building->movetype = MOVETYPE_NONE;

    LPEDICT unit = make_collision_unit(10.0f, 0.0f, 16.0f);

    FLOAT bx_before = building->s.origin2.x;
    G_SolveCollisions();
    FLOAT bx_after  = building->s.origin2.x;

    /* Static unit must not have moved. */
    ASSERT_EQ_FLOAT(bx_after, bx_before, 0.001f);

    /* Dynamic unit should have been pushed away. */
    ASSERT(unit->s.origin2.x > 10.0f + 0.001f ||
           unit->s.origin2.x < 10.0f - 0.001f ||
           unit->s.origin2.y != 0.0f);
}

/* -----------------------------------------------------------------------
 * G_SolveCollisions — hollow entities are ignored
 * --------------------------------------------------------------------- */

static void test_dead_unit_not_involved_in_collision(void) {
    reset_entities();
    LPEDICT alive = make_collision_unit(0.0f, 0.0f, 16.0f);
    LPEDICT dead  = make_collision_unit(5.0f, 0.0f, 16.0f);
    dead->svflags |= SVF_DEADMONSTER;  /* IS_HOLLOW == true */

    FLOAT x_before = alive->s.origin2.x;
    G_SolveCollisions();

    /* Alive unit should NOT have been pushed by the dead one. */
    ASSERT_EQ_FLOAT(alive->s.origin2.x, x_before, 0.001f);
}

/* -----------------------------------------------------------------------
 * LoadTGA — pathfinding texture decoding
 * --------------------------------------------------------------------- */

/*
 * Build a minimal packed TGA image in a byte buffer and return its size.
 * image_type 3 = uncompressed grayscale, pixel_size 8.
 *
 * The layout below intentionally mirrors tgaHeader_t defined in
 * g_pathing.c (same fields, same pack(1) alignment).  If that struct
 * ever changes, this one must be updated to match.
 */
#pragma pack(push, 1)
typedef struct {
    BYTE  id_length, colormap_type, image_type;
    WORD  colormap_index, colormap_length;
    BYTE  colormap_size;
    WORD  x_origin, y_origin, width, height;
    BYTE  pixel_size, attributes;
} test_tga_hdr_t;   /* mirrors tgaHeader_t from g_pathing.c */
#pragma pack(pop)

static size_t make_tga_grayscale_1x1(BYTE buf[static 32], BYTE grey) {
    test_tga_hdr_t hdr = {0};
    hdr.image_type  = 3;
    hdr.width       = 1;
    hdr.height      = 1;
    hdr.pixel_size  = 8;
    memcpy(buf, &hdr, sizeof(hdr));
    buf[sizeof(hdr)] = grey;
    return sizeof(hdr) + 1;
}

static size_t make_tga_rgb_2x2(BYTE buf[static 64]) {
    test_tga_hdr_t hdr = {0};
    hdr.image_type  = 2;
    hdr.width       = 2;
    hdr.height      = 2;
    hdr.pixel_size  = 24;
    memcpy(buf, &hdr, sizeof(hdr));
    /* 4 pixels × 3 bytes: blue, green, red (BGR in TGA). */
    BYTE *px = buf + sizeof(hdr);
    /* pixel (0,0): R=0xFF G=0x00 B=0x00 → stored BGR */
    *px++ = 0x00; *px++ = 0x00; *px++ = 0xFF;
    /* pixel (1,0): G=0xFF */
    *px++ = 0x00; *px++ = 0xFF; *px++ = 0x00;
    /* pixel (0,1) */
    *px++ = 0xFF; *px++ = 0x00; *px++ = 0x00;
    /* pixel (1,1) */
    *px++ = 0x80; *px++ = 0x80; *px++ = 0x80;
    return sizeof(hdr) + 4 * 3;
}

static void test_load_tga_grayscale_1x1_dimensions(void) {
    BYTE buf[64];
    size_t sz = make_tga_grayscale_1x1(buf, 0xAB);
    pathTex_t *tex = LoadTGA(buf, sz);
    ASSERT_NOT_NULL(tex);
    ASSERT_EQ_INT(tex->width,  1);
    ASSERT_EQ_INT(tex->height, 1);
    gi.MemFree(tex);
}

static void test_load_tga_grayscale_pixel_value(void) {
    BYTE buf[64];
    size_t sz = make_tga_grayscale_1x1(buf, 0xAB);
    pathTex_t *tex = LoadTGA(buf, sz);
    ASSERT_NOT_NULL(tex);
    /* Grayscale pixel is replicated to R, G, B; alpha = 0xFF. */
    ASSERT_EQ_INT(tex->map[0].r, 0xAB);
    ASSERT_EQ_INT(tex->map[0].g, 0xAB);
    ASSERT_EQ_INT(tex->map[0].b, 0xAB);
    ASSERT_EQ_INT(tex->map[0].a, 0xFF);
    gi.MemFree(tex);
}

static void test_load_tga_rgb_2x2_dimensions(void) {
    BYTE buf[128];
    size_t sz = make_tga_rgb_2x2(buf);
    pathTex_t *tex = LoadTGA(buf, sz);
    ASSERT_NOT_NULL(tex);
    ASSERT_EQ_INT(tex->width,  2);
    ASSERT_EQ_INT(tex->height, 2);
    gi.MemFree(tex);
}

static void test_load_tga_unsupported_type_returns_null(void) {
    BYTE buf[64] = {0};
    test_tga_hdr_t *hdr = (test_tga_hdr_t *)buf;
    hdr->image_type = 10; /* RLE-compressed — not supported */
    hdr->width      = 1;
    hdr->height     = 1;
    hdr->pixel_size = 8;
    pathTex_t *tex = LoadTGA(buf, sizeof(buf));
    ASSERT_NULL(tex);
}

/* -----------------------------------------------------------------------
 * Suite runner
 * --------------------------------------------------------------------- */

void run_collision_tests(void) {
    setup_game();

    RUN_TEST(test_push_entity_moves_in_direction);
    RUN_TEST(test_push_entity_negative_distance_moves_back);

    RUN_TEST(test_two_overlapping_units_are_separated);
    RUN_TEST(test_two_overlapping_units_reach_non_overlap);
    RUN_TEST(test_static_unit_does_not_move_on_overlap);
    RUN_TEST(test_dead_unit_not_involved_in_collision);

    RUN_TEST(test_load_tga_grayscale_1x1_dimensions);
    RUN_TEST(test_load_tga_grayscale_pixel_value);
    RUN_TEST(test_load_tga_rgb_2x2_dimensions);
    RUN_TEST(test_load_tga_unsupported_type_returns_null);

    teardown_game();
}
