#ifndef WOW_COORDS_H
#define WOW_COORDS_H

#include "common/cmodel.h"

typedef struct {
    VECTOR3 pos, rot; /* Raw MODF placement coordinates and degrees, Y-up. */
    WORD scale;      /* MODF fixed point: 1024 = unity, zero = unspecified/unity. */
} WOWPLACEMENT;
typedef WOWPLACEMENT *LPWOWPLACEMENT;
typedef WOWPLACEMENT const *LPCWOWPLACEMENT;

/* WMO/M2 vertices stay in native Z-up model space. Convert only their ADT placement, once,
 * identically for collision and rendering. B*Ry(y-270)*Rz(-x)*Rx(z-90) = Rz(y+180)*Ry(x)*Rx(z). */
static void Wow_PlacementMatrix(LPCWOWPLACEMENT def, LPMATRIX4 matrix) {
    VECTOR3 pos = CM_WowObjectPoint(def->pos.x, def->pos.y, def->pos.z);
    FLOAT scale = def->scale ? def->scale / 1024.0f : 1.0f;
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &pos);
    Matrix4_rotate(matrix, &(VECTOR3){ def->rot.z, def->rot.x, def->rot.y + 180.0f }, ROTATE_XYZ);
    Matrix4_scale(matrix, &(VECTOR3){ scale, scale, scale });
}

#endif
