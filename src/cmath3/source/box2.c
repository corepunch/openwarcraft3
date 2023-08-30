#include "../cmath3.h"

VECTOR2 Box2_center(LPCBOX2 box) {
    return (VECTOR2) {
        (box->min.x + box->max.x) * 0.5f,
        (box->min.y + box->max.y) * 0.5f,
    };
}

void Box2_moveTo(LPBOX2 box, LPCVECTOR2 newCenterLoc) {
    VECTOR2 center = Box2_center(box);
    box->min.x += newCenterLoc->x - center.x;
    box->max.x += newCenterLoc->x - center.x;
    box->min.y += newCenterLoc->y - center.y;
    box->max.y += newCenterLoc->y - center.y;
}

int Box2_containsPoint(LPCBOX2 box, LPCVECTOR2 point) {
    if (point->x < box->min.x) return 0;
    if (point->y < box->min.y) return 0;
    if (point->x >= box->max.x) return 0;
    if (point->y >= box->max.y) return 0;
    return 1;
}
