#include "r_local.h"
#include <float.h>

void R_GetEntityMatrix(renderEntity_t const *entity, LPMATRIX4 matrix) {
    VECTOR3 origin = entity->origin;

    if ((entity->flags & RF_GROUND_ANCHOR) &&
        entity->model &&
        entity->model->modeltype == ID_MD20) {
        origin.z += M2_GroundOffset(entity->model->m2) * entity->scale;
    }

    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &origin);
    if (entity->model && entity->model->modeltype == ID_MD20) {
        MATRIX4 adt_to_world_basis;
        MATRIX4 tmp;

#ifdef WOW
        if (entity->flags & RF_GROUND_ANCHOR) {
            Matrix4_rotate(matrix, &(VECTOR3){ 0.0f, 0.0f, entity->rotation.x + 180.0f }, ROTATE_XYZ);
        }
#endif

        Matrix4_identity(&adt_to_world_basis);
        adt_to_world_basis.v[0] = 0.0f;
        adt_to_world_basis.v[1] = -1.0f;
        adt_to_world_basis.v[2] = 0.0f;
        adt_to_world_basis.v[4] = 0.0f;
        adt_to_world_basis.v[5] = 0.0f;
        adt_to_world_basis.v[6] = 1.0f;
        adt_to_world_basis.v[8] = -1.0f;
        adt_to_world_basis.v[9] = 0.0f;
        adt_to_world_basis.v[10] = 0.0f;
        Matrix4_multiply(matrix, &adt_to_world_basis, &tmp);
        *matrix = tmp;

        Matrix4_scale(matrix, &(VECTOR3){ -1.0f, 1.0f, -1.0f });
        Matrix4_rotate(matrix, &(VECTOR3){ 0.0f, entity->rotation.y - 270.0f, 0.0f }, ROTATE_XYZ);
#if defined(WOW)
        if (!(entity->flags & RF_GROUND_ANCHOR)) {
            Matrix4_rotate(matrix, &(VECTOR3){ 0.0f, 0.0f, -entity->rotation.x }, ROTATE_XYZ);
        }
#else
        Matrix4_rotate(matrix, &(VECTOR3){ 0.0f, 0.0f, -entity->rotation.x }, ROTATE_XYZ);
#endif
        Matrix4_rotate(matrix, &(VECTOR3){ entity->rotation.z - 90.0f, 0.0f, 0.0f }, ROTATE_XYZ);
    } else {
        Matrix4_rotate(matrix, &(VECTOR3){0, 0, entity->angle * 180 / M_PI}, ROTATE_XYZ);
    }
    Matrix4_scale(matrix, &(VECTOR3){entity->scale, entity->scale, entity->scale});
}

static BOOL R_EntityInView(renderEntity_t const *entity) {
    float radius;

    if (!entity || (entity->flags & RF_HIDDEN) || !entity->model) {
        return false;
    }
    if (tr.viewDef.rdflags & RDF_NOFRUSTUMCULL) {
        return true;
    }

    radius = MAX(entity->radius * MAX(entity->scale, 1.0f), 16.0f);
    return Frustum_ContainsSphere(&tr.viewDef.frustum, &(SPHERE3){
        .center = entity->origin,
        .radius = radius,
    });
}

void R_DrawEntities(void) {
    FOR_LOOP(i, tr.viewDef.num_entities) {
        if (R_EntityInView(tr.viewDef.entities+i)) {
            R_RenderModel(tr.viewDef.entities+i);
        }
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
    Matrix4_inverse(&viewdef->viewProjectionMatrix, &invproj);
    VECTOR2 const p = R_PointToScreenSpace(x, y);
    LINE3 const line = {
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 0 }),
        Matrix4_multiply_vector3(&invproj, &(VECTOR3 const) { p.x, p.y, 1 }),
    };
    return line;
}

