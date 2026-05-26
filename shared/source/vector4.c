#include "../cmath3.h"

void Vector4_set(VECTOR4* v, float x, float y, float z, float w) {
    v->x = x;
    v->y = y;
    v->z = z;
    v->w = w;
}

VECTOR4 Vector4_scale(LPCVECTOR4 v, float s) {
    return (VECTOR4) {
        .x = v->x * s,
        .y = v->y * s,
        .z = v->z * s,
        .w = v->w * s
    };
}

VECTOR4 Vector4_add(LPCVECTOR4 a, LPCVECTOR4 b) {
    return (VECTOR4) {
        .x = a->x + b->x,
        .y = a->y + b->y,
        .z = a->z + b->z,
        .w = a->w + b->w
    };
}

VECTOR4 Vector4_unm(LPCVECTOR4 v) {
    return (VECTOR4) {
        .x = -v->x,
        .y = -v->y,
        .z = -v->z,
        .w = -v->w
    };
}
