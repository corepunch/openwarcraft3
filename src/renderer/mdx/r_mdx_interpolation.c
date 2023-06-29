#include "r_local.h"
#include "r_mdx.h"

enum {
    KEY_VALUE,
    KEY_INTAN,
    KEY_OUTTAN,
};

static float lerp(float left, float right, float t) {
    return left * (1 - t) + right * t;
}

static float bezier(float left, float outTan, float inTan, float right, float t) {
    float const inverseFactor = 1 - t,
        inverseFactorTimesTwo = inverseFactor * inverseFactor,
        factorTimes2 = t * t,
        factor1 = inverseFactorTimesTwo * inverseFactor,
        factor2 = 3 * t * inverseFactorTimesTwo,
        factor3 = 3 * factorTimes2 * inverseFactor,
        factor4 = factorTimes2 * t;
    return left * factor1 + outTan * factor2 + inTan * factor3 + right * factor4;
}

static float hermite(float left, float outTan, float inTan, float right, float t) {
    float const factorTimes2 = t * t,
        factor1 = factorTimes2 * (2 * t - 3) + 1,
        factor2 = factorTimes2 * (t - 2) + t,
        factor3 = factorTimes2 * (t - 1),
        factor4 = factorTimes2 * (3 - 2 * t);
    return left * factor1 + outTan * factor2 + inTan * factor3 + right * factor4;
}

static int interpInt(mdxKeyFrame_t const *left, mdxKeyFrame_t const *right, DWORD time, MODELKEYTRACKTYPE lineType) {
    int const *leftVector = (int const *)left->data;
    int const *rightVector = (int const *)right->data;
    float const t = (float)(time - left->time) / (float)(right->time - left->time);
    if (left->time == right->time) {
        return leftVector[KEY_VALUE];
    }
    switch (lineType) {
        case TRACK_NO_INTERP:
            return leftVector[KEY_VALUE];
        case TRACK_BEZIER:
            return bezier(leftVector[KEY_VALUE], leftVector[KEY_OUTTAN], rightVector[KEY_INTAN], rightVector[KEY_VALUE], t);
        case TRACK_HERMITE:
            return hermite(leftVector[KEY_VALUE], leftVector[KEY_OUTTAN], rightVector[KEY_INTAN], rightVector[KEY_VALUE], t);
        default:
            return lerp(leftVector[KEY_VALUE], rightVector[KEY_VALUE], t);
    }
}

static float interpFloat(mdxKeyFrame_t const *left, mdxKeyFrame_t const *right, DWORD time, MODELKEYTRACKTYPE lineType) {
    float const *leftVector = (float const *)left->data;
    float const *rightVector = (float const *)right->data;
    float const t = (float)(time - left->time) / (float)(right->time - left->time);
    if (left->time == right->time) {
        return leftVector[KEY_VALUE];
    }
    switch (lineType) {
        case TRACK_NO_INTERP:
            return leftVector[KEY_VALUE];
        case TRACK_BEZIER:
            return bezier(leftVector[KEY_VALUE], leftVector[KEY_OUTTAN], rightVector[KEY_INTAN], rightVector[KEY_VALUE], t);
        case TRACK_HERMITE:
            return hermite(leftVector[KEY_VALUE], leftVector[KEY_OUTTAN], rightVector[KEY_INTAN], rightVector[KEY_VALUE], t);
        default:
            return lerp(leftVector[KEY_VALUE], rightVector[KEY_VALUE], t);
    }
}

static VECTOR3 interpVec3(mdxKeyFrame_t const *left, mdxKeyFrame_t const *right, DWORD time, MODELKEYTRACKTYPE lineType) {
    LPCVECTOR3 leftVector = (LPCVECTOR3)left->data;
    LPCVECTOR3 rightVector = (LPCVECTOR3)right->data;
    float const t = (float)(time - left->time) / (float)(right->time - left->time);
    if (left->time == right->time) {
        return leftVector[KEY_VALUE];
    }
    switch (lineType) {
        case TRACK_NO_INTERP:
            return leftVector[KEY_VALUE];
        case TRACK_BEZIER:
            return Vector3_bezier(&leftVector[KEY_VALUE], &leftVector[KEY_OUTTAN], &rightVector[KEY_INTAN], &rightVector[KEY_VALUE], t);
        case TRACK_HERMITE:
            return Vector3_hermite(&leftVector[KEY_VALUE], &leftVector[KEY_OUTTAN], &rightVector[KEY_INTAN], &rightVector[KEY_VALUE], t);
        default:
            return Vector3_lerp(&leftVector[KEY_VALUE], &rightVector[KEY_VALUE], t);
    }
}

static QUATERNION interpQuat(mdxKeyFrame_t const *left, mdxKeyFrame_t const *right, DWORD time, MODELKEYTRACKTYPE lineType) {
    LPCQUATERNION leftVector = (LPCQUATERNION)left->data;
    LPCQUATERNION rightVector = (LPCQUATERNION)right->data;
    float const t = (float)(time - left->time) / (float)(right->time - left->time);
    if (left->time == right->time) {
        return leftVector[KEY_VALUE];
    }
    switch (lineType) {
        case TRACK_NO_INTERP:
            return leftVector[KEY_VALUE];
        case TRACK_BEZIER:
        case TRACK_HERMITE:
            return Quaternion_sqlerp(&leftVector[KEY_VALUE], &leftVector[KEY_OUTTAN], &rightVector[KEY_INTAN], &rightVector[KEY_VALUE], t);
        default:
            return Quaternion_slerp(&leftVector[KEY_VALUE], &rightVector[KEY_VALUE], t);
    }
}

void R_GetKeyframeValue(mdxKeyFrame_t const *left, mdxKeyFrame_t const *right, DWORD time, mdxKeyTrack_t const *keytrack, HANDLE out) {
    switch (keytrack->datatype) {
        case TDATA_INT1:
            *((int *)out) = interpInt(left, right, time, keytrack->type);
            return;
        case TDATA_FLOAT1:
            *((float *)out) = interpFloat(left, right, time, keytrack->type);
            return;
        case TDATA_FLOAT3:
            *((VECTOR3 *)out) = interpVec3(left, right, time, keytrack->type);
            return;
        case TDATA_FLOAT4:
            *((QUATERNION *)out) = interpQuat(left, right, time, keytrack->type);
            return;
    }
}
