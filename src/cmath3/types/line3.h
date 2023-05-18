#ifndef line3_h
#define line3_h

#include "vector3.h"
#include "sphere3.h"

struct line3 {
    VECTOR3 a;
    VECTOR3 b;
};

typedef struct line3 LINE3;
typedef struct line3 *LPLINE3;
typedef struct line3 const *LPCLINE3;

int Line_intersect_sphere(LPCLINE3 lpLine, LPCSPHERE3 lpSphere, LPVECTOR3 lpOutput);

#endif
