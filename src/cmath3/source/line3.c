#include "../cmath3.h"

int Line3_intersect_sphere3(LPCLINE3 lpLine, LPCSPHERE3 lpSphere, LPVECTOR3 lpOutput) {
    // http://www.codeproject.com/Articles/19799/Simple-Ray-Tracing-in-C-Part-II-Triangles-Intersec

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

    // discriminant
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

    // prefer a solution that's on the line segment itself

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

int Line3_intersect_tristrip(LPCLINE3 lpLine, LPCVECTOR3 lpVertices, int numVertices, LPVECTOR3 lpOutput) {
    VECTOR3 const side1 = Vector3_sub(&lpVertices[0], &lpVertices[1]);
    VECTOR3 const side2 = Vector3_sub(&lpVertices[1], &lpVertices[2]);
    VECTOR3 const normal = Vector3_cross(&side1, &side2);
    VECTOR3 const diff1 = Vector3_sub(&lpLine->a, lpVertices);
    VECTOR3 const diff2 = Vector3_sub(&lpLine->b, lpVertices);
    float const r1 = Vector3_dot(&normal, &diff1);
    float const r2 = Vector3_dot(&normal, &diff2);
    if ((r1 > 0) == (r2 > 0))
        return 0;
    VECTOR3 const distance = Vector3_sub(&lpLine->a, &lpLine->b);
    VECTOR3 const pc = Vector3_mad(&lpLine->a, r1 / (r2 - r1), &distance);
    for (int i = 1; i <= numVertices; i++) {
        VECTOR3 const tside1 = Vector3_sub(&lpVertices[i], &lpVertices[i-1]);
        VECTOR3 const tside2 = Vector3_sub(&pc, &lpVertices[i-1]);
        VECTOR3 const rcross = Vector3_cross(&tside1, &tside2);
        if (Vector3_dot(&normal, &rcross) < 0)
            return 0;
    }
    if (lpOutput)
        *lpOutput = pc;
    return 1;
}
