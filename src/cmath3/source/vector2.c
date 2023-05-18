#include <math.h>
#include "../cmath3.h"

void Vector2_set(LPVECTOR2 v, float x, float y) {
    v->x = x;
    v->y = y;
}

VECTOR2 Vector2_scale(LPCVECTOR2 v, float s) {
    return (VECTOR2) {
        .x = v->x * s,
        .y = v->y * s
    };
}

float Vector2_dot(LPCVECTOR2 a, LPCVECTOR2 b) {
    return a->x * b->x + a->y * b->y;
}

float Vector2_lengthsq(LPCVECTOR2 vec) {
    return Vector2_dot(vec, vec);
}

float Vector2_len(LPCVECTOR2 vec) {
    return sqrtf(Vector2_lengthsq(vec));
}
