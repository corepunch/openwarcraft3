#ifndef triangle3_h
#define triangle3_h

#include "vector3.h"

struct triangle3 {
    VECTOR3 a;
    VECTOR3 b;
    VECTOR3 c;
};

typedef struct triangle3 TRIANGLE3;
typedef struct triangle3 *LPTRIANGLE3;
typedef struct triangle3 const *LPCTRIANGLE3;

VECTOR3 Triangle_normal(LPCTRIANGLE3 triangle);

#endif
