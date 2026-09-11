#include "test.h"
#include "renderer/r_local.h"
#include "renderer/r_game.h"

size2_t R_GetWindowSize(void) { return (size2_t){ 1024, 768 }; }
bool R_TraceModel(renderEntity_t const *ent, LPCLINE3 line, LPFLOAT distance) {
    (void)ent; (void)line; (void)distance; return false;
}

/* Exact snapshot terrain can change while the rendered target stays above a narrow depression. */
TEST(renderer_view, pan_plane_uses_rendered_target) {
    viewDef_t view = { .viewport = { 0, 0.22f, 1, 0.76f }, .target = { 0, 0, 10 } };
    MATRIX4 proj, camera;
    VECTOR3 point, eye = { 0, -10, 20 }, dir = { 0, 10, -10 };
    Matrix4_lookAt(&camera, &eye, &dir, &(VECTOR3){ 0, 0, 1 });
    Matrix4_perspective(&proj, 60, 4.0f / 3.0f, 1, 1000);
    Matrix4_multiply(&proj, &camera, &view.viewProjectionMatrix);
    FOR_LOOP(i, 3) {
        view.camerastate[0].origin.z = i * 10.0f;
        T_ASSERT(R_TraceCameraPlane(&view, 512, 307.2f, &point));
        T_FEQ(point.x, 0, 0.001f); T_FEQ(point.y, 0, 0.001f); T_FEQ(point.z, 10, 0.001f);
    }
    view.target.z = 15;
    T_ASSERT(R_TraceCameraPlane(&view, 512, 307.2f, &point));
    T_FEQ(point.z, 15, 0.001f);
    T_ASSERT(!R_TraceCameraPlane(NULL, 512, 307.2f, &point));
    T_ASSERT(!R_TraceCameraPlane(&view, 512, 307.2f, NULL));
}
