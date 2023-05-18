#ifndef quaternion_h
#define quaternion_h

struct quaternion { float x, y, z, w; };

typedef struct quaternion QUATERNION;
typedef struct quaternion *LPQUATERNION;
typedef struct quaternion const *LPCQUATERNION;

QUATERNION Quaternion_slerp(LPCQUATERNION p, LPCQUATERNION q, float t);
QUATERNION Quaternion_sqlerp(LPCQUATERNION a, LPCQUATERNION b, LPCQUATERNION c, LPCQUATERNION d, float t);

#endif
