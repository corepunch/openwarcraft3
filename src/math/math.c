#include "../common/common.h"

#include <string.h>
#include <math.h>

void Matrix4_identity(LPMATRIX4 m) {
    m->v[0] = 1; m->v[1] = 0; m->v[2] = 0; m->v[3] = 0;
    m->v[4] = 0; m->v[5] = 1; m->v[6] = 0; m->v[7] = 0;
    m->v[8] = 0; m->v[9] = 0; m->v[10] = 1;m->v[11] = 0;
    m->v[12] = 0;m->v[13] = 0;m->v[14] = 0;m->v[15] = 1;
}

void Matrix4_translate(LPMATRIX4 m, LPCVECTOR3 v) {
    m->v[12] += m->v[0] * v->x + m->v[4] * v->y + m->v[8] * v->z;
    m->v[13] += m->v[1] * v->x + m->v[5] * v->y + m->v[9] * v->z;
    m->v[14] += m->v[2] * v->x + m->v[6] * v->y + m->v[10] * v->z;
    m->v[15] += m->v[3] * v->x + m->v[7] * v->y + m->v[11] * v->z;
}

void Matrix4_scale(LPMATRIX4 m, LPCVECTOR3 v) {
    m->v[0] *= v->x; m->v[1] *= v->x; m->v[2] *= v->x;
    m->v[4] *= v->y; m->v[5] *= v->y; m->v[6] *= v->y;
    m->v[8] *= v->z; m->v[9] *= v->z; m->v[10]*= v->z;
}

void Matrix4_transpose(LPCMATRIX4 m, LPMATRIX4 out) {
    out->v[0] = m->v[0];
    out->v[4] = m->v[1];
    out->v[8] = m->v[2];
    out->v[12] = m->v[3];
    out->v[1] = m->v[4];
    out->v[5] = m->v[5];
    out->v[9] = m->v[6];
    out->v[13] = m->v[7];
    out->v[2] = m->v[8];
    out->v[6] = m->v[9];
    out->v[10] = m->v[10];
    out->v[14] = m->v[11];
    out->v[3] = m->v[12];
    out->v[7] = m->v[13];
    out->v[11] = m->v[14];
    out->v[15] = m->v[15];
}

void Matrix4_rotate4(LPMATRIX4 m, LPCVECTOR4 quat) {
    MATRIX4 r, tmp;

    float fTx  = 2.0f*quat->x;
    float fTy  = 2.0f*quat->y;
    float fTz  = 2.0f*quat->z;
    float fTwx = fTx*quat->w;
    float fTwy = fTy*quat->w;
    float fTwz = fTz*quat->w;
    float fTxx = fTx*quat->x;
    float fTxy = fTy*quat->x;
    float fTxz = fTz*quat->x;
    float fTyy = fTy*quat->y;
    float fTyz = fTz*quat->y;
    float fTzz = fTz*quat->z;
    
    Matrix4_identity(&r);

    r.column[0].x = 1.0f-(fTyy+fTzz);
    r.column[1].x = fTxy-fTwz;
    r.column[2].x = fTxz+fTwy;
    r.column[0].y = fTxy+fTwz;
    r.column[1].y = 1.0f-(fTxx+fTzz);
    r.column[2].y = fTyz-fTwx;
    r.column[0].z = fTxz-fTwy;
    r.column[1].z = fTyz+fTwx;
    r.column[2].z = 1.0f-(fTxx+fTyy);
    
    Matrix4_multiply(m, &r, &tmp);
    *m = tmp;
//    memcpy(m, &tmp, sizeof(MATRIX4));
}

void Matrix4_perspective(LPMATRIX4 m, float angle, float aspect, float znear, float zfar) {
    float const radians = angle * 3.14159f / 360.0f;
    float const sine = sin(radians);
    float const cotan = cos(radians) / sine;
    float const clip = zfar - znear;

    m->v[0]  = cotan / aspect;
    m->v[1]  = 0.0f;
    m->v[2]  = 0.0f;
    m->v[3]  = 0.0f;
    m->v[4]  = 0.0f;
    m->v[5]  = cotan;
    m->v[6]  = 0.0f;
    m->v[7]  = 0.0f;
    m->v[8]  = 0.0f;
    m->v[9]  = 0.0f;
    m->v[10] = -(znear + zfar) / clip;
    m->v[11] = -1.0f;
    m->v[12] = 0.0f;
    m->v[13] = 0.0f;
    m->v[14] = -(2.0f * znear * zfar) / clip;
    m->v[15] = 0.0f;
}

