/* galaxy_camera.h — camera natives */

#define MAX_GALAXY_CAMS   64
typedef struct { FLOAT tx, ty, tz, pitch, yaw, dist, fov, height; } sc2GCam_t;
static sc2GCam_t sc2_gcams[MAX_GALAXY_CAMS];
static LONG sc2_gcam_n = 1;    /* 1-based; 0 = null handle */

/* CameraInfoFromId: look up map camera by ID, store in local table, return handle. */
static DWORD sc2_CameraInfoFromId(LPJASS j) {
    DWORD map_id = (DWORD)jass_checkinteger(j, 1);
    FLOAT tx = 0, ty = 0, tz = 0, pitch = 56.0f, yaw = 180.0f, dist = 34.0f, fov = 28.0f, height = 0;
    if (sc2_galaxy_get_camera_by_id &&
        sc2_gcam_n < MAX_GALAXY_CAMS &&
        sc2_galaxy_get_camera_by_id(map_id, &tx, &ty, &tz, &pitch, &yaw, &dist, &fov, &height)) {
        LONG h = sc2_gcam_n++;
        sc2_gcams[h] = (sc2GCam_t){ tx, ty, tz, pitch, yaw, dist, fov, height };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "camerainfo");
    }
    return jass_pushnullhandle(j, "camerainfo");
}

static DWORD sc2_CameraInfoDefault(LPJASS j) { return jass_pushnullhandle(j, "camerainfo"); }

/* CameraApplyInfo: apply stored camera to client (duration = 0 → instant snap). */
static DWORD sc2_CameraApplyInfo(LPJASS j) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, 2, "camerainfo");
    FLOAT dur = jass_checknumber(j, 3);
#ifdef SC2_DEBUG_CUTSCENE
    fprintf(stderr, "CameraApplyInfo: handle=%ld count=%ld duration=%.2f callback=%d\n",
            (long)h, (long)sc2_gcam_n, dur, sc2_galaxy_on_camera != NULL);
#endif
    if (h > 0 && h < sc2_gcam_n && sc2_galaxy_on_camera) {
        sc2GCam_t *c = &sc2_gcams[h];
        sc2_galaxy_on_camera(c->tx, c->ty, c->yaw, c->pitch, c->dist, c->fov, c->height, dur);
    }
    return jass_pushnull(j);
}

static DWORD sc2_CameraPan(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_CameraSave(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_CameraRestore(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_CameraLockInput(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_CameraShakeStart(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_CameraGetTarget(LPJASS j)   { return jass_pushinteger(j, 0); }
