#ifndef box2_h
#define box2_h

#include "vector2.h"

struct box2 {
    VECTOR2 min;
    VECTOR2 max;
};

typedef struct box2 BOX2;
typedef struct box2 *LPBOX2;
typedef struct box2 const *LPCBOX2;

#endif
