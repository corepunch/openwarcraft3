#include "../cmath3.h"

int Rect_contains(LPCRECT rect, LPCVECTOR2 point) {
    if (rect->x > point->x) return 0;
    if (rect->y > point->y) return 0;
    if (rect->x + rect->w <= point->x) return 0;
    if (rect->y + rect->h <= point->y) return 0;
    return 1;
}

RECT Rect_scale(LPCRECT rect, float scale) {
    RECT const screen = {
        .x = rect->x * scale,
        .y = rect->y * scale,
        .w = rect->w * scale,
        .h = rect->h * scale,
    };
    return screen;
}

RECT Rect_div(LPCRECT rect, int res) {
    return Rect_scale(rect, 1.0 / res);
}

VECTOR2 Rect_center(LPCRECT rect) {
    VECTOR2 const center = {
        rect->x + rect->w * 0.5f,
        rect->y + rect->h * 0.5f,
    };
    return center;
}
