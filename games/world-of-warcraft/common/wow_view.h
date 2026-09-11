#ifndef WOW_VIEW_H
#define WOW_VIEW_H

#include "common/shared.h"

/* WoW camera interpolation wraps in degrees; linear interpolation would spin the long way across 0/360. */
static FLOAT Wow_LerpDegrees(FLOAT a, FLOAT b, FLOAT t) {
    FLOAT delta = fmodf(b - a, 360.0f);
    if (delta > 180.0f) delta -= 360.0f;
    else if (delta < -180.0f) delta += 360.0f;
    return a + delta * t;
}

/* WoW uses downward pitch from the horizon and heading from +X. The orbit view uses tilt
 * from -Z and inverse heading from +Y; copying native angles made the camera overhead/sideways. */
static VECTOR3 Wow_EulerFromCamera(FLOAT pitch, FLOAT yaw) {
    return (VECTOR3){ pitch - 90.0f, 0.0f, 90.0f - yaw };
}

/* Native {downward pitch, heading, roll}; keep movement heading out of view-matrix coordinates. */
static VECTOR3 Wow_CameraFromEuler(LPCVECTOR3 euler) {
    return (VECTOR3){ euler->x + 90.0f, 90.0f - euler->z, euler->y };
}

/* Native camera angles are {downward pitch, heading, roll} in degrees, Z-up. */
static VECTOR3 Wow_ViewForward(LPCVECTOR3 angles) {
    FLOAT yaw = (FLOAT)DEG2RAD(angles->y), pitch = (FLOAT)DEG2RAD(angles->x);
    return (VECTOR3){ cosf(pitch) * cosf(yaw), cosf(pitch) * sinf(yaw), -sinf(pitch) };
}

/* Shadow callers share the same no-cull override for both cheap and terrain-adjusted bounds. */
static BOOL Wow_ShadowBoundsVisible(LPCFRUSTUM3 frustum, LPCBOX3 bounds, BOOL cull) {
    return !cull || Frustum_ContainsAABox(frustum, bounds);
}

#endif
