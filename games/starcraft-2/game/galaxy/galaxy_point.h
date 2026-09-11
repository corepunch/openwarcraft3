/* galaxy_point.h — point, region, and visibility natives */
/* PointFromId: look up map point object by ID, return typed handle. */
static DWORD sc2_PointFromId(LPJASS j) {
    DWORD map_id = (DWORD)jass_checkinteger(j, 1);
    FLOAT x = 0.0f, y = 0.0f;
    if (sc2_galaxy_get_point_by_id && sc2_gpoint_n < MAX_GALAXY_POINTS &&
        sc2_galaxy_get_point_by_id(map_id, &x, &y)) {
        LONG h = sc2_gpoint_n++;
        sc2_gpoints[h] = (sc2GPoint_t){ x, y };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "point");
    }
    if (sc2_gpoint_n >= MAX_GALAXY_POINTS)
        fprintf(stderr, "PointFromId: table full (%d entries) — id=%u lost\n",
                MAX_GALAXY_POINTS, map_id);
    return jass_pushnullhandle(j, "point");
}

/* Point(x, y): create a point from explicit coordinates. */
static DWORD sc2_Point(LPJASS j) {
    FLOAT x = jass_checknumber(j, 1), y = jass_checknumber(j, 2);
    if (sc2_gpoint_n < MAX_GALAXY_POINTS) {
        LONG h = sc2_gpoint_n++;
        sc2_gpoints[h] = (sc2GPoint_t){ x, y };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "point");
    }
    return jass_pushnullhandle(j, "point");
}

static FLOAT sc2_point_x(LPJASS j, int idx) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, idx, "point");
    return (h > 0 && h < sc2_gpoint_n) ? sc2_gpoints[h].x : 0.0f;
}
static FLOAT sc2_point_y(LPJASS j, int idx) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, idx, "point");
    return (h > 0 && h < sc2_gpoint_n) ? sc2_gpoints[h].y : 0.0f;
}

static DWORD sc2_PointGetX(LPJASS j) { return jass_pushnumber(j, sc2_point_x(j, 1)); }
static DWORD sc2_PointGetY(LPJASS j) { return jass_pushnumber(j, sc2_point_y(j, 1)); }
static DWORD sc2_PointGetHeight(LPJASS j)        { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_PointGetFacing(LPJASS j)        { return jass_pushnumber(j, 0.0f); }

/* PointWithOffset: returns a new point offset from the base. */
static DWORD sc2_PointWithOffset(LPJASS j) {
    FLOAT bx = sc2_point_x(j, 1), by = sc2_point_y(j, 1);
    FLOAT dx = jass_checknumber(j, 2), dy = jass_checknumber(j, 3);
    if (sc2_gpoint_n < MAX_GALAXY_POINTS) {
        LONG h = sc2_gpoint_n++;
        sc2_gpoints[h] = (sc2GPoint_t){ bx + dx, by + dy };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "point");
    }
    return jass_pushnullhandle(j, "point");
}

/* PointWithOffsetPolar: returns a new point offset by (dist, angle_deg). */
static DWORD sc2_PointWithOffsetPolar(LPJASS j) {
    FLOAT bx = sc2_point_x(j, 1), by = sc2_point_y(j, 1);
    FLOAT dist = jass_checknumber(j, 2);
    FLOAT ang  = jass_checknumber(j, 3) * 3.14159265f / 180.0f; /* degrees → radians */
    if (sc2_gpoint_n < MAX_GALAXY_POINTS) {
        LONG h = sc2_gpoint_n++;
        sc2_gpoints[h] = (sc2GPoint_t){ bx + dist * cosf(ang), by + dist * sinf(ang) };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "point");
    }
    return jass_pushnullhandle(j, "point");
}

static DWORD sc2_PointReflect(LPJASS j)          { return jass_pushnullhandle(j, "point"); }
static DWORD sc2_PointPathingCliffLevel(LPJASS j){ return jass_pushinteger(j, 0); }
static DWORD sc2_PointSetFacing(LPJASS j)        { (void)j; return jass_pushnull(j); }

static DWORD sc2_AngleBetweenPoints(LPJASS j) {
    FLOAT ax = sc2_point_x(j, 1), ay = sc2_point_y(j, 1);
    FLOAT bx = sc2_point_x(j, 2), by = sc2_point_y(j, 2);
    return jass_pushnumber(j, atan2f(by - ay, bx - ax) * 180.0f / 3.14159265f);
}

static DWORD sc2_DistanceBetweenPoints(LPJASS j) {
    FLOAT ax = sc2_point_x(j, 1), ay = sc2_point_y(j, 1);
    FLOAT bx = sc2_point_x(j, 2), by = sc2_point_y(j, 2);
    FLOAT dx = bx - ax, dy = by - ay;
    return jass_pushnumber(j, sqrtf(dx*dx + dy*dy));
}

static DWORD sc2_RegionEmpty(LPJASS j)           { return jass_pushnullhandle(j, "region"); }
static DWORD sc2_RegionEntireMap(LPJASS j)       { return jass_pushnullhandle(j, "region"); }
static DWORD sc2_RegionPlayableMap(LPJASS j)     { return jass_pushnullhandle(j, "region"); }
static DWORD sc2_RegionPlayableMapSet(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_RegionRect(LPJASS j)            { return jass_pushnullhandle(j, "region"); }
static DWORD sc2_RegionCircle(LPJASS j)          { return jass_pushnullhandle(j, "region"); }
static DWORD sc2_RegionAddCircle(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_RegionAddRect(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_RegionAddRegion(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_RegionAttachToUnit(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_RegionContainsPoint(LPJASS j)   { return jass_pushboolean(j, false); }
static DWORD sc2_RegionFromId(LPJASS j)          { return jass_pushnullhandle(j, "region"); }
static DWORD sc2_RegionGetAttachUnit(LPJASS j)   { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_RegionGetBoundsMax(LPJASS j)    { return jass_pushnullhandle(j, "point"); }
static DWORD sc2_RegionGetBoundsMin(LPJASS j)    { return jass_pushnullhandle(j, "point"); }
static DWORD sc2_RegionGetCenter(LPJASS j)       { return jass_pushnullhandle(j, "point"); }
static DWORD sc2_RegionGetOffset(LPJASS j)       { return jass_pushnullhandle(j, "point"); }
static DWORD sc2_RegionRandomPoint(LPJASS j)     { return jass_pushnullhandle(j, "point"); }
static DWORD sc2_RegionSetCenter(LPJASS j)       { (void)j; return jass_pushnull(j); }

static DWORD sc2_VisRevealArea(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_VisExploreArea(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_VisRevealerCreate(LPJASS j)     { return jass_pushnullhandle(j, "revealer"); }
static DWORD sc2_VisRevealerLastCreated(LPJASS j){ return jass_pushnullhandle(j, "revealer"); }
static DWORD sc2_VisRevealerDestroy(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_VisEnable(LPJASS j)             { (void)j; return jass_pushnull(j); }
