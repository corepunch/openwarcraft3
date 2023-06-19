#ifndef rect_h
#define rect_h

#include "vector2.h"

typedef struct rect {
    float x, y, w, h;
} rect_t;

typedef struct rect RECT;
typedef struct rect *LPRECT;
typedef struct rect const *LPCRECT;

int Rect_contains(LPCRECT rect, LPCVECTOR2 point);
RECT Rect_scale(LPCRECT rect, float scale);
RECT Rect_div(LPCRECT rect, int res);

#endif /* rect_h */
