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

static float interpNum(LPCMODELKEYFRAME lpLeft, LPCMODELKEYFRAME lpRight, DWORD dwTime, MODELKEYTRACKTYPE dwLineType) {
    float const *lpLeftVector = (float const *)lpLeft->data;
    float const *lpRightVector = (float const *)lpRight->data;
    float const t = (float)(dwTime - lpLeft->time) / (float)(lpRight->time - lpLeft->time);
    if (lpLeft->time == lpRight->time) {
        return lpLeftVector[KEY_VALUE];
    }
    switch (dwLineType) {
        case TRACK_NO_INTERP:
            return lpLeftVector[KEY_VALUE];
        case TRACK_BEZIER:
            return bezier(lpLeftVector[KEY_VALUE], lpLeftVector[KEY_OUTTAN], lpRightVector[KEY_INTAN], lpRightVector[KEY_VALUE], t);
        case TRACK_HERMITE:
            return hermite(lpLeftVector[KEY_VALUE], lpLeftVector[KEY_OUTTAN], lpRightVector[KEY_INTAN], lpRightVector[KEY_VALUE], t);
        default:
            return lerp(lpLeftVector[KEY_VALUE], lpRightVector[KEY_VALUE], t);
    }
}

static VECTOR3 interpVec3(LPCMODELKEYFRAME lpLeft, LPCMODELKEYFRAME lpRight, DWORD dwTime, MODELKEYTRACKTYPE dwLineType) {
    LPCVECTOR3 lpLeftVector = (LPCVECTOR3)lpLeft->data;
    LPCVECTOR3 lpRightVector = (LPCVECTOR3)lpRight->data;
    float const t = (float)(dwTime - lpLeft->time) / (float)(lpRight->time - lpLeft->time);
    if (lpLeft->time == lpRight->time) {
        return lpLeftVector[KEY_VALUE];
    }
    switch (dwLineType) {
        case TRACK_NO_INTERP:
            return lpLeftVector[KEY_VALUE];
        case TRACK_BEZIER:
            return Vector3_bezier(&lpLeftVector[KEY_VALUE], &lpLeftVector[KEY_OUTTAN], &lpRightVector[KEY_INTAN], &lpRightVector[KEY_VALUE], t);
        case TRACK_HERMITE:
            return Vector3_hermite(&lpLeftVector[KEY_VALUE], &lpLeftVector[KEY_OUTTAN], &lpRightVector[KEY_INTAN], &lpRightVector[KEY_VALUE], t);
        default:
            return Vector3_lerp(&lpLeftVector[KEY_VALUE], &lpRightVector[KEY_VALUE], t);
    }
}

static QUATERNION interpQuat(LPCMODELKEYFRAME lpLeft, LPCMODELKEYFRAME lpRight, DWORD dwTime, MODELKEYTRACKTYPE dwLineType) {
    LPCQUATERNION lpLeftVector = (LPCQUATERNION)lpLeft->data;
    LPCQUATERNION lpRightVector = (LPCQUATERNION)lpRight->data;
    float const t = (float)(dwTime - lpLeft->time) / (float)(lpRight->time - lpLeft->time);
    if (lpLeft->time == lpRight->time) {
        return lpLeftVector[KEY_VALUE];
    }
    switch (dwLineType) {
        case TRACK_NO_INTERP:
            return lpLeftVector[KEY_VALUE];
        case TRACK_BEZIER:
        case TRACK_HERMITE:
            return Quaternion_sqlerp(&lpLeftVector[KEY_VALUE], &lpLeftVector[KEY_OUTTAN], &lpRightVector[KEY_INTAN], &lpRightVector[KEY_VALUE], t);
        default:
            return Quaternion_slerp(&lpLeftVector[KEY_VALUE], &lpRightVector[KEY_VALUE], t);
    }
}

void R_GetKeyframeValue(LPCMODELKEYFRAME lpLeft, LPCMODELKEYFRAME lpRight, DWORD dwTime, LPCMODELKEYTRACK lpKeytrack, HANDLE out) {
    switch (lpKeytrack->datatype) {
        case kModelKeyTrackDataTypeFloat:
            *((float *)out) = interpNum(lpLeft, lpRight, dwTime, lpKeytrack->type);
            return;
        case kModelKeyTrackDataTypeVector3:
            *((VECTOR3 *)out) = interpVec3(lpLeft, lpRight, dwTime, lpKeytrack->type);
            return;
        case kModelKeyTrackDataTypeQuaternion:
            *((QUATERNION *)out) = interpQuat(lpLeft, lpRight, dwTime, lpKeytrack->type);
            return;
    }
}
