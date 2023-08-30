#ifndef matrix4_h
#define matrix4_h

#include "vector3.h"
#include "vector4.h"
#include "quaternion.h"

struct matrix4 {
    union {
        float v[16];
        VECTOR4 column[4];
        float m[4][4];
    };
};

typedef struct matrix4 MATRIX4;
typedef struct matrix4 *LPMATRIX4;
typedef struct matrix4 const *LPCMATRIX4;

void Matrix4_identity(LPMATRIX4 m);
void Matrix4_translate(LPMATRIX4 m, LPCVECTOR3 v);
void Matrix4_rotate(LPMATRIX4 m, LPCVECTOR3 v, ROTATIONORDER order);
void Matrix4_scale(LPMATRIX4 m, LPCVECTOR3 v);
void Matrix4_multiply(LPCMATRIX4 m1, LPCMATRIX4 m2, LPMATRIX4 out);
void Matrix4_ortho(LPMATRIX4 m, float left, float right, float bottom, float top, float znear, float zfar);
void Matrix4_perspective(LPMATRIX4 m, float angle, float aspect, float znear, float zfar);
void Matrix4_lookAt(LPMATRIX4 m, LPCVECTOR3 eye, LPCVECTOR3 direction, LPCVECTOR3 up);
void Matrix4_inverse(LPCMATRIX4 m, LPMATRIX4 out);
void Matrix4_transpose(LPCMATRIX4 m, LPMATRIX4 out);
void Matrix4_rotate4(LPMATRIX4 m, LPCVECTOR4 quat);
void Matrix4_from_rotation_origin(LPMATRIX4 out, LPCQUATERNION rotation, LPCVECTOR3 origin);
void Matrix4_from_rotation_translation_scale_origin(LPMATRIX4 out, LPCQUATERNION q, LPCVECTOR3 v, LPCVECTOR3 s, LPCVECTOR3 o);
void Matrix4_from_translation(LPMATRIX4 out, LPCVECTOR3 v);
void Matrix4_rotateQuat(LPMATRIX4 m, LPCQUATERNION quat);
VECTOR3 Matrix4_multiply_vector3(LPCMATRIX4 m1, LPCVECTOR3 v);

#endif /* matrix4_h */
