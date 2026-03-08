/*
 * test_cmath3.c — Unit tests for the cmath3 math library.
 *
 * Covers: LerpNumber, Vector2, Vector3, Box2, Rect, Box3.
 * Build: see Makefile `test` target.
 */
#include <math.h>

#include "../src/cmath3/cmath3.h"
#include "test_harness.h"

/* ------------------------------------------------------------------ */
/* LerpNumber                                                           */
/* ------------------------------------------------------------------ */
static void test_LerpNumber(void) {
    ASSERT_FLOAT_EQ(LerpNumber(0.0f, 1.0f, 0.0f), 0.0f);
    ASSERT_FLOAT_EQ(LerpNumber(0.0f, 1.0f, 1.0f), 1.0f);
    ASSERT_FLOAT_EQ(LerpNumber(0.0f, 10.0f, 0.5f), 5.0f);
    ASSERT_FLOAT_EQ(LerpNumber(2.0f, 4.0f, 0.25f), 2.5f);
    /* Extrapolation */
    ASSERT_FLOAT_EQ(LerpNumber(0.0f, 1.0f, 2.0f), 2.0f);
}

/* ------------------------------------------------------------------ */
/* Vector2                                                              */
/* ------------------------------------------------------------------ */
static void test_Vector2_set(void) {
    VECTOR2 v;
    Vector2_set(&v, 3.0f, 4.0f);
    ASSERT_FLOAT_EQ(v.x, 3.0f);
    ASSERT_FLOAT_EQ(v.y, 4.0f);
}

static void test_Vector2_add(void) {
    VECTOR2 a = {1.0f, 2.0f};
    VECTOR2 b = {3.0f, 4.0f};
    VECTOR2 r = Vector2_add(&a, &b);
    ASSERT_FLOAT_EQ(r.x, 4.0f);
    ASSERT_FLOAT_EQ(r.y, 6.0f);
}

static void test_Vector2_sub(void) {
    VECTOR2 a = {5.0f, 3.0f};
    VECTOR2 b = {2.0f, 1.0f};
    VECTOR2 r = Vector2_sub(&a, &b);
    ASSERT_FLOAT_EQ(r.x, 3.0f);
    ASSERT_FLOAT_EQ(r.y, 2.0f);
}

static void test_Vector2_scale(void) {
    VECTOR2 v = {2.0f, 3.0f};
    VECTOR2 r = Vector2_scale(&v, 2.0f);
    ASSERT_FLOAT_EQ(r.x, 4.0f);
    ASSERT_FLOAT_EQ(r.y, 6.0f);
}

static void test_Vector2_dot(void) {
    VECTOR2 a = {1.0f, 0.0f};
    VECTOR2 b = {0.0f, 1.0f};
    ASSERT_FLOAT_EQ(Vector2_dot(&a, &b), 0.0f);

    VECTOR2 c = {3.0f, 4.0f};
    VECTOR2 d = {3.0f, 4.0f};
    ASSERT_FLOAT_EQ(Vector2_dot(&c, &d), 25.0f);
}

static void test_Vector2_len(void) {
    VECTOR2 v = {3.0f, 4.0f};
    ASSERT_FLOAT_EQ(Vector2_len(&v), 5.0f);

    VECTOR2 zero = {0.0f, 0.0f};
    ASSERT_FLOAT_EQ(Vector2_len(&zero), 0.0f);
}

static void test_Vector2_lengthsq(void) {
    VECTOR2 v = {3.0f, 4.0f};
    ASSERT_FLOAT_EQ(Vector2_lengthsq(&v), 25.0f);
}

static void test_Vector2_distance(void) {
    VECTOR2 a = {0.0f, 0.0f};
    VECTOR2 b = {3.0f, 4.0f};
    ASSERT_FLOAT_EQ(Vector2_distance(&a, &b), 5.0f);
}

static void test_Vector2_normalize(void) {
    VECTOR2 v = {3.0f, 4.0f};
    Vector2_normalize(&v);
    ASSERT_FLOAT_EQ(Vector2_len(&v), 1.0f);
    ASSERT_FLOAT_EQ(v.x, 0.6f);
    ASSERT_FLOAT_EQ(v.y, 0.8f);
}

static void test_Vector2_lerp(void) {
    VECTOR2 a = {0.0f, 0.0f};
    VECTOR2 b = {2.0f, 4.0f};
    VECTOR2 r = Vector2_lerp(&a, &b, 0.5f);
    ASSERT_FLOAT_EQ(r.x, 1.0f);
    ASSERT_FLOAT_EQ(r.y, 2.0f);
}

static void test_Vector2_mad(void) {
    /* mad(v, s, b) = v + b * s */
    VECTOR2 v = {1.0f, 1.0f};
    VECTOR2 b = {2.0f, 3.0f};
    VECTOR2 r = Vector2_mad(&v, 2.0f, &b);
    ASSERT_FLOAT_EQ(r.x, 5.0f);
    ASSERT_FLOAT_EQ(r.y, 7.0f);
}

