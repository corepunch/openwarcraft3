#ifndef line3_h
#define line3_h

#include "vector3.h"
#include "sphere3.h"
#include "plane3.h"
#include "box3.h"

struct line3 {
    VECTOR3 a;
    VECTOR3 b;
};

typedef struct line3 LINE3;
typedef struct line3 *LPLINE3;
typedef struct line3 const *LPCLINE3;

int Line3_intersect_sphere3(LPCLINE3 lpLine, LPCSPHERE3 lpSphere, LPVECTOR3 lpOutput);
int Line3_intersect_plane3(LPCLINE3 lpLine, LPCPLANE3 lpPlane, LPVECTOR3 lpOutput);
int Line3_intersect_convex(LPCLINE3 lpLine, LPCVECTOR3 lpVertices, int numVertices, LPVECTOR3 lpOutput);
int Line3_intersect_box3(LPCLINE3 lpLine, LPCBOX3 lpBox, LPVECTOR3 lpOutput);

#endif
