#ifdef BZ_TESTS
#include "../g_local.h"
#include "shared/test.h"

BOOL run_test_jass(LPCSTR src);

static Doodads_t doodad_row = { .id = MAKEFOURCC('L', 'O', 'o', '2') };
static DestructableData_t not_destructable;

static LPEDICT make_test_doodad(FLOAT x, FLOAT y) {
    LPEDICT ent = G_Spawn();

    ent->class_id = doodad_row.id;
    ent->s.class_id = ent->class_id;
    ent->s.origin2 = (VECTOR2){ x, y };
    ent->s.origin.x = x;
    ent->s.origin.y = y;
    ent->data.Doodads = &doodad_row;
    ent->data.DestructableData = &not_destructable;
    ent->svflags |= SVF_STATIC_SCENERY;
    return ent;
}

TEST(wc3_doodad, rect_special_hide_only_changes_matching_doodads) {
    BOX2 rect = { .min = { 0, 0 }, .max = { 128, 128 } };
    LPEDICT inside = make_test_doodad(64, 64);
    LPEDICT outside = make_test_doodad(256, 64);

    T_EQ(G_SetDoodadAnimationRect(&rect, doodad_row.id, "hide", false), 1);
    T_ASSERT(inside->s.renderfx & RF_HIDDEN);
    T_ASSERT(!(outside->s.renderfx & RF_HIDDEN));

    T_EQ(G_SetDoodadAnimationRect(&rect, doodad_row.id, "show", false), 1);
    T_ASSERT(!(inside->s.renderfx & RF_HIDDEN));
}

TEST(wc3_doodad, radius_nearest_only_changes_one_doodad) {
    LPEDICT near = make_test_doodad(32, 0);
    LPEDICT far = make_test_doodad(96, 0);

    T_EQ(G_SetDoodadAnimationRadius(0, 0, 128, doodad_row.id, true, "hide", false), 1);
    T_ASSERT(near->s.renderfx & RF_HIDDEN);
    T_ASSERT(!(far->s.renderfx & RF_HIDDEN));
}

TEST(wc3_doodad, jass_natives_route_radius_and_rect_to_map_doodads) {
    LPEDICT radius_match = make_test_doodad(32, 32);
    LPEDICT rect_match = make_test_doodad(192, 192);

    T_ASSERT(run_test_jass(
        "function main takes nothing returns nothing\n"
        "  local rect r = Rect(128.0, 128.0, 256.0, 256.0)\n"
        "  call SetDoodadAnimation(0.0, 0.0, 64.0, 'LOo2', true, \"hide\", false)\n"
        "  call SetDoodadAnimationRect(r, 'LOo2', \"hide\", false)\n"
        "endfunction\n"));
    T_ASSERT(radius_match->s.renderfx & RF_HIDDEN);
    T_ASSERT(rect_match->s.renderfx & RF_HIDDEN);
}

TEST(wc3_doodad, nonlooping_animation_holds_authored_final_frame) {
    LPEDICT ent = make_test_doodad(0, 0);
    animation_t death = { .name = "Death", .interval = { 1000, 1300 }, .flags = 1 };
    umove_t move = { "death", NULL, G_DoodadAnimationEnd };

    ent->animation = &death;
    ent->currentmove = &move;
    ent->s.frame = 1200;
    ent->aiflags &= ~AI_HOLD_FRAME;

    M_MoveFrame(ent);

    T_EQ(ent->s.frame, 1299);
    T_ASSERT(ent->aiflags & AI_HOLD_FRAME);
}

TEST(wc3_doodad, looping_animation_wraps_to_sequence_start) {
    LPEDICT ent = make_test_doodad(0, 0);
    animation_t stand = { .name = "Stand", .interval = { 1000, 1300 }, .flags = 0 };
    umove_t move = { "stand", NULL, G_DoodadAnimationEnd };

    ent->animation = &stand;
    ent->currentmove = &move;
    ent->s.frame = 1200;
    ent->aiflags &= ~AI_HOLD_FRAME;

    M_MoveFrame(ent);

    T_EQ(ent->s.frame, 1000);
    T_ASSERT(!(ent->aiflags & AI_HOLD_FRAME));
}
#endif
