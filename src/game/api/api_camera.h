#define API_PLAYERSTATE(NAME) \
LPPLAYER NAME = G_GetPlayerByNumber(jass_getcontext(j)->unit->s.player);

extern LPPLAYER currentplayer;

DWORD SetCameraTargetController(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT xoffset = jass_checknumber(j, 2);
    //FLOAT yoffset = jass_checknumber(j, 3);
    //BOOL inheritOrientation = jass_checkboolean(j, 4);
    return 0;
}
DWORD SetCameraOrientController(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT xoffset = jass_checknumber(j, 2);
    //FLOAT yoffset = jass_checknumber(j, 3);
    return 0;
}
DWORD SetCameraPosition(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    return 0;
}
DWORD SetCameraQuickPosition(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    return 0;
}
DWORD SetCameraBounds(LPJASS j) {
    FLOAT x1 = jass_checknumber(j, 1);
    FLOAT y1 = jass_checknumber(j, 2);
    FLOAT x2 = jass_checknumber(j, 3);
    FLOAT y2 = jass_checknumber(j, 4);
    FLOAT x3 = jass_checknumber(j, 5);
    FLOAT y3 = jass_checknumber(j, 6);
    FLOAT x4 = jass_checknumber(j, 7);
    FLOAT y4 = jass_checknumber(j, 8);
    return 0;
}
DWORD StopCamera(LPJASS j) {
    return 0;
}
DWORD ResetToGameCamera(LPJASS j) {
    FLOAT duration = jass_checknumber(j, 1);
    LPGAMECLIENT gc = G_GetPlayerClientByNumber(PLAYER_NUM(currentplayer));
    gc->camera.old_state = gc->camera.state;
    gc->camera.state.viewangles = (VECTOR3) { 326, 0, 0 };
    gc->camera.state.fov = 50 * FOV_ASPECT;
    gc->camera.state.target_distance = 1650;
    gc->camera.start_time = gi.GetTime();
    gc->camera.end_time = gc->camera.start_time + (duration * 1000);
    return 0;
}
DWORD PanCameraTo(LPJASS j) {
    LPGAMECLIENT gc = G_GetPlayerClientByNumber(PLAYER_NUM(currentplayer));
    gc->camera.state.position.x = jass_checknumber(j, 1);
    gc->camera.state.position.y = jass_checknumber(j, 2);
    gc->camera.end_time = gc->camera.start_time;
    return 0;
}
DWORD PanCameraToTimed(LPJASS j) {
    FLOAT duration = jass_checknumber(j, 3);
    LPGAMECLIENT gc = G_GetPlayerClientByNumber(PLAYER_NUM(currentplayer));
    gc->camera.state.position.x = jass_checknumber(j, 1);
    gc->camera.state.position.y = jass_checknumber(j, 2);
    gc->camera.end_time = gc->camera.start_time + (duration * 1000);
    return 0;
}
DWORD PanCameraToWithZ(LPJASS j) {
    LPGAMECLIENT gc = G_GetPlayerClientByNumber(PLAYER_NUM(currentplayer));
    gc->camera.state.position.x = jass_checknumber(j, 1);
    gc->camera.state.position.y = jass_checknumber(j, 2);
    gc->camera.end_time = gc->camera.start_time;
    //FLOAT zOffsetDest = jass_checknumber(j, 3);
    return 0;
}
DWORD PanCameraToTimedWithZ(LPJASS j) {
    FLOAT duration = jass_checknumber(j, 4);
    LPGAMECLIENT gc = G_GetPlayerClientByNumber(PLAYER_NUM(currentplayer));
    gc->camera.state.position.x = jass_checknumber(j, 1);
    gc->camera.state.position.y = jass_checknumber(j, 2);
    gc->camera.end_time = gc->camera.start_time + (duration * 1000);
    //FLOAT zOffsetDest = jass_checknumber(j, 3);
    return 0;
}
DWORD SetCinematicCamera(LPJASS j) {
    //LPCSTR cameraModelFile = jass_checkstring(j, 1);
    return 0;
}
DWORD SetCameraField(LPJASS j) {
    //HANDLE whichField = jass_checkhandle(j, 1, "camerafield");
    //FLOAT value = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD AdjustCameraField(LPJASS j) {
    //HANDLE whichField = jass_checkhandle(j, 1, "camerafield");
    //FLOAT offset = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD CreateCameraSetup(LPJASS j) {
    API_ALLOC(CAMERASETUP, camerasetup);
    return 1;
}
DWORD CameraSetupSetField(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    CAMERAFIELD *whichField = jass_checkhandle(j, 2, "camerafield");
    FLOAT value = jass_checknumber(j, 3);
    switch (*whichField) {
        case CAMERA_FIELD_TARGET_DISTANCE: whichSetup->target_distance = value; break;
        case CAMERA_FIELD_FARZ: whichSetup->far_z = value; break;
        case CAMERA_FIELD_ANGLE_OF_ATTACK: whichSetup->viewangles.x = -90 - value; break;
        case CAMERA_FIELD_FIELD_OF_VIEW: whichSetup->fov = value; break;
        case CAMERA_FIELD_ROLL: whichSetup->viewangles.y = value; break;
        case CAMERA_FIELD_ROTATION: whichSetup->viewangles.z = 90 - value; break;
        case CAMERA_FIELD_ZOFFSET: whichSetup->z_offset = value; break;
    }
//    FLOAT duration = jass_checknumber(j, 4);
    return 0;
}
DWORD CameraSetupGetField(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    DWORD *whichField = jass_checkhandle(j, 2, "camerafield");
    FLOAT value = 0;
    switch (*whichField) {
        case CAMERA_FIELD_TARGET_DISTANCE: value = whichSetup->target_distance; break;
        case CAMERA_FIELD_FARZ: value = whichSetup->far_z; break;
        case CAMERA_FIELD_ANGLE_OF_ATTACK: value = -90 - whichSetup->viewangles.x; break;
        case CAMERA_FIELD_FIELD_OF_VIEW: value = whichSetup->fov; break;
        case CAMERA_FIELD_ROLL: value = whichSetup->viewangles.y; break;
        case CAMERA_FIELD_ROTATION: value = 90 - whichSetup->viewangles.z; break;
        case CAMERA_FIELD_ZOFFSET: value = whichSetup->z_offset; break;
    }
    return jass_pushnumber(j, value);
}
DWORD CameraSetupSetDestPosition(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    FLOAT x = jass_checknumber(j, 2);
    FLOAT y = jass_checknumber(j, 3);
//    FLOAT duration = jass_checknumber(j, 4);
    whichSetup->position.x = x;
    whichSetup->position.y = y;
    return 0;
}
DWORD CameraSetupGetDestPositionLoc(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    return jass_pushlighthandle(j, &whichSetup->position, "location");
}
DWORD CameraSetupGetDestPositionX(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    return jass_pushnumber(j, whichSetup->position.x);
}
DWORD CameraSetupGetDestPositionY(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    return jass_pushnumber(j, whichSetup->position.y);
}
DWORD CameraSetupApply(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //BOOL doPan = jass_checkboolean(j, 2);
    //BOOL panTimed = jass_checkboolean(j, 3);
    return 0;
}
DWORD CameraSetupApplyWithZ(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //FLOAT zDestOffset = jass_checknumber(j, 2);
    return 0;
}
DWORD CameraSetupApplyForceDuration(LPJASS j) {
    LPCAMERASETUP whichSetup = jass_checkhandle(j, 1, "camerasetup");
    BOOL doPan = jass_checkboolean(j, 2);
    FLOAT forceDuration = jass_checknumber(j, 3);
    LPGAMECLIENT gc = G_GetPlayerClientByNumber(PLAYER_NUM(currentplayer));
    gc->camera.old_state = gc->camera.state;
    gc->camera.state = *whichSetup;
    gc->camera.start_time = gi.GetTime();
    gc->camera.end_time = gc->camera.start_time + (doPan ? forceDuration * 1000 : 0);
    return 0;
}
DWORD CameraSetupApplyForceDurationWithZ(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //FLOAT zDestOffset = jass_checknumber(j, 2);
    //FLOAT forceDuration = jass_checknumber(j, 3);
    return 0;
}
DWORD CameraSetTargetNoise(LPJASS j) {
    //FLOAT mag = jass_checknumber(j, 1);
    //FLOAT velocity = jass_checknumber(j, 2);
    return 0;
}
DWORD CameraSetSourceNoise(LPJASS j) {
    //FLOAT mag = jass_checknumber(j, 1);
    //FLOAT velocity = jass_checknumber(j, 2);
    return 0;
}
DWORD CameraSetSmoothingFactor(LPJASS j) {
    //FLOAT factor = jass_checknumber(j, 1);
    return 0;
}
DWORD GetCameraMargin(LPJASS j) {
    LONG whichMargin = jass_checkinteger(j, 1);
    mapCameraBounds_t const *bounds = &level.mapinfo->cameraBounds;
    switch (whichMargin) {
        case 0: jass_pushnumber(j, bounds->margin.left * TILE_SIZE); break;
        case 1: jass_pushnumber(j, bounds->margin.right * TILE_SIZE); break;
        case 2: jass_pushnumber(j, bounds->margin.top * TILE_SIZE); break;
        case 3: jass_pushnumber(j, bounds->margin.bottom * TILE_SIZE); break;
        default: jass_pushnull(j);
    }
    return 1;
}
DWORD GetCameraBoundMinX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraBoundMinY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraBoundMaxX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraBoundMaxY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraField(LPJASS j) {
    //HANDLE whichField = jass_checkhandle(j, 1, "camerafield");
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionZ(LPJASS j) {
    return jass_pushnumber(j, 0);
}

DWORD GetCameraTargetPositionLoc(LPJASS j) {
    API_ALLOC(VECTOR2, location);
    API_PLAYERSTATE(playerstate);
    *location = playerstate->origin;
    return 1;
}
DWORD GetCameraEyePositionX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraEyePositionY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraEyePositionZ(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraEyePositionLoc(LPJASS j) {
    return jass_pushnullhandle(j, "location");
}
