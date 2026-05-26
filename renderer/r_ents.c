#include "r_local.h"

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

void R_DrawEntities(void) {
    FOR_LOOP(i, tr.viewDef.num_entities) {
        R_RenderModel(tr.viewDef.entities+i);
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
    LINE3 const line = R_LineForScreenPoint(viewdef, x, y);
    FOR_LOOP(i, viewdef->num_entities) {
        renderEntity_t *ent = &viewdef->entities[i];
        if (MDLX_TraceModel(ent, &line)) {
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

static void DrawEntityOverlay(renderEntity_t const *ent) {
    VERTEX simp[6];
    RECT full = {0,0,1,1};
    COLOR32 black = {0,0,0,255};
    COLOR32 green = {0,255,0,255};
    VECTOR3 pos = Vector3_add(&ent->origin, &(VECTOR3){0,0,150});
    MATRIX4 const *proj = &tr.viewDef.viewProjectionMatrix;
    VECTOR3 p = Matrix4_multiply_vector3(proj, &pos);
    RECT screen = {p.x-0.035,p.y,0.07,0.015};

    R_AddQuad(simp, &screen, &full, black, 0);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);

    screen.w *= ent->health;

    R_AddQuad(simp, &screen, &full, green, 0);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);
}

void R_RenderOverlays(void) {
    MATRIX4 ui_matrix;
    Matrix4_identity(&ui_matrix);
    
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    
//    R_BindTexture(tr.texture[TEX_WHITE], 0);
    
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    FOR_LOOP(i, tr.viewDef.num_entities) {
        renderEntity_t const *ent = &tr.viewDef.entities[i];
        if (!(ent->flags & RF_SELECTED))
            continue;
        if (ent->flags & RF_HIDDEN)
            return;
        R_BindTexture(ent->healthbar, 0);
        DrawEntityOverlay(ent);
    }
}

#ifdef USE_SHADOWMAPS
extern bool is_rendering_lights;
#endif
DWORD selCircles[NUM_SELECTION_CIRCLES] = { 100, 300, 100000 };

#define FLAT_SHADOW_Z_BIAS 16.0f

static void R_RenderUberSplat(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (entity->splat && !(entity->flags & RF_NO_UBERSPLAT)) {
        R_RenderSplat(origin, entity->splatsize, entity->splat, tr.shader[SHADER_DEFAULT], COLOR32_WHITE);
    }
}

static void R_RenderShadow(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (!entity->shadow || (entity->flags & RF_NO_SHADOW) || !tr.world) {
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
        float width = entity->shadow->width * 32.0f;
        float height = entity->shadow->height * 32.0f;
        mins.x = origin->x - width * 0.3f;
        mins.y = origin->y - height * 0.3f;
        maxs.x = origin->x + width * 0.7f;
        maxs.y = origin->y + height * 0.7f;
    }

    COLOR32 shadowColor = {0, 0, 0, 128};
#ifndef WOW
    if (!(tr.viewDef.rdflags & RDF_NOFRUSTUMCULL)) {
        BOX3 bounds = {
            .min = { mins.x, mins.y, entity->origin.z - 32.0f },
            .max = { maxs.x, maxs.y, entity->origin.z + 32.0f },
        };
        if (!Frustum_ContainsAABox(&tr.viewDef.frustum, &bounds)) {
            return;
        }
    }
#endif
    R_RenderFlatRectSplat(&mins, &maxs, entity->origin.z + FLAT_SHADOW_Z_BIAS, entity->shadow, tr.shader[SHADER_SHADOWSPLAT], shadowColor);
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