bool R_TraceEntity(viewDef_t const *viewdef, float x, float y, LPDWORD number) {
    if (!viewdef || !number) {
        return false;
    }
    LINE3 const line = R_LineForScreenPoint(viewdef, x, y);
#ifdef WOW
    FLOAT best = FLT_MAX;
    DWORD best_number = 0;
#endif

    FOR_LOOP(i, viewdef->num_entities) {
        renderEntity_t *ent = &viewdef->entities[i];
        if (MDLX_TraceModel(ent, &line)) {
            *number = ent->number;
            return true;
        }
#ifdef WOW
        VECTOR3 ab = Vector3_sub(&line.b, &line.a);
        VECTOR3 ac;
        VECTOR3 center = ent->origin;
        FLOAT radius = MAX(1.5f, ent->radius * MAX(1.0f, ent->scale));
        FLOAT denom = Vector3_dot(&ab, &ab);
        FLOAT t;
        VECTOR3 closest;
        VECTOR3 delta;
        FLOAT dist2;

        if (!ent->number || !ent->model || denom <= 0.0001f) {
            continue;
        }
        center.z += radius;
        ac = Vector3_sub(&center, &line.a);
        t = Vector3_dot(&ac, &ab) / denom;
        t = MAX(0.0f, MIN(1.0f, t));
        closest = (VECTOR3){
            line.a.x + ab.x * t,
            line.a.y + ab.y * t,
            line.a.z + ab.z * t,
        };
        delta = Vector3_sub(&center, &closest);
        dist2 = Vector3_dot(&delta, &delta);
        if (dist2 <= radius * radius && t < best) {
            best = t;
            best_number = ent->number;
        }
#endif
    }
#ifdef WOW
    if (best_number) {
        *number = best_number;
        return true;
    }
#endif
    return false;
}

DWORD R_EntitiesInRect(viewDef_t const *viewdef, LPCRECT rect, DWORD max, LPDWORD array) {
    if (!viewdef || !rect || !array || max == 0) {
        return 0;
    }
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
        VECTOR3 const org = Matrix4_multiply_vector3(&viewdef->viewProjectionMatrix, &ent->origin);
        if (ent->number != 0 && Rect_contains(&screen, (LPVECTOR2)&org)) {
            if (count >= max) {
                break;
            }
            array[count++] = ent->number;
        }
    }
    return count;
}

#ifdef USE_SHADOWMAPS
extern bool is_rendering_lights;
#endif
DWORD selCircles[NUM_SELECTION_CIRCLES] = { 100, 300, 100000 };

static void R_RenderUberSplat(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (entity->splat && !(entity->flags & RF_NO_UBERSPLAT)) {
        R_RenderSplat(origin, entity->splatsize, entity->splat, tr.shader[SHADER_DEFAULT], COLOR32_WHITE);
    }
}

static void R_RenderShadow(const renderEntity_t *entity, LPCVECTOR2 origin) {
    LPCTEXTURE shadow = entity->shadow;
    BOX3 bounds;
#ifdef WOW
    BOOL use_fast_blob = false;
    float shadow_z = entity->origin.z + WOW_SPLAT_Z_BIAS;

    if (!shadow) {
        shadow = tr.texture[TEX_BLOB_SHADOW];
    }
    use_fast_blob = shadow == tr.texture[TEX_BLOB_SHADOW] &&
                    entity->shadow_w <= 0 &&
                    entity->shadow_h <= 0;
#endif
    if (!shadow || (entity->flags & RF_NO_SHADOW)
#ifndef WOW
        || !tr.world
#endif
    ) {
        return;
    }

    VECTOR2 mins;
    VECTOR2 maxs;
    if (entity->shadow_w > 0 && entity->shadow_h > 0) {
        mins.x = origin->x - entity->shadow_x;
        mins.y = origin->y - entity->shadow_y;
        maxs.x = mins.x + entity->shadow_w;
        maxs.y = mins.y + entity->shadow_h;
    } else {
#ifdef WOW
        float radius = MAX(entity->radius * MAX(entity->scale, 1.0f), 1.0f);
        float width = MAX(radius * 2.4f, 2.0f);
        float height = MAX(radius * 1.6f, 1.5f);
        mins.x = origin->x - width * 0.5f;
        mins.y = origin->y - height * 0.5f;
        maxs.x = mins.x + width;
        maxs.y = mins.y + height;
#else
        int pivot_x = (int)(shadow->width * 0.3f + 0.5f);
        int pivot_y = (int)(shadow->height * 0.7f + 0.5f);
        float width = shadow->width * 32.0f;
        float height = shadow->height * 32.0f;
        mins.x = origin->x - pivot_x * 32.0f;
        mins.y = origin->y - (shadow->height - pivot_y) * 32.0f;
        maxs.x = mins.x + width;
        maxs.y = mins.y + height;
#endif
    }

    COLOR32 shadowColor = {0, 0, 0, 128};
#ifdef WOW
    if (use_fast_blob) {
        shadow_z = R_GetHeightAtPoint(origin->x, origin->y) + WOW_SPLAT_Z_BIAS;
    }
    bounds = (BOX3){
        .min = { mins.x, mins.y, shadow_z - 16.0f },
        .max = { maxs.x, maxs.y, shadow_z + 16.0f },
    };
    if (!(tr.viewDef.rdflags & RDF_NOFRUSTUMCULL) &&
        !Frustum_ContainsAABox(&tr.viewDef.frustum, &bounds)) {
        return;
    }
    if (use_fast_blob) {
        R_RenderFlatRectSplat(&mins, &maxs, shadow_z, shadow, tr.shader[SHADER_SHADOWSPLAT], shadowColor);
        return;
    }
#else
    if (!(tr.viewDef.rdflags & RDF_NOFRUSTUMCULL)) {
        bounds = (BOX3){
            .min = { mins.x, mins.y, entity->origin.z - 32.0f },
            .max = { maxs.x, maxs.y, entity->origin.z + 32.0f },
        };
        if (!Frustum_ContainsAABox(&tr.viewDef.frustum, &bounds)) {
            return;
        }
    }
#endif
    R_RenderRectSplat(&mins, &maxs, shadow, tr.shader[SHADER_SHADOWSPLAT], shadowColor);
}

