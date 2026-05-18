#include "r_local.h"

#define R_UI_BASE_WIDTH  0.8f
#define R_UI_BASE_HEIGHT 0.6f
#define R_UI_MIN_ASPECT  (4.0f / 3.0f)

RECT R_UISceneRect(void) {
    size2_t window = R_GetWindowSize();
    float aspect = (float)window.width / (float)window.height;
    float x_scale = aspect > R_UI_MIN_ASPECT ? aspect / R_UI_MIN_ASPECT : 1.0f;
    float y_scale = aspect < R_UI_MIN_ASPECT ? R_UI_MIN_ASPECT / aspect : 1.0f;
    float scene_w = R_UI_BASE_WIDTH  * x_scale;
    float scene_h = R_UI_BASE_HEIGHT * y_scale;
    return MAKE(RECT,
                (R_UI_BASE_WIDTH  - scene_w) * 0.5f,
                (R_UI_BASE_HEIGHT - scene_h) * 0.5f,
                scene_w, scene_h);
}

void R_PrintSysText(LPCSTR string, DWORD x, DWORD y, COLOR32 color) {
    static VERTEX simp[256 * 6];
    size2_t window = R_GetWindowSize();
    LPVERTEX it = simp;
    for (LPCSTR s = string; *s; s++) {
        DWORD ch = *s;
        float fx = ch % 16;
        float fy = ch / 16;
        it = R_AddQuad(it, &(RECT ) {
            x + 10 * (s - string), window.height - y - 16, 8, 16
        }, &(RECT ) {
            fx/16,fy/8+1.f/8,1.f/16,-1.f/8
        }, color, 0);
    }
    
    DWORD num_vertices = (DWORD)(it - simp);
    MATRIX4 ui_matrix;
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, 0.0f, window.height, 0.0f, 100.0f);
    
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * num_vertices, simp, GL_STATIC_DRAW);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    
    R_BindTexture(tr.texture[TEX_FONT], 0);
    
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, num_vertices);
}

void R_SetBlending(BLEND_MODE mode) {
    if (mode == BLEND_MODE_ADD) {
        R_Call(glBlendFunc, GL_ONE, GL_ONE);
    } else {
        R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    return;
//    switch (mode) {
//        case BLEND_MODE_NONE: R_Call(glBlendFunc, GL_ONE, GL_ZERO); break;
//        case BLEND_MODE_BLEND: R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
//        case BLEND_MODE_ALPHAKEY: R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
//        case BLEND_MODE_ADD: R_Call(glBlendFunc, GL_ONE, GL_ONE); break;
////        case AM_ADDALPHA: R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE); break;
//        case BLEND_MODE_MODULATE: R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
//        case BLEND_MODE_MODULATE_2X: R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
//    }
}

void R_DrawImageEx(LPCDRAWIMAGE drawImage) {
    VERTEX simp[6];
    
    if (drawImage->rotate) { // Some UI backdrops use 90-degree-rotated UVs
        R_AddQuad(simp, &drawImage->screen, &(RECT) {
            .x = drawImage->uv.x + drawImage->uv.w, 
            .y = drawImage->uv.y, 
            .w = -drawImage->uv.w, 
            .h = drawImage->uv.h
        }, drawImage->color, 0);
        VECTOR2 tmp1 = simp[1].texcoord;
        VECTOR2 tmp2 = simp[5].texcoord;
        simp[1].texcoord = tmp2;
        simp[5].texcoord = tmp1;
    } else {
        R_AddQuad(simp, &drawImage->screen, &(RECT) {
            .x = drawImage->uv.x, 
            .y = drawImage->uv.y + drawImage->uv.h, 
            .w = drawImage->uv.w, 
            .h = -drawImage->uv.h
        }, drawImage->color, 0);
    }

    LPCSHADER shader = tr.shader[drawImage->shader];
    
    MATRIX4 ui_matrix, model_matrix;
    RECT const scene = R_UISceneRect();
    Matrix4_ortho(&ui_matrix, scene.x, scene.x + scene.w, scene.y, scene.y + scene.h, 0.0f, 100.0f);
    Matrix4_identity(&model_matrix);
    
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glUseProgram, shader->progid);
    R_Call(glUniformMatrix4fv, shader->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, shader->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniform1f , shader->uActiveGlow, drawImage->uActiveGlow);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);
    R_Call(glEnable, GL_BLEND);
    
    R_SetBlending(drawImage->alphamode);
    R_BindTexture(drawImage->texture, 0);
    
//    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (drawImage->uv.w > 1 || drawImage->uv.h > 1) {
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);

    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void R_DrawImage(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv, COLOR32 color) {
    R_DrawImageEx(&MAKE(DRAWIMAGE,
                        .texture = texture,
                        .screen = *screen,
                        .uv = uv ? *uv : MAKE(RECT,0,0,1,1),
                        .color = color,
                        .rotate = false,
                        .shader = SHADER_UI));
}

void R_DrawPic(LPCTEXTURE texture, float x, float y) {
    RECT screen = { x, y, texture->width / 2000.0, texture->height / 2000.0};
    R_DrawImage(texture, &screen, NULL, COLOR32_WHITE);
}

