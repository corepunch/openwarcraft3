#ifndef box3_h
#define box3_h

#include "vector3.h"

struct box3 {
    VECTOR3 min;
    VECTOR3 max;
};

typedef struct box3 BOX3;
typedef struct box3 *LPBOX3;
typedef struct box3 const *LPCBOX3;

#endif
