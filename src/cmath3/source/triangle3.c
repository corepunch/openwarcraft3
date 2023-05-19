#include "../cmath3.h"

VECTOR3 Triangle_normal(LPCTRIANGLE3 lpTriangle) {
    VECTOR3 const tside1 = Vector3_sub(&lpTriangle->b, &lpTriangle->a);
    VECTOR3 const tside2 = Vector3_sub(&lpTriangle->c, &lpTriangle->a);
    return Vector3_cross(&tside1, &tside2);
}

