#include "client.h"

int CL_ParseEntityBits(LPSIZEBUF buf, DWORD *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadShort(buf);
}

void CL_GetProjectionMatrix(LPMATRIX4 projectionMatrix) {
    VECTOR3 const vieworg = Vector3_unm(&cl.viewDef.vieworg);
    SIZE2 const windowSize = renderer->GetWindowSize();
    float const aspect = (float)windowSize.width / (float)windowSize.height;
    Matrix4_perspective(projectionMatrix, cl.viewDef.fov, aspect, 100.0, 100000.0);
    Matrix4_rotate(projectionMatrix, &cl.viewDef.viewangles, ROTATE_XYZ);
    Matrix4_translate(projectionMatrix, &vieworg);
}

LINE3 CL_GetMouseLine(DWORD pixelX, DWORD pixelY) {
    LINE3 line;
    MATRIX4 projectionMatrix;
    MATRIX4 inverseProjectionMatrix;
    SIZE2 const windowSize = renderer->GetWindowSize();
    float const x = ((float)pixelX / (float)windowSize.width - 0.5) * 2;
    float const y = (0.5 - (float)pixelY / (float)windowSize.height) * 2;
    CL_GetProjectionMatrix(&projectionMatrix);
    Matrix4_inverse(&projectionMatrix, &inverseProjectionMatrix);
    Matrix4_multiply_vector3(&inverseProjectionMatrix, &(VECTOR3) { x, y, 0 }, &line.a);
    Matrix4_multiply_vector3(&inverseProjectionMatrix, &(VECTOR3) { x, y, 1 }, &line.b);
    return line;
}

int selectedEntity = -1;
struct client_message msg = { CMD_NO_COMMAND };

short GetHeightMapValue(int x, int y);

bool CL_ProcessEntitySelection(clientEntity_t *entity) {
    if (entity->current.player == cl.playerNumber) {
        selectedEntity = entity->current.number;
        return true;
    }
    if (entity->current.player > 0 && selectedEntity != -1) {
        msg.targetentity = entity->current.number;
        msg.entity = selectedEntity;
        msg.cmd = CMD_ATTACK;
        return true;
    }
    return false;
}

void CL_SelectEntityAtScreenPoint(DWORD pixelX, DWORD pixelY) {
    LINE3 const line = CL_GetMouseLine(pixelX, pixelY);
    FOR_LOOP(entindex, cl.num_entities) {
        clientEntity_t *e = &cl.ents[entindex];
        if (e->current.player == 0) continue;
        if (Line3_intersect_sphere3(&line, &(const SPHERE3) {
            .center = e->current.origin,
            .radius = 100
        }, NULL) && CL_ProcessEntitySelection(e)) {
            return;
        }
    }
    if (selectedEntity == -1)
        return;
    VECTOR3 targetorg;
    if (CM_IntersectLineWithHeightmap(&line, &targetorg)) {
        msg.location.x = targetorg.x;
        msg.location.y = targetorg.y;
        msg.entity = selectedEntity;
        msg.cmd = CMD_MOVE;
    }
}