static void test_Vector2_unm(void) {
    VECTOR2 v = {3.0f, -5.0f};
    VECTOR2 r = Vector2_unm(&v);
    ASSERT_FLOAT_EQ(r.x, -3.0f);
    ASSERT_FLOAT_EQ(r.y, 5.0f);
}

/* ------------------------------------------------------------------ */
/* Vector3                                                              */
/* ------------------------------------------------------------------ */
static void test_Vector3_dot(void) {
    VECTOR3 a = {1.0f, 0.0f, 0.0f};
    VECTOR3 b = {0.0f, 1.0f, 0.0f};
    ASSERT_FLOAT_EQ(Vector3_dot(&a, &b), 0.0f);

    VECTOR3 c = {1.0f, 2.0f, 3.0f};
    VECTOR3 d = {4.0f, 5.0f, 6.0f};
    ASSERT_FLOAT_EQ(Vector3_dot(&c, &d), 32.0f);
}

static void test_Vector3_len(void) {
    VECTOR3 v = {0.0f, 3.0f, 4.0f};
    ASSERT_FLOAT_EQ(Vector3_len(&v), 5.0f);
}

static void test_Vector3_lengthsq(void) {
    VECTOR3 v = {1.0f, 2.0f, 2.0f};
    ASSERT_FLOAT_EQ(Vector3_lengthsq(&v), 9.0f);
}

static void test_Vector3_distance(void) {
    VECTOR3 a = {0.0f, 0.0f, 0.0f};
    VECTOR3 b = {0.0f, 3.0f, 4.0f};
    ASSERT_FLOAT_EQ(Vector3_distance(&a, &b), 5.0f);
}

static void test_Vector3_cross(void) {
    VECTOR3 a = {1.0f, 0.0f, 0.0f};
    VECTOR3 b = {0.0f, 1.0f, 0.0f};
    VECTOR3 r = Vector3_cross(&a, &b);
    ASSERT_FLOAT_EQ(r.x, 0.0f);
    ASSERT_FLOAT_EQ(r.y, 0.0f);
    ASSERT_FLOAT_EQ(r.z, 1.0f);
}

static void test_Vector3_add(void) {
    VECTOR3 a = {1.0f, 2.0f, 3.0f};
    VECTOR3 b = {4.0f, 5.0f, 6.0f};
    VECTOR3 r = Vector3_add(&a, &b);
    ASSERT_FLOAT_EQ(r.x, 5.0f);
    ASSERT_FLOAT_EQ(r.y, 7.0f);
    ASSERT_FLOAT_EQ(r.z, 9.0f);
}

static void test_Vector3_sub(void) {
    VECTOR3 a = {5.0f, 7.0f, 9.0f};
    VECTOR3 b = {1.0f, 2.0f, 3.0f};
    VECTOR3 r = Vector3_sub(&a, &b);
    ASSERT_FLOAT_EQ(r.x, 4.0f);
    ASSERT_FLOAT_EQ(r.y, 5.0f);
    ASSERT_FLOAT_EQ(r.z, 6.0f);
}

static void test_Vector3_scale(void) {
    VECTOR3 v = {1.0f, 2.0f, 3.0f};
    VECTOR3 r = Vector3_scale(&v, 3.0f);
    ASSERT_FLOAT_EQ(r.x, 3.0f);
    ASSERT_FLOAT_EQ(r.y, 6.0f);
    ASSERT_FLOAT_EQ(r.z, 9.0f);
}

static void test_Vector3_normalize(void) {
    VECTOR3 v = {0.0f, 3.0f, 4.0f};
    Vector3_normalize(&v);
    ASSERT_FLOAT_EQ(Vector3_len(&v), 1.0f);
    ASSERT_FLOAT_EQ(v.x, 0.0f);
    ASSERT_FLOAT_EQ(v.y, 0.6f);
    ASSERT_FLOAT_EQ(v.z, 0.8f);
}

static void test_Vector3_lerp(void) {
    VECTOR3 a = {0.0f, 0.0f, 0.0f};
    VECTOR3 b = {2.0f, 4.0f, 6.0f};
    VECTOR3 r = Vector3_lerp(&a, &b, 0.5f);
    ASSERT_FLOAT_EQ(r.x, 1.0f);
    ASSERT_FLOAT_EQ(r.y, 2.0f);
    ASSERT_FLOAT_EQ(r.z, 3.0f);
}

