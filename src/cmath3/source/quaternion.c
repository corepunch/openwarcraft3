#include <math.h>
#include "../cmath3.h"

QUATERNION Quaternion_slerp(LPCQUATERNION a, LPCQUATERNION b, float t) {
    float ax = a->x, ay = a->y, az = a->z, aw = a->w;
    float bx = b->x, by = b->y, bz = b->z, bw = b->w;
    float omega, cosom, sinom, scale0, scale1;
    cosom = ax * bx + ay * by + az * bz + aw * bw;
    if (cosom < 0.0f) {
        cosom = -cosom;
        bx = -bx;
        by = -by;
        bz = -bz;
        bw = -bw;
    }
    if (1.0f - cosom > EPSILON) {
        omega = acosf(cosom);
        sinom = sinf(omega);
        scale0 = sinf((1.0f - t) * omega) / sinom;
        scale1 = sinf(t * omega) / sinom;
    } else {
        scale0 = 1.0f - t;
        scale1 = t;
    }
    return (QUATERNION) {
        .x = scale0 * ax + scale1 * bx,
        .y = scale0 * ay + scale1 * by,
        .z = scale0 * az + scale1 * bz,
        .w = scale0 * aw + scale1 * bw,
    };
}

QUATERNION Quaternion_sqlerp(LPCQUATERNION a, LPCQUATERNION b, LPCQUATERNION c, LPCQUATERNION d, float t) {
    QUATERNION temp1 = Quaternion_slerp(a, d, t);
    QUATERNION temp2 = Quaternion_slerp(b, c, t);
    return Quaternion_slerp(&temp1, &temp2, 2 * t * (1 - t));
}
