#include "r_local.h"


void R_GetEntityMatrix(renderEntity_t const *entity, LPMATRIX4 matrix) {
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &entity->origin);
    Matrix4_rotate(matrix, &(VECTOR3){0, 0, entity->angle * 180 / 3.14f}, ROTATE_XYZ);
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

renderEntity_t *R_Trace(viewDef_t const *viewdef, float x, float y) {
    MATRIX4 invproj;
    Matrix4_inverse(&tr.viewDef.projectionMatrix, &invproj);
    size2_t window = R_GetWindowSize();
    VECTOR2 p = {
        .x = (x / window.width - 0.5) * 2,
        .y = (0.5 - y / window.height) * 2,
    };
    LINE3 line = {
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 0 }),
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 1 }),
    };
    FOR_LOOP(i, tr.viewDef.num_entities) {
        renderEntity_t *edict = &tr.viewDef.entities[i];
        if (R_TraceModel(edict, &line)) {
            return edict;
        }
    }
    return NULL;
}
