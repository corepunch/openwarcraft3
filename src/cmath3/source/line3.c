#include "../cmath3.h"

int Line_intersect_sphere(LPCLINE3 lpLine, LPCSPHERE3 lpSphere, LPVECTOR3 lpOutput) {
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
