#include "r_local.h"
#include "r_game.h"

static VECTOR2 R_PointToViewSpace(viewDef_t const *viewdef, float x, float y) {
    size2_t window = R_GetWindowSize();
    RECT viewport = viewdef ? viewdef->viewport : (RECT){ 0, 0, 1, 1 };
    FLOAT left;
    FLOAT top;
    FLOAT width;
    FLOAT height;

    if (viewport.w <= 0.0f || viewport.h <= 0.0f) {
        viewport = (RECT){ 0, 0, 1, 1 };
    }
    left = viewport.x * window.width;
    top = (1.0f - (viewport.y + viewport.h)) * window.height;
    width = viewport.w * window.width;
    height = viewport.h * window.height;
    return (VECTOR2){
        .x = ((x - left) / width - 0.5f) * 2.0f,
        .y = (0.5f - (y - top) / height) * 2.0f,
    };
}

LINE3 R_LineForScreenPoint(viewDef_t const *viewdef, float x, float y) {
    MATRIX4 invproj;
    Matrix4_inverse(&viewdef->viewProjectionMatrix, &invproj);
    VECTOR2 const p = R_PointToViewSpace(viewdef, x, y);
    LINE3 const line = {
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 0 }),
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 1 }),
    };
    return line;
}

/* Drag-panning stays on the camera target plane instead of jumping across terrain tiers. */
bool R_TraceCameraPlane(viewDef_t const *viewdef, float x, float y, LPVECTOR3 point) {
    LINE3 line;
    PLANE3 plane;

    if (!viewdef || !point) return false;
    line = R_LineForScreenPoint(viewdef, x, y);
    /* Snapshot terrain Z can dip below the rendered focus; sharing its target prevents drag-anchor jumps. */
    plane = (PLANE3){ .normal = { 0, 0, 1 }, .distance = -viewdef->target.z };
    return Line3_intersect_plane3(&line, &plane, point);
}

bool R_TraceEntity(viewDef_t const *viewdef, float x, float y, LPDWORD number) {
    if (!viewdef || !number) {
        return false;
    }
    LINE3 const line = R_LineForScreenPoint(viewdef, x, y);
    FLOAT best = FLT_MAX;
    DWORD best_number = 0;

    FOR_LOOP(i, viewdef->num_entities) {
        renderEntity_t *ent = &viewdef->entities[i];
        FLOAT distance;

        if (!ent->number || !ent->model || (ent->flags & (RF_HIDDEN | RF_NOT_SELECTABLE))) {
            continue;
        }
        if (R_TraceModel(ent, &line, &distance) && distance < best) {
            best = distance;
            best_number = ent->number;
        }
    }
    if (best_number) {
        *number = best_number;
        return true;
    }
    return false;
}

DWORD R_EntitiesInRect(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array) {
    if (!viewdef || !rect || !array || max == 0) {
        return 0;
    }
    tr.viewDef = *viewdef;
    VECTOR2 const a = R_PointToViewSpace(viewdef, rect->x, rect->y);
    VECTOR2 const b = R_PointToViewSpace(viewdef, rect->x+rect->w, rect->y+rect->h);
    RECT const screen = {
        .x = MIN(a.x, b.x),
        .y = MIN(a.y, b.y),
        .w = MAX(a.x, b.x) - MIN(a.x, b.x),
        .h = MAX(a.y, b.y) - MIN(a.y, b.y),
    };
    DWORD count = 0;
    FOR_LOOP(i, viewdef->num_entities) {
        renderEntity_t const *ent = &viewdef->entities[i];
        if (!ent->number || !ent->model || (ent->flags & (RF_HIDDEN | RF_NOT_SELECTABLE))) {
            continue;
        }
        VECTOR3 const org = Matrix4_multiply_vector3(&viewdef->viewProjectionMatrix, &ent->origin);
        if (Rect_contains(&screen, (LPVECTOR2)&org)) {
            if (count >= max) {
                break;
            }
            array[count++] = ent->number;
        }
    }
    return count;
}