void Matrix4_ortho(LPMATRIX4 m, float left, float right, float bottom, float top, float znear, float zfar) {
    float const width = right - left;
    float const invheight = top - bottom;
    float const clip = zfar - znear;

    m->v[0]  = 2.0f / width;
    m->v[1]  = 0.0f;
    m->v[2]  = 0.0f;
    m->v[3]  = 0.0f;
    m->v[4]  = 0.0f;
    m->v[5]  = 2.0f / invheight;
    m->v[6]  = 0.0f;
    m->v[7]  = 0.0f;
    m->v[8]  = 0.0f;
    m->v[9]  = 0.0f;
    m->v[10] = -2.0f / clip;
    m->v[11] = 0.0f;
    m->v[12] = -(left + right) / width;
    m->v[13] = -(top + bottom) / invheight;
    m->v[14] = -(znear + zfar) / clip;
    m->v[15] = 1.0f;
}

void Matrix4_lookAt(LPMATRIX4 m, LPCVECTOR3 eye, LPCVECTOR3 direction, LPCVECTOR3 up) {
    VECTOR3 zaxis = Vector3_unm(direction);
    VECTOR3 xyz = Vector3_unm(eye);
    VECTOR3 xaxis = Vector3_cross(up, &zaxis);
    VECTOR3 yaxis = Vector3_cross(&zaxis, &xaxis);

    Vector3_normalize(&xaxis);
    Vector3_normalize(&yaxis);
    Vector3_normalize(&zaxis);

    m->v[0] = xaxis.x;
    m->v[1] = yaxis.x;
    m->v[2] = zaxis.x;
    m->v[3] = 0.0f;
    m->v[4] = xaxis.y;
    m->v[5] = yaxis.y;
    m->v[6] = zaxis.y;
    m->v[7] = 0.0f;
    m->v[8] = xaxis.z;
    m->v[9] = yaxis.z;
    m->v[10] = zaxis.z;
    m->v[11] = 0.0f;
    m->v[12] = 0.0f;
    m->v[13] = 0.0f;
    m->v[14] = 0.0f;
    m->v[15] = 1.0f;
    
    Matrix4_translate(m, &xyz);
}

void Matrix4_multiply(LPCMATRIX4 m1, LPCMATRIX4 m2, LPMATRIX4 out) {
//    for (int i = 0 ; i < 4 ; i++) {
//        for (int j = 0 ; j < 4 ; j++) {
//            out->v[ i * 4 + j ] =
//                  a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
//                + a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
//                + a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
//                + a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
//        }
//    }
    out->v[0] = m1->v[0] * m2->v[0] + m1->v[4] * m2->v[1] + m1->v[8] * m2->v[2] + m1->v[12] * m2->v[3];
    out->v[4] = m1->v[0] * m2->v[4] + m1->v[4] * m2->v[5] + m1->v[8] * m2->v[6] + m1->v[12] * m2->v[7];
    out->v[8] = m1->v[0] * m2->v[8] + m1->v[4] * m2->v[9] + m1->v[8] * m2->v[10] + m1->v[12] * m2->v[11];
    out->v[12] = m1->v[0] * m2->v[12] + m1->v[4] * m2->v[13] + m1->v[8] * m2->v[14] + m1->v[12] * m2->v[15];
    out->v[1] = m1->v[1] * m2->v[0] + m1->v[5] * m2->v[1] + m1->v[9] * m2->v[2] + m1->v[13] * m2->v[3];
    out->v[5] = m1->v[1] * m2->v[4] + m1->v[5] * m2->v[5] + m1->v[9] * m2->v[6] + m1->v[13] * m2->v[7];
    out->v[9] = m1->v[1] * m2->v[8] + m1->v[5] * m2->v[9] + m1->v[9] * m2->v[10] + m1->v[13] * m2->v[11];
    out->v[13] = m1->v[1] * m2->v[12] + m1->v[5] * m2->v[13] + m1->v[9] * m2->v[14] + m1->v[13] * m2->v[15];
    out->v[2] = m1->v[2] * m2->v[0] + m1->v[6] * m2->v[1] + m1->v[10] * m2->v[2] + m1->v[14] * m2->v[3];
    out->v[6] = m1->v[2] * m2->v[4] + m1->v[6] * m2->v[5] + m1->v[10] * m2->v[6] + m1->v[14] * m2->v[7];
    out->v[10] = m1->v[2] * m2->v[8] + m1->v[6] * m2->v[9] + m1->v[10] * m2->v[10] + m1->v[14] * m2->v[11];
    out->v[14] = m1->v[2] * m2->v[12] + m1->v[6] * m2->v[13] + m1->v[10] * m2->v[14] + m1->v[14] * m2->v[15];
    out->v[3] = m1->v[3] * m2->v[0] + m1->v[7] * m2->v[1] + m1->v[11] * m2->v[2] + m1->v[15] * m2->v[3];
    out->v[7] = m1->v[3] * m2->v[4] + m1->v[7] * m2->v[5] + m1->v[11] * m2->v[6] + m1->v[15] * m2->v[7];
    out->v[11] = m1->v[3] * m2->v[8] + m1->v[7] * m2->v[9] + m1->v[11] * m2->v[10] + m1->v[15] * m2->v[11];
    out->v[15] = m1->v[3] * m2->v[12] + m1->v[7] * m2->v[13] + m1->v[11] * m2->v[14] + m1->v[15] * m2->v[15];
}

