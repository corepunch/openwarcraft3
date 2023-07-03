#include "../cmath3.h"

VECTOR3 Box3_Center(LPCBOX3 box) {
    return (VECTOR3) {
        (box->min.x + box->max.x) * 0.5f,
        (box->min.y + box->max.y) * 0.5f,
        (box->min.z + box->max.z) * 0.5f,
    };
}
