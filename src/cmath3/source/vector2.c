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

VECTOR2 Vector2_sub(LPCVECTOR2 a, LPCVECTOR2 b) {
    return (VECTOR2) {
        .x = a->x - b->x,
        .y = a->y - b->y,
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

float Vector2_distance(LPCVECTOR2 a, LPCVECTOR2 b) {
    VECTOR2 const dist = Vector2_sub(a, b);
    return Vector2_len(&dist);
}

void Vector2_normalize(LPVECTOR2 v) {
    *v = Vector2_scale(v, 1 / Vector2_len(v));
}

VECTOR2 Vector2_lerp(LPCVECTOR2 a, LPCVECTOR2 b, float t) {
    return (VECTOR2) {
        .x = a->x * (1 - t) + b->x * t,
        .y = a->y * (1 - t) + b->y * t,
    };
}

float LerpNumber(float a, float b, float t) {
    return a * (1 - t) + b * t;
}
