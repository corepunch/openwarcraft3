#include "../cmath3.h"

VECTOR3 Triangle_normal(LPCTRIANGLE3 triangle) {
    VECTOR3 const tside1 = Vector3_sub(&triangle->b, &triangle->a);
    VECTOR3 const tside2 = Vector3_sub(&triangle->c, &triangle->a);
    return Vector3_cross(&tside1, &tside2);
}
