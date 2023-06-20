#include "r_local.h"


void R_GetEntityMatrix(renderEntity_t const *entity, LPMATRIX4 matrix) {
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &entity->origin);
    Matrix4_rotate(matrix, &(VECTOR3){0, 0, entity->angle * 180 / M_PI}, ROTATE_XYZ);
    Matrix4_scale(matrix, &(VECTOR3){entity->scale, entity->scale, entity->scale});
}

void R_DrawEntities(void) {
    FOR_LOOP(i, tr.viewDef.num_entities) {
        renderEntity_t const *edict = &tr.viewDef.entities[i];
        if (!(tr.viewDef.rdflags & RDF_NOFRUSTUMCULL) &&
            !R_IsPointVisible(&edict->origin, 1.25f))
            continue;
        R_RenderModel(edict);
    }
}

VECTOR2 R_PointToScreenSpace(float x, float y) {
    size2_t window = R_GetWindowSize();
    VECTOR2 p = {
        .x = (x / window.width - 0.5) * 2,
        .y = (0.5 - y / window.height) * 2,
    };
    return p;
}

LINE3 R_LineForScreenPoint(viewDef_t const *viewdef, float x, float y) {
    MATRIX4 invproj;
    Matrix4_inverse(&viewdef->projectionMatrix, &invproj);
    VECTOR2 const p = R_PointToScreenSpace(x, y);
    LINE3 const line = {
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 0 }),
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 1 }),
    };
    return line;
}

bool R_TraceEntity(viewDef_t const *viewdef, float x, float y, LPDWORD number) {
    LINE3 const line = R_LineForScreenPoint(viewdef, x, y);
    FOR_LOOP(i, viewdef->num_entities) {
        renderEntity_t *ent = &viewdef->entities[i];
        if (R_TraceModel(ent, &line)) {
            *number = ent->number;
            return true;
        }
    }
    return false;
}

DWORD R_EntitiesInRect(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array) {
    tr.viewDef = *viewdef;
    VECTOR2 const a = R_PointToScreenSpace(rect->x, rect->y);
    VECTOR2 const b = R_PointToScreenSpace(rect->x+rect->w, rect->y+rect->h);
    RECT const screen = {
        .x = MIN(a.x, b.x),
        .y = MIN(a.y, b.y),
        .w = MAX(a.x, b.x) - MIN(a.x, b.x),
        .h = MAX(a.y, b.y) - MIN(a.y, b.y),
    };
    DWORD count = 0;
    FOR_LOOP(i, viewdef->num_entities) {
        renderEntity_t const *ent = &viewdef->entities[i];
        VECTOR3 const org = Matrix4_multiply_vector3(&viewdef->projectionMatrix, &ent->origin);
        if (ent->number != 0 && Rect_contains(&screen, (LPVECTOR2)&org)) {
            array[count++] = ent->number;
        }
    }
    return count;
}
