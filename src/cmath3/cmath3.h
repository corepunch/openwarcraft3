#ifndef __cmath3_h__
#define __cmath3_h__

#define EPSILON 1e-6

#include <math.h>

#include "types/vector2.h"
#include "types/vector3.h"
#include "types/vector4.h"
#include "types/matrix3.h"
#include "types/matrix4.h"
#include "types/quaternion.h"
#include "types/line3.h"
#include "types/sphere3.h"
#include "types/plane3.h"
#include "types/box3.h"
#include "types/triangle3.h"
#include "types/rect.h"

float LerpNumber(float a, float b, float t);

#endif
