#include "../cmath3.h"

int Line3_intersect_sphere3(LPCLINE3 line, LPCSPHERE3 sphere, LPVECTOR3 output) {
    float cx = sphere->center.x;
    float cy = sphere->center.y;
    float cz = sphere->center.z;
    float px = line->a.x;
    float py = line->a.y;
    float pz = line->a.z;
    float vx = line->b.x - px;
    float vy = line->b.y - py;
    float vz = line->b.z - pz;
    float A = vx * vx + vy * vy + vz * vz;
    float B = 2.0 * (px * vx + py * vy + pz * vz - vx * cx - vy * cy - vz * cz);
    float C = px * px - 2 * px * cx + cx * cx + py * py - 2 * py * cy + cy * cy +
               pz * pz - 2 * pz * cz + cz * cz - sphere->radius * sphere->radius;
    float D = B * B - 4 * A * C;

    if ( D < 0 ) {
        return 0;
    } else if (!output) {
        return 1;
    }

    float t1 = ( -B - sqrtf ( D ) ) / ( 2.0 * A );

    VECTOR3 solution1 = {
        line->a.x * ( 1 - t1 ) + t1 * line->b.x,
        line->a.y * ( 1 - t1 ) + t1 * line->b.y,
        line->a.z * ( 1 - t1 ) + t1 * line->b.z
    };

    if ( D == 0 ) {
        *output = solution1;
        return 1;
    }

    float t2 = ( -B + sqrtf( D ) ) / ( 2.0 * A );
    VECTOR3 solution2 = {
        line->a.x * ( 1 - t2 ) + t2 * line->b.x,
        line->a.y * ( 1 - t2 ) + t2 * line->b.y,
        line->a.z * ( 1 - t2 ) + t2 * line->b.z
    };

    if ( fabs( t1 - 0.5 ) < fabs( t2 - 0.5 ) ) {
        *output = solution1;
        return 1;
    }

    *output = solution2;
    return 1;
}

int Line3_intersect_plane3(LPCLINE3 line, LPCPLANE3 plane, LPVECTOR3 output) {
    VECTOR3 lineDirection = Vector3_sub(&line->b, &line->a);
    if (Vector3_dot(&plane->normal, &lineDirection) == 0)
        return 0;
    Vector3_normalize(&lineDirection);
    float const p1 = Vector3_dot(&plane->normal, &plane->point);
    float const p2 = Vector3_dot(&plane->normal, &line->a);
    float const t = (p1 - p2) / Vector3_dot(&plane->normal, &lineDirection);
    VECTOR3 const scaledLineDirection = Vector3_scale(&lineDirection, t);
    *output = Vector3_add(&line->a, &scaledLineDirection);
    return 1;
}

static inline float DotNormal(LPCVECTOR3 normal, LPCVECTOR3 a, LPCVECTOR3 b, LPCVECTOR3 c) {
    VECTOR3 const rcross = Triangle_normal(&(const TRIANGLE3) { *a, *b, *c });
    return Vector3_dot(normal, &rcross);
}

int Line3_intersect_triangle(LPCLINE3 line, LPCTRIANGLE3 triangle, LPVECTOR3 output) {
    VECTOR3 const normal = Triangle_normal(triangle);
    VECTOR3 const diff1 = Vector3_sub(&line->a, &triangle->a);
    VECTOR3 const diff2 = Vector3_sub(&line->b, &triangle->a);
    float const r1 = Vector3_dot(&normal, &diff1);
    float const r2 = Vector3_dot(&normal, &diff2);
    if ((r1 > 0) == (r2 > 0))
        return 0;
    VECTOR3 const distance = Vector3_sub(&line->a, &line->b);
    VECTOR3 const pc = Vector3_mad(&line->a, r1 / (r2 - r1), &distance);
    if (DotNormal(&normal, &triangle->a, &triangle->b, &pc) < 0)
        return 0;
    if (DotNormal(&normal, &triangle->b, &triangle->c, &pc) < 0)
        return 0;
    if (DotNormal(&normal, &triangle->c, &triangle->a, &pc) < 0)
        return 0;
    if (output)
        *output = pc;
    return 1;
}
//
//bool intersect_triangle(
//    in Ray R, in vec3 A, in vec3 B, in vec3 C, out float t,
//    out float u, out float v, out vec3 N
//) {
//   vec3 E1 = B-A;
//   vec3 E2 = C-A;
//         N = cross(E1,E2);
//   float det = -dot(R.Dir, N);
//   float invdet = 1.0/det;
//   vec3 AO  = R.Origin - A;
//   vec3 DAO = cross(AO, R.Dir);
//   u =  dot(E2,DAO) * invdet;
//   v = -dot(E1,DAO) * invdet;
//   t =  dot(AO,N)  * invdet;
//   return (det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u+v) <= 1.0);
//}

int Line3_intersect_box3(LPCLINE3 line, LPCBOX3 box, LPVECTOR3 output) {
    float st, et, fst = 0, fet = 1;
    float const *bmin = &box->min.x;
    float const *bmax = &box->max.x;
    float const *si = &line->a.x;
    float const *ei = &line->b.x;
    for (int i = 0; i < 3; i++) {
        if (*si < *ei) {
            if (*si > *bmax || *ei < *bmin)
                return 0;
            float di = *ei - *si;
            st = (*si < *bmin)? (*bmin - *si) / di: 0;
            et = (*ei > *bmax)? (*bmax - *si) / di: 1;
        }
        else {
            if (*ei > *bmax || *si < *bmin)
                return 0;
            float di = *ei - *si;
            st = (*si > *bmax)? (*bmax - *si) / di: 0;
            et = (*ei < *bmin)? (*bmin - *si) / di: 1;
        }

        if (st > fst) fst = st;
        if (et < fet) fet = et;
        if (fet < fst)
            return 0;
        bmin++; bmax++;
        si++; ei++;
    }

    if (output) {
        *output = Vector3_lerp(&line->a, &line->b, fst);
    }
    return 1;
}