void Matrix4_inverse(LPCMATRIX4 m, LPMATRIX4 out) {
    float det, invDet;
    
    // 2x2 sub-determinants required to calculate 4x4 determinant
    float det2_01_01 = m->v[0] * m->v[5] - m->v[1] * m->v[4];
    float det2_01_02 = m->v[0] * m->v[6] - m->v[2] * m->v[4];
    float det2_01_03 = m->v[0] * m->v[7] - m->v[3] * m->v[4];
    float det2_01_12 = m->v[1] * m->v[6] - m->v[2] * m->v[5];
    float det2_01_13 = m->v[1] * m->v[7] - m->v[3] * m->v[5];
    float det2_01_23 = m->v[2] * m->v[7] - m->v[3] * m->v[6];
    
    // 3x3 sub-determinants required to calculate 4x4 determinant
    float det3_201_012 = m->v[8] * det2_01_12 - m->v[9] * det2_01_02 + m->v[10] * det2_01_01;
    float det3_201_013 = m->v[8] * det2_01_13 - m->v[9] * det2_01_03 + m->v[11] * det2_01_01;
    float det3_201_023 = m->v[8] * det2_01_23 - m->v[10] * det2_01_03 + m->v[11] * det2_01_02;
    float det3_201_123 = m->v[9] * det2_01_23 - m->v[10] * det2_01_13 + m->v[11] * det2_01_12;
    
    det = (- det3_201_123 * m->v[12] + det3_201_023 * m->v[13] - det3_201_013 * m->v[14] + det3_201_012 * m->v[15]);
    
    if (fabs(det) < EPSILON) {
        return;
    }
    
    invDet = 1.0f / det;
    
    // remaining 2x2 sub-determinants
    float det2_03_01 = m->v[0] * m->v[13] - m->v[1] * m->v[12];
    float det2_03_02 = m->v[0] * m->v[14] - m->v[2] * m->v[12];
    float det2_03_03 = m->v[0] * m->v[15] - m->v[3] * m->v[12];
    float det2_03_12 = m->v[1] * m->v[14] - m->v[2] * m->v[13];
    float det2_03_13 = m->v[1] * m->v[15] - m->v[3] * m->v[13];
    float det2_03_23 = m->v[2] * m->v[15] - m->v[3] * m->v[14];
    
    float det2_13_01 = m->v[4] * m->v[13] - m->v[5] * m->v[12];
    float det2_13_02 = m->v[4] * m->v[14] - m->v[6] * m->v[12];
    float det2_13_03 = m->v[4] * m->v[15] - m->v[7] * m->v[12];
    float det2_13_12 = m->v[5] * m->v[14] - m->v[6] * m->v[13];
    float det2_13_13 = m->v[5] * m->v[15] - m->v[7] * m->v[13];
    float det2_13_23 = m->v[6] * m->v[15] - m->v[7] * m->v[14];
    
    // remaining 3x3 sub-determinants
    float det3_203_012 = m->v[8] * det2_03_12 - m->v[9] * det2_03_02 + m->v[10] * det2_03_01;
    float det3_203_013 = m->v[8] * det2_03_13 - m->v[9] * det2_03_03 + m->v[11] * det2_03_01;
    float det3_203_023 = m->v[8] * det2_03_23 - m->v[10] * det2_03_03 + m->v[11] * det2_03_02;
    float det3_203_123 = m->v[9] * det2_03_23 - m->v[10] * det2_03_13 + m->v[11] * det2_03_12;
    
    float det3_213_012 = m->v[8] * det2_13_12 - m->v[9] * det2_13_02 + m->v[10] * det2_13_01;
    float det3_213_013 = m->v[8] * det2_13_13 - m->v[9] * det2_13_03 + m->v[11] * det2_13_01;
    float det3_213_023 = m->v[8] * det2_13_23 - m->v[10] * det2_13_03 + m->v[11] * det2_13_02;
    float det3_213_123 = m->v[9] * det2_13_23 - m->v[10] * det2_13_13 + m->v[11] * det2_13_12;
    
    float det3_301_012 = m->v[12] * det2_01_12 - m->v[13] * det2_01_02 + m->v[14] * det2_01_01;
    float det3_301_013 = m->v[12] * det2_01_13 - m->v[13] * det2_01_03 + m->v[15] * det2_01_01;
    float det3_301_023 = m->v[12] * det2_01_23 - m->v[14] * det2_01_03 + m->v[15] * det2_01_02;
    float det3_301_123 = m->v[13] * det2_01_23 - m->v[14] * det2_01_13 + m->v[15] * det2_01_12;
    
    out->v[0] =  - det3_213_123 * invDet;
    out->v[4] =  + det3_213_023 * invDet;
    out->v[8] =  - det3_213_013 * invDet;
    out->v[12] = + det3_213_012 * invDet;
    out->v[1] =  + det3_203_123 * invDet;
    out->v[5] =  - det3_203_023 * invDet;
    out->v[9] =  + det3_203_013 * invDet;
    out->v[13] = - det3_203_012 * invDet;
    out->v[2] =  + det3_301_123 * invDet;
    out->v[6] =  - det3_301_023 * invDet;
    out->v[10] = + det3_301_013 * invDet;
    out->v[14] = - det3_301_012 * invDet;
    out->v[3] =  - det3_201_123 * invDet;
    out->v[7] =  + det3_201_023 * invDet;
    out->v[11] = - det3_201_013 * invDet;
    out->v[15] = + det3_201_012 * invDet;
}

