#ifndef line3_h
#define line3_h

#include "vector3.h"
#include "sphere3.h"
#include "plane3.h"
#include "box3.h"
#include "triangle3.h"

struct line3 {
    VECTOR3 a;
    VECTOR3 b;
};

typedef struct line3 LINE3;
typedef struct line3 *LPLINE3;
typedef struct line3 const *LPCLINE3;

int Line3_intersect_sphere3(LPCLINE3 line, LPCSPHERE3 sphere, LPVECTOR3 output);
int Line3_intersect_plane3(LPCLINE3 line, LPCPLANE3 plane, LPVECTOR3 output);
int Line3_intersect_triangle(LPCLINE3 line, LPCTRIANGLE3 triangle, LPVECTOR3 output);
int Line3_intersect_box3(LPCLINE3 line, LPCBOX3 box, LPVECTOR3 output);

#endif
