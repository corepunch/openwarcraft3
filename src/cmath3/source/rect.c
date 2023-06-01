#include "../cmath3.h"

int Rect_contains(LPCRECT rect, LPCVECTOR2 point) {
    if (rect->x > point->x) return 0;
    if (rect->y > point->y) return 0;
    if (rect->x + rect->width <= point->x) return 0;
    if (rect->y + rect->height <= point->y) return 0;
    return 1;
}