struct render_mesh R_MakeLoadingIndicatorMesh(void) {
    enum { SEGMENTS = 64 };
    VERTEX vertices[(SEGMENTS + 1) * 2] = { 0 };
    struct render_mesh mesh = { 0 };

    FOR_LOOP(i, SEGMENTS + 1) {
        FLOAT const progress = (FLOAT)i / (FLOAT)SEGMENTS;
        FLOAT const u = 1.0f - progress;
        FLOAT const angle = progress * (FLOAT)(M_PI * 2.0);
        FLOAT const x = cosf(angle);
        FLOAT const y = sinf(angle);
        VERTEX *outerVert = &vertices[i * 2];
        VERTEX *innerVert = &vertices[i * 2 + 1];

        outerVert->position = MAKE(VECTOR3, x * 0.5f, y * 0.5f, 0);
        outerVert->texcoord = MAKE(VECTOR2, u, 0.5f);
        outerVert->color = COLOR32_WHITE;
        innerVert->position = MAKE(VECTOR3, x * 0.34f, y * 0.34f, 0);
        innerVert->texcoord = MAKE(VECTOR2, u, 0.5f);
        innerVert->color = COLOR32_WHITE;
    }

    mesh.buffer = R_MakeVertexArrayObject(vertices, (SEGMENTS + 1) * 2);
    mesh.numVertices = (SEGMENTS + 1) * 2;
    mesh.primitive = GL_TRIANGLE_STRIP;
    return mesh;
}

void R_DrawLoadingIndicator(LPCRECT rect, DWORD time, COLOR32 color) {
    struct render_mesh const *mesh = &tr.mesh[MESH_LOADING_INDICATOR];
    FLOAT const cx = rect->x + rect->w * 0.5f;
    FLOAT const cy = rect->y + rect->h * 0.5f;
    FLOAT const size = MAX(MIN(rect->w, rect->h) * 0.11f, 0.006f);
    MATRIX4 ui_matrix, model_matrix;
    RECT const scene = R_UISceneRect();

    if (!color.a) {
        color = MAKE(COLOR32, 235, 220, 180, 255);
    }

    Matrix4_ortho(&ui_matrix, scene.x, scene.x + scene.w, scene.y, scene.y + scene.h, 0.0f, 100.0f);
    Matrix4_identity(&model_matrix);
    Matrix4_translate(&model_matrix, &MAKE(VECTOR3, cx, cy, 0));
    Matrix4_rotate(&model_matrix, &MAKE(VECTOR3, 0, 0, -360.0f * (FLOAT)(time % 900) / 900.0f), ROTATE_XYZ);
    Matrix4_scale(&model_matrix, &MAKE(VECTOR3, size, size, 1));

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glBindVertexArray, mesh->buffer->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, mesh->buffer->vbo);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_BindTexture(tr.texture[TEX_LOADING_INDICATOR], 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glDrawArrays, mesh->primitive, 0, mesh->numVertices);
}

void R_DrawWireRect(LPCRECT rect, COLOR32 color) {
    static VERTEX simp[5];
    R_AddStrip(simp, rect, color);

    MATRIX4 ui_matrix;
    size2_t const window = R_GetWindowSize();
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, 0.0f, window.height, 0.0f, 100.0f);

    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(simp), simp, GL_STATIC_DRAW);
    
    R_BindTexture(tr.texture[TEX_WHITE], 0);
    
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_LINE_STRIP, 0, sizeof(simp) / sizeof(*simp));
}

void R_DrawSelectionRect(LPCRECT rect, COLOR32 color) {
    R_DrawWireRect(rect, color);
}

void R_DrawBoundingBox(LPCBOX3 box, LPCMATRIX4 modelMatrix, LPCMATRIX4 vpMatrix, COLOR32 color) {
    static const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7},
    };
    VECTOR3 corners[8] = {
        { box->min.x, box->min.y, box->min.z },
        { box->max.x, box->min.y, box->min.z },
        { box->max.x, box->max.y, box->min.z },
        { box->min.x, box->max.y, box->min.z },
        { box->min.x, box->min.y, box->max.z },
        { box->max.x, box->min.y, box->max.z },
        { box->max.x, box->max.y, box->max.z },
        { box->min.x, box->max.y, box->max.z },
    };
    VERTEX simp[24] = { 0 };
    for (int i = 0; i < 12; i++) {
        simp[i * 2 + 0].position = corners[edges[i][0]];
        simp[i * 2 + 0].color = color;
        simp[i * 2 + 1].position = corners[edges[i][1]];
        simp[i * 2 + 1].color = color;
    }

    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uViewProjectionMatrix, 1, GL_FALSE, vpMatrix->v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uModelMatrix, 1, GL_FALSE, modelMatrix->v);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(simp), simp, GL_STATIC_DRAW);

    R_BindTexture(tr.texture[TEX_WHITE], 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glDisable, GL_DEPTH_TEST);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_LINES, 0, 24);
    R_Call(glEnable, GL_DEPTH_TEST);
}
