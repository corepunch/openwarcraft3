#ifndef sphere3_h
#define sphere3_h

#include "vector3.h"

struct sphere3 {
    VECTOR3 center;
    float radius;
};

typedef struct sphere3 SPHERE3;
typedef struct sphere3 *LPSPHERE3;
typedef struct sphere3 const *LPCSPHERE3;

#endif
