#ifndef matrix3_h
#define matrix3_h

#include "vector3.h"

struct matrix3 {
    union {
        float v[9];
        VECTOR3 column[3];
    };
};

typedef struct matrix3 MATRIX3;
typedef struct matrix3 *LPMATRIX3;
typedef struct matrix3 const *LPCMATRIX3;
typedef struct matrix4 const *LPCMATRIX4;

void Matrix3_normal(LPMATRIX3 out, LPCMATRIX4 modelview);

#endif