void Matrix4_multiply_vector3(LPCMATRIX4 m, LPCVECTOR3 v, LPVECTOR3 out) {
    float fInvW = 1.0f / (m->v[3] * v->x + m->v[7] * v->y + m->v[11] * v->z + m->v[15]);
    out->x = (m->v[0] * v->x + m->v[4] * v->y + m->v[8]  * v->z + m->v[12]) * fInvW;
    out->y = (m->v[1] * v->x + m->v[5] * v->y + m->v[9]  * v->z + m->v[13]) * fInvW;
    out->z = (m->v[2] * v->x + m->v[6] * v->y + m->v[10] * v->z + m->v[14]) * fInvW;
}

void Matrix4_rotate(LPMATRIX4 m, LPCVECTOR3 euler, enum rotation_order order) {
    float const DEG2RAD = 3.14159f / 180.f;
    
    MATRIX4 rx, ry, rz, tmp, tmp2;

    Matrix4_identity(&rx);
    Matrix4_identity(&ry);
    Matrix4_identity(&rz);
    
    rx.v[5] = rx.v[10] = cos(euler->x * DEG2RAD);
    rx.v[9] = rx.v[6] = sin(euler->x * DEG2RAD);
    rx.v[9] = -rx.v[9];

    ry.v[0] = ry.v[10] = cos(euler->y * DEG2RAD);
    ry.v[8] = ry.v[2] = sin(euler->y * DEG2RAD);
    ry.v[2] = -ry.v[2];

    rz.v[0] = rz.v[5] = cos(euler->z * DEG2RAD);
    rz.v[4] = rz.v[1] = sin(euler->z * DEG2RAD);
    rz.v[4] = -rz.v[4];
    
    switch (order) {
        case ROTATE_XYZ:
            Matrix4_multiply(&rz, &ry, &tmp);
            Matrix4_multiply(&tmp, &rx, &tmp2);
            break;
        case ROTATE_XZY:
            Matrix4_multiply(&ry, &rz, &tmp);
            Matrix4_multiply(&tmp, &rx, &tmp2);
            break;
        case ROTATE_YXZ:
            Matrix4_multiply(&rz, &rx, &tmp);
            Matrix4_multiply(&tmp, &ry, &tmp2);
            break;
        case ROTATE_YZX:
            Matrix4_multiply(&rx, &rz, &tmp);
            Matrix4_multiply(&tmp, &ry, &tmp2);
            break;
        case ROTATE_ZXY:
            Matrix4_multiply(&ry, &rx, &tmp);
            Matrix4_multiply(&tmp, &rz, &tmp2);
            break;
        case ROTATE_ZYX:
            Matrix4_multiply(&rx, &ry, &tmp);
            Matrix4_multiply(&tmp, &rz, &tmp2);
            break;
        default:
            Matrix4_identity(&tmp2);
            break;
    }
    
    tmp = *m;
    Matrix4_multiply(&tmp, &tmp2, m);
}

