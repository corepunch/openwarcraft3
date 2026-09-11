#ifndef BZ_SERVER_ROUTING_H
#define BZ_SERVER_ROUTING_H

#define BZ_ROUTE_SLIDE_STEP (15.0f * (FLOAT)M_PI / 180.0f) // radians; WC3 deflection increment; used by local steering
#define BZ_ROUTE_SLIDE_RINGS 6 // steps/side; WC3 searches through 90 degrees; bounds local steering

#define BZ_PATH_WORK_BUDGET 32768 // queue pops/tick; WC3 default completes a 256x256 open field in two ticks

typedef struct {
    VECTOR2 waypoint, target;
    FLOAT radius;
    BOOL valid;
} ROUTEPATH;
typedef ROUTEPATH *LPROUTEPATH;
typedef ROUTEPATH const *LPCROUTEPATH;

typedef struct {
    LPEDICT ent;
    FLOAT angle, dist;
    int rings;
    BOOL (*valid)(LPEDICT ent, LPCVECTOR2 point);
} ROUTESLIDE;
typedef ROUTESLIDE *LPROUTESLIDE;
typedef ROUTESLIDE const *LPCROUTESLIDE;

FLOAT CM_SlideRoute(LPCROUTESLIDE slide);
BOOL CM_AccelerateRoute(LPROUTEPATH path, pathAccelParams_t const *params, LPVECTOR2 dir);
#endif
