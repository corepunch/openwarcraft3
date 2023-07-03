#ifndef plane3_h
#define plane3_h

#include "vector3.h"

struct plane3 {
    union {
        struct { VECTOR3 normal; float distance; };
        struct { float a, b, c, d; };
        float v[4];
    };
};

typedef struct plane3 PLANE3;
typedef struct plane3 *LPPLANE3;
typedef struct plane3 const *LPCPLANE3;

void Plane3_Normalize(LPPLANE3 plane);
float Plane3_MultiplyVector3(LPCPLANE3 plane, LPCVECTOR3 point);

#endif
