#include "r_local.h"

void R_GetEntityMatrix(renderEntity_t const *entity, LPMATRIX4 matrix) {
    Matrix4_identity(matrix);
    Matrix4_translate(matrix, &entity->origin);
    Matrix4_rotate(matrix, &(VECTOR3){0, 0, entity->angle * 180 / M_PI}, ROTATE_XYZ);
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

extern bool is_rendering_lights;
DWORD selCircles[NUM_SELECTION_CIRCLES] = { 100, 300, 100000 };

static void R_RenderUberSplat(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (entity->splat && !(entity->flags & RF_NO_UBERSPLAT)) {
        R_RenderSplat(origin, entity->splatsize, entity->splat, tr.shader[SHADER_DEFAULT], COLOR32_WHITE);
    }
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
    R_GetEntityMatrix(entity, &transform);
    
    if (entity->flags & RF_HIDDEN)
        return;
    
    if (is_rendering_lights && (entity->flags & RF_NO_SHADOW))
        return;
    
    switch (entity->model->modeltype) {
        case ID_MDLX:
            MDX_RenderModel(entity, entity->model->mdx, &transform);
            break;
        case ID_43DM:
            M3_RenderModel(entity, entity->model->m3, &transform);
            break;
    }
    
    if (is_rendering_lights)
        return;
    
    R_RenderUberSplat(entity, (LPCVECTOR2)&entity->origin);
    R_RenderSelectedCircle(entity, (LPCVECTOR2)&entity->origin);
}