struct rect rect_new(float x, float y, float width, float height) {
    return (struct rect) { x, y, width, height };
}

struct edges edges_new(float left, float top, float right, float bottom) {
    return (struct edges) { left, top, right, bottom };
}

struct color color_new(float r, float g, float b, float a) {
    return (struct color) { r, g, b, a };
}

bool rect_contains_point(struct rect const *rect, LPCVECTOR2 point) {
    if (rect->x > point->x) return false;
    if (rect->y > point->y) return false;
    if ((rect->x + rect->width) <= point->x) return false;
    if ((rect->y + rect->height) <= point->y) return false;
    return true;
}

void transform3_identity(struct transform3 *transform) {
    memcpy(transform, &(struct transform3) {
        .translation = { 0, 0, 0 },
        .rotation = { 0, 0, 0 },
        .scale = { 1, 1, 1 },
    }, sizeof(struct transform3));
}

void transform3_to_matrix4(struct transform3 const *transform, LPMATRIX4 matrix) {
//    VECTOR3 neg_pivot = { 0, 0, 0 };
//    Vector3Negate(transform->pivot, neg_pivot);
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &transform->translation);
//    Matrix4_translate(matrix, &transform->pivot);
    Matrix4_rotate(matrix, &transform->rotation, ROTATE_XYZ);
    Matrix4_scale(matrix, &transform->scale);
//    Matrix4_translate(matrix, &neg_pivot);
}

void transform2_identity(struct transform2 *transform) {
    memcpy(transform, &(struct transform2) {
        .translation = { 0, 0},
        .rotation = 0,
        .scale = { 1, 1 },
    }, sizeof(struct transform2));
}

void transform2_to_matrix4(struct transform2 const *transform, LPCVECTOR2 pivot, LPMATRIX4 matrix) {
    Matrix4_identity(matrix);

    Matrix4_translate(matrix, &(VECTOR3 const) {
        .x = transform->translation.x,
        .y = transform->translation.y,
        .z = 0
    });

    if (pivot) {
        Matrix4_translate(matrix, &(VECTOR3 const) {
            .x = pivot->x,
            .y = pivot->y,
            .z = 0
        });
    }

    Matrix4_rotate(matrix, &(VECTOR3 const) {
        .x = 0,
        .y = 0,
        .z = transform->rotation
    }, ROTATE_XYZ);

    Matrix4_scale(matrix, &(VECTOR3 const) {
        .x = transform->scale.x,
        .y = transform->scale.y,
        .z = 1
    });

    if (pivot) {
        Matrix4_translate(matrix, &(VECTOR3 const) {
            .x = -pivot->x,
            .y = -pivot->y,
            .z = 0
        });
    }
}