static void test_Vector3_mad(void) {
    /* mad(v, s, b) = v + b * s */
    VECTOR3 v = {1.0f, 1.0f, 1.0f};
    VECTOR3 b = {1.0f, 2.0f, 3.0f};
    VECTOR3 r = Vector3_mad(&v, 2.0f, &b);
    ASSERT_FLOAT_EQ(r.x, 3.0f);
    ASSERT_FLOAT_EQ(r.y, 5.0f);
    ASSERT_FLOAT_EQ(r.z, 7.0f);
}

static void test_Vector3_unm(void) {
    VECTOR3 v = {1.0f, -2.0f, 3.0f};
    VECTOR3 r = Vector3_unm(&v);
    ASSERT_FLOAT_EQ(r.x, -1.0f);
    ASSERT_FLOAT_EQ(r.y, 2.0f);
    ASSERT_FLOAT_EQ(r.z, -3.0f);
}

static void test_Vector3_set(void) {
    VECTOR3 v;
    Vector3_set(&v, 1.0f, 2.0f, 3.0f);
    ASSERT_FLOAT_EQ(v.x, 1.0f);
    ASSERT_FLOAT_EQ(v.y, 2.0f);
    ASSERT_FLOAT_EQ(v.z, 3.0f);
}

static void test_Vector3_clear(void) {
    VECTOR3 v = {5.0f, 6.0f, 7.0f};
    Vector3_clear(&v);
    ASSERT_FLOAT_EQ(v.x, 0.0f);
    ASSERT_FLOAT_EQ(v.y, 0.0f);
    ASSERT_FLOAT_EQ(v.z, 0.0f);
}

/* ------------------------------------------------------------------ */
/* Box2                                                                 */
/* ------------------------------------------------------------------ */
static void test_Box2_center(void) {
    BOX2 box = {{0.0f, 0.0f}, {4.0f, 6.0f}};
    VECTOR2 c = Box2_center(&box);
    ASSERT_FLOAT_EQ(c.x, 2.0f);
    ASSERT_FLOAT_EQ(c.y, 3.0f);
}

static void test_Box2_containsPoint(void) {
    BOX2 box = {{0.0f, 0.0f}, {4.0f, 4.0f}};

    VECTOR2 inside = {2.0f, 2.0f};
    ASSERT(Box2_containsPoint(&box, &inside));

    VECTOR2 outside = {5.0f, 2.0f};
    ASSERT(!Box2_containsPoint(&box, &outside));

    /* On the min boundary: inside */
    VECTOR2 on_min = {0.0f, 0.0f};
    ASSERT(Box2_containsPoint(&box, &on_min));

    /* On the max boundary: outside (half-open interval) */
    VECTOR2 on_max = {4.0f, 4.0f};
    ASSERT(!Box2_containsPoint(&box, &on_max));
}

static void test_Box2_moveTo(void) {
    BOX2 box = {{0.0f, 0.0f}, {4.0f, 4.0f}};
    VECTOR2 newCenter = {6.0f, 8.0f};
    Box2_moveTo(&box, &newCenter);
    ASSERT_FLOAT_EQ(box.min.x, 4.0f);
    ASSERT_FLOAT_EQ(box.min.y, 6.0f);
    ASSERT_FLOAT_EQ(box.max.x, 8.0f);
    ASSERT_FLOAT_EQ(box.max.y, 10.0f);
}

/* ------------------------------------------------------------------ */
/* Box3                                                                 */
/* ------------------------------------------------------------------ */
static void test_Box3_Center(void) {
    BOX3 box = {{0.0f, 0.0f, 0.0f}, {2.0f, 4.0f, 6.0f}};
    VECTOR3 c = Box3_Center(&box);
    ASSERT_FLOAT_EQ(c.x, 1.0f);
    ASSERT_FLOAT_EQ(c.y, 2.0f);
    ASSERT_FLOAT_EQ(c.z, 3.0f);
}

/* ------------------------------------------------------------------ */
/* Rect                                                                 */
/* ------------------------------------------------------------------ */
static void test_Rect_contains(void) {
    RECT rect = {1.0f, 1.0f, 4.0f, 4.0f};  /* x, y, w, h */

    VECTOR2 inside = {3.0f, 3.0f};
    ASSERT(Rect_contains(&rect, &inside));

    VECTOR2 outside = {6.0f, 3.0f};
    ASSERT(!Rect_contains(&rect, &outside));

    /* Left boundary: outside */
    VECTOR2 left = {0.5f, 2.0f};
    ASSERT(!Rect_contains(&rect, &left));

    /* Right boundary (x + w): outside (half-open) */
    VECTOR2 right = {5.0f, 2.0f};
    ASSERT(!Rect_contains(&rect, &right));
}

static void test_Rect_scale(void) {
    RECT rect = {2.0f, 4.0f, 6.0f, 8.0f};
    RECT r = Rect_scale(&rect, 2.0f);
    ASSERT_FLOAT_EQ(r.x, 4.0f);
    ASSERT_FLOAT_EQ(r.y, 8.0f);
    ASSERT_FLOAT_EQ(r.w, 12.0f);
    ASSERT_FLOAT_EQ(r.h, 16.0f);
}

