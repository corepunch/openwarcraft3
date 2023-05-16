#ifndef math_h
#define math_h

enum rotation_order {
    ROTATE_XYZ,
    ROTATE_XZY,
    ROTATE_YZX,
    ROTATE_YXZ,
    ROTATE_ZXY,
    ROTATE_ZYX
};

struct vector2 { float x, y; };
struct vector3 { float x, y, z; };
struct vector4 { float x, y, z, w; };
struct quaternion { float x, y, z, w; };
struct matrix4 { union { float v[16]; struct vector4 column[4]; }; };

float vector3_dot(struct vector3 const *a, struct vector3 const *b);
float vector3_lengthsq(struct vector3 const *vec);
float vector3_len(struct vector3 const *vec);
bool vector3_eq(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_lerp(struct vector3 const *a, struct vector3 const *b, float t);
struct vector3 vector3_cross(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_sub(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_add(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_mad(struct vector3 const *v, float s, struct vector3 const *b);
struct vector3 vector3_mul(struct vector3 const *a, struct vector3 const *b);
struct vector3 vector3_scale(struct vector3 const *v, float s);
void vector3_normalize(struct vector3* v);
void vector3_set(struct vector3* v, float x, float y, float z);
void vector3_clear(struct vector3* v);
struct vector3 vector3_unm(struct vector3 const* v);
void vector2_set(struct vector2* v, float x, float y);
struct vector2 vector2_scale(struct vector2 const *v, float s);
float vector2_dot(struct vector2 const *a, struct vector2 const *b);
float vector2_lengthsq(struct vector2 const *vec);
float vector2_len(struct vector2 const *vec);
void vector4_set(struct vector4* v, float x, float y, float z, float w);
struct vector4 vector4_scale(struct vector4 const *v, float s);
struct vector4 vector4_add(struct vector4 const *a, struct vector4 const *b);
struct vector4 vector4_unm(struct vector4 const* v);
void matrix4_identity(struct matrix4 *m);
void matrix4_translate(struct matrix4 *m, struct vector3 const *v);
void matrix4_rotate(struct matrix4 *m, struct vector3 const *v, enum rotation_order order);
void matrix4_scale(struct matrix4 *m, struct vector3 const *v);
void matrix4_multiply(struct matrix4 const *m1, struct matrix4 const *m2, struct matrix4 *out);
void matrix4_multiply_vector3(struct matrix4 const *m1, struct vector3 const *v, struct vector3 *out);
void matrix4_ortho(struct matrix4 *m, float left, float right, float bottom, float top, float znear, float zfar);
void matrix4_perspective(struct matrix4 *m, float angle, float aspect, float znear, float zfar);
void matrix4_lookat(struct matrix4 *m, struct vector3 const *eye, struct vector3 const *direction, struct vector3 const *up);
void matrix4_inverse(struct matrix4 const *m, struct matrix4 *out);
void matrix4_transpose(struct matrix4 const *m, struct matrix4 *out);
void matrix4_rotate4(struct matrix4 *m, struct vector4 const *quat);
void matrix4_from_rotation_origin(struct matrix4 *out, struct vector4 const *rotation, struct vector3 const *origin);
void matrix4_from_rotation_translation_scale_origin(struct matrix4 *out, struct vector4 const *q, struct vector3 const *v, struct vector3 const *s, struct vector3 const *o);
void matrix4_from_translation(struct matrix4 *out, struct vector3 const *v);

struct vector4 quaternion_lerp(struct vector4 const *p, struct vector4 const *q, float t);

#endif
