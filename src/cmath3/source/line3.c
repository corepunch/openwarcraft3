#include "../cmath3.h"

int Line3_intersect_sphere3(LPCLINE3 lpLine, LPCSPHERE3 lpSphere, LPVECTOR3 lpOutput) {
    float cx = lpSphere->center.x;
    float cy = lpSphere->center.y;
    float cz = lpSphere->center.z;
    float px = lpLine->a.x;
    float py = lpLine->a.y;
    float pz = lpLine->a.z;
    float vx = lpLine->b.x - px;
    float vy = lpLine->b.y - py;
    float vz = lpLine->b.z - pz;
    float A = vx * vx + vy * vy + vz * vz;
    float B = 2.0 * (px * vx + py * vy + pz * vz - vx * cx - vy * cy - vz * cz);
    float C = px * px - 2 * px * cx + cx * cx + py * py - 2 * py * cy + cy * cy +
               pz * pz - 2 * pz * cz + cz * cz - lpSphere->radius * lpSphere->radius;
    float D = B * B - 4 * A * C;

    if ( D < 0 ) {
        return 0;
    } else if (!lpOutput) {
        return 1;
    }

    float t1 = ( -B - sqrtf ( D ) ) / ( 2.0 * A );

    VECTOR3 solution1 = {
        lpLine->a.x * ( 1 - t1 ) + t1 * lpLine->b.x,
        lpLine->a.y * ( 1 - t1 ) + t1 * lpLine->b.y,
        lpLine->a.z * ( 1 - t1 ) + t1 * lpLine->b.z
    };
    
    if ( D == 0 ) {
        *lpOutput = solution1;
        return 1;
    }

    float t2 = ( -B + sqrtf( D ) ) / ( 2.0 * A );
    VECTOR3 solution2 = {
        lpLine->a.x * ( 1 - t2 ) + t2 * lpLine->b.x,
        lpLine->a.y * ( 1 - t2 ) + t2 * lpLine->b.y,
        lpLine->a.z * ( 1 - t2 ) + t2 * lpLine->b.z
    };

    if ( fabs( t1 - 0.5 ) < fabs( t2 - 0.5 ) ) {
        *lpOutput = solution1;
        return 1;
    }

    *lpOutput = solution2;
    return 1;
}

int Line3_intersect_plane3(LPCLINE3 lpLine, LPCPLANE3 lpPlane, LPVECTOR3 lpOutput) {
    VECTOR3 lineDirection = Vector3_sub(&lpLine->b, &lpLine->a);
    if (Vector3_dot(&lpPlane->normal, &lineDirection) == 0)
        return 0;
    Vector3_normalize(&lineDirection);
    float const p1 = Vector3_dot(&lpPlane->normal, &lpPlane->point);
    float const p2 = Vector3_dot(&lpPlane->normal, &lpLine->a);
    float const t = (p1 - p2) / Vector3_dot(&lpPlane->normal, &lineDirection);
    VECTOR3 const scaledLineDirection = Vector3_scale(&lineDirection, t);
    *lpOutput = Vector3_add(&lpLine->a, &scaledLineDirection);
    return 1;
}

static inline int DotNormal(LPCVECTOR3 normal, LPCVECTOR3 a, LPCVECTOR3 b, LPCVECTOR3 c) {
    VECTOR3 const rcross = Triangle_normal(&(const TRIANGLE3) { *a, *b, *c });
    return Vector3_dot(normal, &rcross);
}

int Line3_intersect_triangle(LPCLINE3 lpLine, LPCTRIANGLE3 lpTriangle, LPVECTOR3 lpOutput) {
    VECTOR3 const normal = Triangle_normal(lpTriangle);
    VECTOR3 const diff1 = Vector3_sub(&lpLine->a, &lpTriangle->a);
    VECTOR3 const diff2 = Vector3_sub(&lpLine->b, &lpTriangle->a);
    float const r1 = Vector3_dot(&normal, &diff1);
    float const r2 = Vector3_dot(&normal, &diff2);
    if ((r1 > 0) == (r2 > 0))
        return 0;
    VECTOR3 const distance = Vector3_sub(&lpLine->a, &lpLine->b);
    VECTOR3 const pc = Vector3_mad(&lpLine->a, r1 / (r2 - r1), &distance);
    if (DotNormal(&normal, &lpTriangle->a, &lpTriangle->b, &pc) < 0)
        return 0;
    if (DotNormal(&normal, &lpTriangle->b, &lpTriangle->c, &pc) < 0)
        return 0;
    if (DotNormal(&normal, &lpTriangle->c, &lpTriangle->a, &pc) < 0)
        return 0;
    if (lpOutput)
        *lpOutput = pc;
    return 1;
}

int Line3_intersect_box3(LPCLINE3 lpLine, LPCBOX3 lpBox, LPVECTOR3 lpOutput) {
    float st, et, fst = 0, fet = 1;
    float const *bmin = &lpBox->min.x;
    float const *bmax = &lpBox->max.x;
    float const *si = &lpLine->a.x;
    float const *ei = &lpLine->b.x;
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

    if (lpOutput) {
        *lpOutput = Vector3_lerp(&lpLine->a, &lpLine->b, fst);
    }
    return 1;
}