float Vector3_dot(LPCVECTOR3 a,LPCVECTOR3 b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

float Vector3_lengthsq(LPCVECTOR3 vec) {
    return Vector3_dot(vec, vec);
}

float Vector3_len(LPCVECTOR3 vec) {
    return sqrtf(Vector3_lengthsq(vec));
}

bool Vector3_eq(LPCVECTOR3 a, LPCVECTOR3 b) {
    return
        fabs(a->x - b->x) < EPSILON &&
        fabs(a->y - b->y) < EPSILON &&
        fabs(a->z - b->z) < EPSILON;
}

VECTOR3 Vector3_bezier(LPCVECTOR3 a, LPCVECTOR3 b, LPCVECTOR3 c, LPCVECTOR3 d, float t) {
    float const inverseFactor = 1 - t;
    float const inverseFactorTimesTwo = inverseFactor * inverseFactor;
    float const factorTimes2 = t * t;
    float const factor1 = inverseFactorTimesTwo * inverseFactor;
    float const factor2 = 3 * t * inverseFactorTimesTwo;
    float const factor3 = 3 * factorTimes2 * inverseFactor;
    float const factor4 = factorTimes2 * t;

    return (VECTOR3) {
        a->x * factor1 + b->x * factor2 + c->x * factor3 + d->x * factor4,
        a->y * factor1 + b->y * factor2 + c->y * factor3 + d->y * factor4,
        a->z * factor1 + b->z * factor2 + c->z * factor3 + d->z * factor4,
    };
}

VECTOR3 Vector3_hermite(LPCVECTOR3 a, LPCVECTOR3 b, LPCVECTOR3 c, LPCVECTOR3 d, float t) {
    float const factorTimes2 = t * t;
    float const factor1 = factorTimes2 * (2 * t - 3) + 1;
    float const factor2 = factorTimes2 * (t - 2) + t;
    float const factor3 = factorTimes2 * (t - 1);
    float const factor4 = factorTimes2 * (3 - 2 * t);

    return (VECTOR3) {
        a->x * factor1 + b->x * factor2 + c->x * factor3 + d->x * factor4,
        a->y * factor1 + b->y * factor2 + c->y * factor3 + d->y * factor4,
        a->z * factor1 + b->z * factor2 + c->z * factor3 + d->z * factor4,
    };
}

VECTOR3 Vector3_lerp(LPCVECTOR3 a, LPCVECTOR3 b, float t) {
    return (VECTOR3) {
        .x = a->x * (1 - t) + b->x * t,
        .y = a->y * (1 - t) + b->y * t,
        .z = a->z * (1 - t) + b->z * t
    };
}

VECTOR3 Vector3_cross(LPCVECTOR3 a, LPCVECTOR3 b) {
    return (VECTOR3) {
        .x = a->y * b->z - a->z * b->y,
        .y = a->z * b->x - a->x * b->z,
        .z = a->x * b->y - a->y * b->x
    };
}

VECTOR3 Vector3_sub(LPCVECTOR3 a, LPCVECTOR3 b) {
    return (VECTOR3) {
        .x = a->x - b->x,
        .y = a->y - b->y,
        .z = a->z - b->z
    };
}

VECTOR3 Vector3_add(LPCVECTOR3 a, LPCVECTOR3 b) {
    return (VECTOR3) {
        .x = a->x + b->x,
        .y = a->y + b->y,
        .z = a->z + b->z
    };
}

VECTOR3 Vector3_mad(LPCVECTOR3 v, float s, LPCVECTOR3 b) {
    return (VECTOR3) {
        .x = v->x + b->x * s,
        .y = v->y + b->y * s,
        .z = v->z + b->z * s
    };
}

VECTOR3 Vector3_mul(LPCVECTOR3 a, LPCVECTOR3 b) {
    return (VECTOR3) {
        .x = a->x * b->x,
        .y = a->y * b->y,
        .z = a->z * b->z
    };
}

VECTOR3 Vector3_scale(LPCVECTOR3 v, float s) {
    return (VECTOR3) {
        .x = v->x * s,
        .y = v->y * s,
        .z = v->z * s
    };
}

void Vector3_normalize(LPVECTOR3 v) {
    *v = Vector3_scale(v, 1 / Vector3_len(v));
}

void Vector3_set(LPVECTOR3 v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

void Vector3_clear(LPVECTOR3 v) {
    Vector3_set(v, 0, 0, 0);
}

VECTOR3 Vector3_unm(VECTOR3 const* v) {
    return (VECTOR3) {
        .x = -v->x,
        .y = -v->y,
        .z = -v->z
    };
}

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

void Matrix4_from_rotation_origin(LPMATRIX4 out, LPCVECTOR4 rotation, LPCVECTOR3 origin) {
    const float x = rotation->x;
    const float y = rotation->y;
    const float z = rotation->z;
    const float w = rotation->w;
    const float x2 = x + x;
    const float y2 = y + y;
    const float z2 = z + z;
    const float xx = x * x2;
    const float xy = x * y2;
    const float xz = x * z2;
    const float yy = y * y2;
    const float yz = y * z2;
    const float zz = z * z2;
    const float wx = w * x2;
    const float wy = w * y2;
    const float wz = w * z2;
    const float ox = origin->x;
    const float oy = origin->y;
    const float oz = origin->z;

    out->v[0] = (1 - (yy + zz));
    out->v[1] = (xy + wz);
    out->v[2] = (xz - wy);
    out->v[3] = 0;
    out->v[4] = (xy - wz);
    out->v[5] = (1 - (xx + zz));
    out->v[6] = (yz + wx);
    out->v[7] = 0;
    out->v[8] = (xz + wy);
    out->v[9] = (yz - wx);
    out->v[10] = (1 - (xx + yy));
    out->v[11] = 0;
    out->v[12] = ox - (out->v[0] * ox + out->v[4] * oy + out->v[8] * oz);
    out->v[13] = oy - (out->v[1] * ox + out->v[5] * oy + out->v[9] * oz);
    out->v[14] = oz - (out->v[2] * ox + out->v[6] * oy + out->v[10] * oz);
    out->v[15] = 1;
}


void Matrix4_from_rotation_translation_scale_origin(LPMATRIX4 out, LPCVECTOR4 q, LPCVECTOR3 v, LPCVECTOR3 s, LPCVECTOR3 o) {
    const float x = q->x;
    const float y = q->y;
    const float z = q->z;
    const float w = q->w;
    const float x2 = x + x;
    const float y2 = y + y;
    const float z2 = z + z;
    const float xx = x * x2;
    const float xy = x * y2;
    const float xz = x * z2;
    const float yy = y * y2;
    const float yz = y * z2;
    const float zz = z * z2;
    const float wx = w * x2;
    const float wy = w * y2;
    const float wz = w * z2;
    const float sx = s->x;
    const float sy = s->y;
    const float sz = s->z;
    const float ox = o->x;
    const float oy = o->y;
    const float oz = o->z;
    const float out0 = (1 - (yy + zz)) * sx;
    const float out1 = (xy + wz) * sx;
    const float out2 = (xz - wy) * sx;
    const float out4 = (xy - wz) * sy;
    const float out5 = (1 - (xx + zz)) * sy;
    const float out6 = (yz + wx) * sy;
    const float out8 = (xz + wy) * sz;
    const float out9 = (yz - wx) * sz;
    const float out10 = (1 - (xx + yy)) * sz;

    out->v[0] = out0;
    out->v[1] = out1;
    out->v[2] = out2;
    out->v[3] = 0;
    out->v[4] = out4;
    out->v[5] = out5;
    out->v[6] = out6;
    out->v[7] = 0;
    out->v[8] = out8;
    out->v[9] = out9;
    out->v[10] = out10;
    out->v[11] = 0;
    out->v[12] = v->x + ox - (out0 * ox + out4 * oy + out8 * oz);
    out->v[13] = v->y + oy - (out1 * ox + out5 * oy + out9 * oz);
    out->v[14] = v->z + oz - (out2 * ox + out6 * oy + out10 * oz);
    out->v[15] = 1;
}

void Matrix4_from_translation(LPMATRIX4 out, LPCVECTOR3 v) {
  out->v[0] = 1;
  out->v[1] = 0;
  out->v[2] = 0;
  out->v[3] = 0;
  out->v[4] = 0;
  out->v[5] = 1;
  out->v[6] = 0;
  out->v[7] = 0;
  out->v[8] = 0;
  out->v[9] = 0;
  out->v[10] = 1;
  out->v[11] = 0;
  out->v[12] = v->x;
  out->v[13] = v->y;
  out->v[14] = v->z;
  out->v[15] = 1;
}

void Matrix3_normal(LPMATRIX3 out, LPCMATRIX4 modelview) {
    struct matrix4 inverse;
    Matrix4_inverse(modelview, &inverse);
    float const m33[9] = {
        inverse.v[0], inverse.v[1], inverse.v[2],
        inverse.v[4], inverse.v[5], inverse.v[6],
        inverse.v[8], inverse.v[9], inverse.v[10]
    };
    memcpy(out, m33, sizeof(m33));
}
