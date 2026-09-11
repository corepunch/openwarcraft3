#include "renderer/r_game.h"
#include "games/starcraft-2/renderer/m3/r_m3.h"
#include "shared/test.h"

/* The ray must follow model rotation/scale and return world-space distance for nearest-hit sorting. */
TEST(sc2_control, m3_picking) {
    m3Model_t m3 = { .boundings = { .min = {-1, -0.5f, 0}, .max = {1, 0.5f, 2} } };
    model_t model = { .modeltype = ID_43DM, .m3 = &m3 };
    renderEntity_t ent = { .model = &model, .origin = {10, 20, 8}, .scale = 2, .angle = M_PI / 2 };
    LINE3 ray = {{10, 21.5f, 20}, {10, 21.5f, 0}};
    FLOAT dist;
    BOX3 box;
    T_ASSERT(R_GetEntityBounds(&ent, &box)); T_FEQ(box.max.z, 2, 0.001f);
    T_ASSERT(R_TraceModel(&ent, &ray, &dist)); T_FEQ(dist, 8, 0.001f);
    ray.a.x = ray.b.x = 11.5f;
    T_ASSERT(!R_TraceModel(&ent, &ray, &dist));
    ent.model = NULL;
    T_ASSERT(!R_GetEntityBounds(&ent, &box)); T_ASSERT(!R_TraceModel(&ent, &ray, &dist));
}

/* Exercise the SC2 executable's wire schema, including the radius consumed by its renderer. */
TEST(sc2_control, fractional_snapshot_geometry) {
    entityState_t from = {0}, to = { .number = 1, .model = 1, .origin = {35.275f, 23.625f, 8.125f},
        .radius = 0.375f, .renderfx = RF_SELECTED }, out = {0};
    FOR_LOOP(i, 2) {
        BYTE buf[256];
        sizeBuf_t msg = { .data = buf, .maxsize = sizeof(buf) };
        DWORD bits = 0;
        MSG_WriteDeltaEntity(&msg, &from, &to, true);
        int num = MSG_ReadEntityBits(&msg, &bits);
        MSG_ReadDeltaEntity(&msg, &out, num, bits);
        T_FEQ(out.origin.x, to.origin.x, 0.00001f);
        T_FEQ(out.origin.y, to.origin.y, 0.00001f);
        T_FEQ(out.origin.z, to.origin.z, 0.00001f);
        renderEntity_t ent = { .radius = out.radius };
        T_FEQ(R_SelectionRadius(&ent), 0.375f, 0.00001f);
        T_ASSERT(out.renderfx & RF_SELECTED);
        from = to; to.origin.x += 0.125f;
    }
}
