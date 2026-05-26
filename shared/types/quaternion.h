#ifndef quaternion_h
#define quaternion_h

typedef enum {
    ROTATE_XYZ,
    ROTATE_XZY,
    ROTATE_YZX,
    ROTATE_YXZ,
    ROTATE_ZXY,
    ROTATE_ZYX
} ROTATIONORDER;

struct quaternion { float x, y, z, w; };

typedef struct quaternion QUATERNION;
typedef struct quaternion *LPQUATERNION;
typedef struct quaternion const *LPCQUATERNION;

float Quaternion_dotProduct(LPCQUATERNION left, LPCQUATERNION right);
float Quaternion_length(LPCQUATERNION param);

QUATERNION Quaternion_fromEuler(LPCVECTOR3 euler, ROTATIONORDER order);
QUATERNION Quaternion_unm(LPCQUATERNION param);
QUATERNION Quaternion_normalized(LPCQUATERNION param);
QUATERNION Quaternion_fromMatrix(LPCMATRIX4 mat);
QUATERNION Quaternion_slerp(LPCQUATERNION p, LPCQUATERNION q, float t);
QUATERNION Quaternion_sqlerp(LPCQUATERNION a, LPCQUATERNION b, LPCQUATERNION c, LPCQUATERNION d, float t);

#endif