static void R_RenderSelectedCircle(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (entity->flags & RF_SELECTED) {
        COLOR32 color = { 0, 255, 0, 255 };
        float radius = entity->radius;
        FOR_LOOP(i, NUM_SELECTION_CIRCLES) {
            if ((radius * 2) > selCircles[i])
                continue;
            R_RenderSplat(origin, radius, tr.texture[TEX_SELECTION_CIRCLE+i], tr.shader[SHADER_SPLAT], color);
            break;
        }
    }
}

void MDX_RenderModel(renderEntity_t const *, mdxModel_t const *, LPCMATRIX4);
void M3_RenderModel(renderEntity_t const *, m3Model_t const *, LPCMATRIX4);

void R_RenderModel(renderEntity_t const *entity) {
    MATRIX4 transform;
#ifdef WOW
    MATRIX4 attached_transform;
    renderEntity_t attached_entity;
#endif
    R_GetEntityMatrix(entity, &transform);
    
    if ((entity->flags & RF_HIDDEN) || !entity->model)
        return;
    
#ifdef USE_SHADOWMAPS
    if (is_rendering_lights && (entity->flags & RF_NO_SHADOW))
        return;
#endif

#ifdef USE_SHADOWMAPS
    if (!is_rendering_lights)
#endif
        R_RenderUberSplat(entity, (LPCVECTOR2)&entity->origin);
#ifdef USE_SHADOWMAPS
    if (!is_rendering_lights)
#endif
        R_RenderShadow(entity, (LPCVECTOR2)&entity->origin);
    
    switch (entity->model->modeltype) {
        case ID_MDLX:
            MDX_RenderModel(entity, entity->model->mdx, &transform);
            break;
        case ID_43DM:
            M3_RenderModel(entity, entity->model->m3, &transform);
            break;
        case ID_MD20:
            M2_RenderModel(entity, entity->model->m2, &transform);
#ifdef WOW
            if (entity->attached_model &&
                entity->attached_model->modeltype == ID_MD20 &&
#ifdef USE_SHADOWMAPS
                !is_rendering_lights &&
#endif
                M2_AttachmentMatrix(entity->model->m2, 1, &transform, &attached_transform)) {
                attached_entity = *entity;
                attached_entity.model = entity->attached_model;
                attached_entity.attached_model = NULL;
                attached_entity.frame = 0;
                attached_entity.oldframe = 0;
                attached_entity.flags &= ~RF_GROUND_ANCHOR;
                attached_entity.flags |= RF_NO_SHADOW;
                M2_RenderModel(&attached_entity, attached_entity.model->m2, &attached_transform);
            }
#endif
            break;
    }
    
#ifdef USE_SHADOWMAPS
    if (is_rendering_lights)
        return;
#endif

    R_RenderSelectedCircle(entity, (LPCVECTOR2)&entity->origin);
}
