#ifndef math_h
#define math_h

#define ADD_TYPEDEFS(STRUCT, TYPE) \
typedef struct STRUCT TYPE; \
typedef struct STRUCT *LP##TYPE; \
typedef struct STRUCT const *LPC##TYPE;

enum rotation_order {
    ROTATE_XYZ,
    ROTATE_XZY,
    ROTATE_YZX,
    ROTATE_YXZ,
    ROTATE_ZXY,
    ROTATE_ZYX
};

ADD_TYPEDEFS(vector2, VECTOR2);
ADD_TYPEDEFS(vector3, VECTOR3);
ADD_TYPEDEFS(vector4, VECTOR4);
ADD_TYPEDEFS(quaternion, QUATERNION);
ADD_TYPEDEFS(matrix4, MATRIX4);

struct vector2 { float x, y; };
struct vector3 { float x, y, z; };
struct vector4 { float x, y, z, w; };
struct quaternion { float x, y, z, w; };
struct matrix4 { union { float v[16]; VECTOR4 column[4]; }; };

float Vector3_dot(LPCVECTOR3 a, LPCVECTOR3 b);
float Vector3_lengthsq(LPCVECTOR3 vec);
float Vector3_len(LPCVECTOR3 vec);
bool Vector3_eq(LPCVECTOR3 a, LPCVECTOR3 b);
VECTOR3 Vector3_lerp(LPCVECTOR3 a, LPCVECTOR3 b, float t);
VECTOR3 Vector3_cross(LPCVECTOR3 a, LPCVECTOR3 b);
VECTOR3 Vector3_sub(LPCVECTOR3 a, LPCVECTOR3 b);
VECTOR3 Vector3_add(LPCVECTOR3 a, LPCVECTOR3 b);
VECTOR3 Vector3_mad(LPCVECTOR3 v, float s, LPCVECTOR3 b);
VECTOR3 Vector3_mul(LPCVECTOR3 a, LPCVECTOR3 b);
VECTOR3 Vector3_scale(LPCVECTOR3 v, float s);
void Vector3_normalize(LPVECTOR3 v);
void Vector3_set(LPVECTOR3 v, float x, float y, float z);
void Vector3_clear(LPVECTOR3 v);
VECTOR3 Vector3_unm(VECTOR3 const* v);
void Vector2_set(LPVECTOR2 v, float x, float y);
VECTOR2 Vector2_scale(LPCVECTOR2 v, float s);
float Vector2_dot(LPCVECTOR2 a, LPCVECTOR2 b);
float Vector2_lengthsq(LPCVECTOR2 vec);
float Vector2_len(LPCVECTOR2 vec);
void Vector4_set(VECTOR4* v, float x, float y, float z, float w);
VECTOR4 Vector4_scale(LPCVECTOR4 v, float s);
VECTOR4 Vector4_add(LPCVECTOR4 a, LPCVECTOR4 b);
VECTOR4 Vector4_unm(LPCVECTOR4 v);
void Matrix4_identity(LPMATRIX4 m);
void Matrix4_translate(LPMATRIX4 m, LPCVECTOR3 v);
void Matrix4_rotate(LPMATRIX4 m, LPCVECTOR3 v, enum rotation_order order);
void Matrix4_scale(LPMATRIX4 m, LPCVECTOR3 v);
void Matrix4_multiply(LPCMATRIX4 m1, LPCMATRIX4 m2, LPMATRIX4 out);
void Matrix4_multiply_vector3(LPCMATRIX4 m1, LPCVECTOR3 v, LPVECTOR3 out);
void Matrix4_ortho(LPMATRIX4 m, float left, float right, float bottom, float top, float znear, float zfar);
void Matrix4_perspective(LPMATRIX4 m, float angle, float aspect, float znear, float zfar);
void Matrix4_lookat(LPMATRIX4 m, LPCVECTOR3 eye, LPCVECTOR3 direction, LPCVECTOR3 up);
void Matrix4_inverse(LPCMATRIX4 m, LPMATRIX4 out);
void Matrix4_transpose(LPCMATRIX4 m, LPMATRIX4 out);
void Matrix4_rotate4(LPMATRIX4 m, LPCVECTOR4 quat);
void Matrix4_from_rotation_origin(LPMATRIX4 out, LPCVECTOR4 rotation, LPCVECTOR3 origin);
void Matrix4_from_rotation_translation_scale_origin(LPMATRIX4 out, LPCVECTOR4 q, LPCVECTOR3 v, LPCVECTOR3 s, LPCVECTOR3 o);
void Matrix4_from_translation(LPMATRIX4 out, LPCVECTOR3 v);

VECTOR4 quaternion_lerp(LPCVECTOR4 p, LPCVECTOR4 q, float t);

#endif