static void test_Rect_div(void) {
    RECT rect = {4.0f, 8.0f, 12.0f, 16.0f};
    RECT r = Rect_div(&rect, 4);
    ASSERT_FLOAT_EQ(r.x, 1.0f);
    ASSERT_FLOAT_EQ(r.y, 2.0f);
    ASSERT_FLOAT_EQ(r.w, 3.0f);
    ASSERT_FLOAT_EQ(r.h, 4.0f);
}

static void test_Rect_center(void) {
    RECT rect = {0.0f, 0.0f, 10.0f, 6.0f};
    VECTOR2 c = Rect_center(&rect);
    ASSERT_FLOAT_EQ(c.x, 5.0f);
    ASSERT_FLOAT_EQ(c.y, 3.0f);
}

/* ------------------------------------------------------------------ */
/* Quaternion                                                           */
/* ------------------------------------------------------------------ */
static void test_Quaternion_dotProduct(void) {
    QUATERNION a = {1.0f, 0.0f, 0.0f, 0.0f};
    QUATERNION b = {1.0f, 0.0f, 0.0f, 0.0f};
    ASSERT_FLOAT_EQ(Quaternion_dotProduct(&a, &b), 1.0f);

    QUATERNION c = {0.0f, 0.0f, 0.0f, 1.0f};
    ASSERT_FLOAT_EQ(Quaternion_dotProduct(&a, &c), 0.0f);
}

static void test_Quaternion_length(void) {
    QUATERNION q = {0.0f, 0.0f, 0.0f, 1.0f};
    ASSERT_FLOAT_EQ(Quaternion_length(&q), 1.0f);

    QUATERNION q2 = {1.0f, 0.0f, 0.0f, 0.0f};
    ASSERT_FLOAT_EQ(Quaternion_length(&q2), 1.0f);
}

static void test_Quaternion_normalized(void) {
    QUATERNION q = {2.0f, 0.0f, 0.0f, 0.0f};
    QUATERNION n = Quaternion_normalized(&q);
    ASSERT_FLOAT_EQ(Quaternion_length(&n), 1.0f);
    ASSERT_FLOAT_EQ(n.x, 1.0f);
}

static void test_Quaternion_slerp_endpoints(void) {
    /* slerp at t=0 should return a, at t=1 should return b */
    QUATERNION a = {0.0f, 0.0f, 0.0f, 1.0f};
    QUATERNION b = {1.0f, 0.0f, 0.0f, 0.0f};
    QUATERNION r0 = Quaternion_slerp(&a, &b, 0.0f);
    ASSERT_FLOAT_EQ(r0.x, a.x);
    ASSERT_FLOAT_EQ(r0.y, a.y);
    ASSERT_FLOAT_EQ(r0.z, a.z);
    ASSERT_FLOAT_EQ(r0.w, a.w);

    QUATERNION r1 = Quaternion_slerp(&a, &b, 1.0f);
    ASSERT_FLOAT_EQ(r1.x, b.x);
    ASSERT_FLOAT_EQ(r1.y, b.y);
    ASSERT_FLOAT_EQ(r1.z, b.z);
    ASSERT_FLOAT_EQ(r1.w, b.w);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void) {
    /* LerpNumber */
    test_LerpNumber();

    /* Vector2 */
    test_Vector2_set();
    test_Vector2_add();
    test_Vector2_sub();
    test_Vector2_scale();
    test_Vector2_dot();
    test_Vector2_len();
    test_Vector2_lengthsq();
    test_Vector2_distance();
    test_Vector2_normalize();
    test_Vector2_lerp();
    test_Vector2_mad();
    test_Vector2_unm();

    /* Vector3 */
    test_Vector3_dot();
    test_Vector3_len();
    test_Vector3_lengthsq();
    test_Vector3_distance();
    test_Vector3_cross();
    test_Vector3_add();
    test_Vector3_sub();
    test_Vector3_scale();
    test_Vector3_normalize();
    test_Vector3_lerp();
    test_Vector3_mad();
    test_Vector3_unm();
    test_Vector3_set();
    test_Vector3_clear();

    /* Box2 */
    test_Box2_center();
    test_Box2_containsPoint();
    test_Box2_moveTo();

    /* Box3 */
    test_Box3_Center();

    /* Rect */
    test_Rect_contains();
    test_Rect_scale();
    test_Rect_div();
    test_Rect_center();

    /* Quaternion */
    test_Quaternion_dotProduct();
    test_Quaternion_length();
    test_Quaternion_normalized();
    test_Quaternion_slerp_endpoints();

    TEST_SUMMARY();
    return TEST_RESULT();
}
