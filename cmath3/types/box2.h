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

VECTOR2 Box2_center(LPCBOX2 box2);
void Box2_moveTo(LPBOX2 box, LPCVECTOR2 newCenterLoc);
int Box2_containsPoint(LPCBOX2 box, LPCVECTOR2 point);

#endif
