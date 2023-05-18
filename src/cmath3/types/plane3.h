#ifndef plane3_h
#define plane3_h

#include "vector3.h"

struct plane3 {
    VECTOR3 point;
    VECTOR3 normal;
};

typedef struct plane3 PLANE3;
typedef struct plane3 *LPPLANE3;
typedef struct plane3 const *LPCPLANE3;

#endif
