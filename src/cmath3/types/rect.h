#ifndef rect_h
#define rect_h

#include "vector2.h"

typedef struct rect {
    float x, y, width, height;
} rect_t;

typedef struct rect RECT;
typedef struct rect *LPRECT;
typedef struct rect const *LPCRECT;

int Rect_contains(LPCRECT rect, LPCVECTOR2 point);

#endif /* rect_h */
