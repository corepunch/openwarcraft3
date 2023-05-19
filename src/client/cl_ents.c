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

int selectedEntity = -1;
struct client_message msg = { CMD_NO_COMMAND };

short GetHeightMapValue(int x, int y);

VECTOR3 PointIntoHeightmap(LPWAR3MAP lpMap, LPCVECTOR3 lpPoint) {
    return (VECTOR3) {
        .x = (lpPoint->x - lpMap->center.x) / TILESIZE,
        .y = (lpPoint->y - lpMap->center.y) / TILESIZE,
        .z = lpPoint->z
    };
}

VECTOR3 PointFromHeightmap(LPWAR3MAP lpMap, LPCVECTOR3 lpPoint) {
    return (VECTOR3) {
        .x = lpPoint->x * TILESIZE + lpMap->center.x,
        .y = lpPoint->y * TILESIZE + lpMap->center.y,
        .z = lpPoint->z
    };
}

bool CL_GetClickPoint(LPCLINE3 lpLine, LPVECTOR3 lpOutput) {
    extern LPWAR3MAP lpMap;
    LINE3 line = {
        .a = PointIntoHeightmap(lpMap, &lpLine->a),
        .b = PointIntoHeightmap(lpMap, &lpLine->b),
    };
    FOR_LOOP(x, lpMap->width) {
        FOR_LOOP(y, lpMap->height) {
            TRIANGLE3 const tri1 = {
                { x, y, GetHeightMapValue(x, y) },
                { x+1, y, GetHeightMapValue(x+1, y) },
                { x+1, y+1, GetHeightMapValue(x, y+1) },
            };
            TRIANGLE3 const tri2 = {
                { x+1, y+1, GetHeightMapValue(x, y+1) },
                { x, y+1, GetHeightMapValue(x, y+1) },
                { x, y, GetHeightMapValue(x, y) },
            };
            if (Line3_intersect_triangle(&line, &tri1, lpOutput)) {
                *lpOutput = PointFromHeightmap(lpMap, lpOutput);
                return true;
            }
            if (Line3_intersect_triangle(&line, &tri2, lpOutput)) {
                *lpOutput = PointFromHeightmap(lpMap, lpOutput);
                return true;
            }
        }
    }
    return false;
}

void CL_SelectEntityAtScreenPoint(DWORD dwPixelX, DWORD dwPixelY) {
    LINE3 const line = CL_GetMouseLine(dwPixelX, dwPixelY);
    int clickedEntity = -1;
    FOR_LOOP(entindex, cl.num_entities) {
        struct client_entity *e = &cl.ents[entindex];
        if (Line3_intersect_sphere3(&line, &(const SPHERE3) {
            .center = e->current.origin,
            .radius = 100
        }, NULL)) {
            clickedEntity = e->current.number;
        }
    }
    if (clickedEntity != -1 && cl.ents[clickedEntity].current.team == cl.team) {
        selectedEntity = clickedEntity;
    } else if (selectedEntity != -1) {
        VECTOR3 targetorg;
//        Line3_intersect_plane3(&line, &(const PLANE3) {
//            .point = { 0, 0, 0 },
//            .normal = { 0, 0, 1 },
//        }, &targetorg);
        CL_GetClickPoint(&line, &targetorg);
        msg.location.x = targetorg.x;
        msg.location.y = targetorg.y;
        msg.entity = selectedEntity;
        msg.cmd = CMD_MOVE;
    }
}
