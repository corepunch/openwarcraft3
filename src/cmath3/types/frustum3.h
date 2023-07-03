#ifndef frustum3_h
#define frustum3_h

#include "plane3.h"
#include "sphere3.h"
#include "box3.h"

enum {
    FRUSTUM_LEFT,
    FRUSTUM_RIGHT,
    FRUSTUM_BOTTOM,
    FRUSTUM_TOP,
    FRUSTUM_BACK,
    FRUSTUM_FRONT,
    FRUSTUM_NUM_PLANES
};

struct frustum3 {
    union {
        struct { PLANE3 left, right, bottom, top, front, back; };
        PLANE3 planes[FRUSTUM_NUM_PLANES];
    };
};

typedef struct frustum3 FRUSTUM3;
typedef struct frustum3 *LPFRUSTUM3;
typedef struct frustum3 const *LPCFRUSTUM3;

void Frustum_Calculate(LPCMATRIX4 matrix, LPFRUSTUM3 output);
int Frustum_ContainsPoint(LPCFRUSTUM3 frustum, LPCVECTOR3 point);
int Frustum_ContainsSphere(LPCFRUSTUM3 frustum, LPCSPHERE3 sphere);
int Frustum_ContainsBox(LPCFRUSTUM3 frustum, LPCBOX3 box, LPCMATRIX4 matrix);
int Frustum_ContainsAABox(LPCFRUSTUM3 frustum, LPCBOX3 box);

#endif
