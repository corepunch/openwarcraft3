#include "client.h"

int CL_ParseEntityBits(LPSIZEBUF buf, DWORD *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadShort(buf);
}

void CL_GetProjectionMatrix(LPMATRIX4 lpProjectionMatrix) {
    SIZE2 const windowSize = renderer->GetWindowSize();
    float const aspect = (float)windowSize.width / (float)windowSize.height;
    Matrix4_perspective(lpProjectionMatrix, cl.viewDef.fov, aspect, 100.0, 100000.0);
    Matrix4_rotate(lpProjectionMatrix, &cl.viewDef.viewangles, ROTATE_XYZ);
    Matrix4_translate(lpProjectionMatrix, &cl.viewDef.vieworg);
}

LINE3 CL_GetMouseLine(DWORD dwPixelX, DWORD dwPixelY) {
    LINE3 line;
    MATRIX4 projectionMatrix, inverseProjectionMatrix;
    SIZE2 const windowSize = renderer->GetWindowSize();
    float const x = ((float)dwPixelX / (float)windowSize.width - 0.5) * 2;
    float const y = (0.5 - (float)dwPixelY / (float)windowSize.height) * 2;
    CL_GetProjectionMatrix(&projectionMatrix);
    Matrix4_inverse(&projectionMatrix, &inverseProjectionMatrix);
    Matrix4_multiply_vector3(&inverseProjectionMatrix, &(VECTOR3) { x, y, 0 }, &line.a);
    Matrix4_multiply_vector3(&inverseProjectionMatrix, &(VECTOR3) { x, y, 1 }, &line.b);
    return line;
}

int selected_entity = -1;

void CL_SelectEntityAtScreenPoint(DWORD dwPixelX, DWORD dwPixelY) {
    LINE3 const line = CL_GetMouseLine(dwPixelX, dwPixelY);
    FOR_LOOP(entindex, cl.num_entities) {
        struct client_entity *clent = &cl.ents[entindex];
        if (Line_intersect_sphere(&line, &(const SPHERE3) {
            .center = clent->current.origin,
            .radius = 100
        }, NULL)) {
            selected_entity = clent->current.number;
        }
    }
}
