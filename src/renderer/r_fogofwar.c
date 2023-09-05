#include "r_local.h"

#define SIGHT_SIZE 64
#define SIGHT_DISTANCE 2000
#define MAX_FOGOFWAR_CASTERS 20000
#define NUM_SIGHT_SECIONS 5

LPCSTR vs_shadow =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"out vec4 v_color;\n"
"out vec2 v_texcoord;\n"
"uniform mat4 uViewProjectionMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform vec2 uEyePosition;\n"
"void main() {\n"
"    vec4 pos = vec4(i_position, 1.0);\n"
"    vec3 eye = vec3(uEyePosition, 0.0);\n"
"    vec3 up = vec3(0, 0, 1);\n"
"    vec3 dir = normalize(i_position - eye);\n"
"    vec3 side = normalize(cross(dir, up));\n"
"    if (distance(eye.xy, i_position.xy) > 100.0) {\n"
"      float x = (i_texcoord.x - 0.5) * 2.0;\n"
"      float y = i_texcoord.y;\n"
"      pos.xy += side.xy * x * /*i_position.z*/100.0;\n"
"      pos.xy += normalize(pos.xyz - eye).xy * y * 2000.0;\n"
"    }\n"
"    pos.z = 0;\n"
"    v_color = i_color;\n"
"    v_texcoord = i_texcoord;\n"
"    gl_Position = uViewProjectionMatrix * uModelMatrix * pos;\n"
"}\n";

LPCSTR fs_shadow =
"#version 140\n"
"in vec4 v_color;\n"
"in vec2 v_texcoord;\n"
"out vec4 o_color;\n"
"void main() {\n"
"    float f = 2.0 * abs(v_texcoord.x - 0.5);\n"
"    float k = smoothstep(0.0,0.2,v_texcoord.y);\n"
"    o_color = vec4(mix(1.0,f*f,k));\n"
"}\n";

enum {
    FOW_RT_IMMEDIATE,
    FOW_RT_HISTORY,
    FOW_RT_RESULT,
    FOW_RT_COUNT,
};

enum {
    FOW_SHADER_RAYCAST,
    FOW_SHADER_COUNT,
};

static struct {
    LPSHADER shader[FOW_SHADER_COUNT];
    LPRENDERTARGET rt[FOW_RT_COUNT];
    LPBUFFER casters;
    LPTEXTURE sight;
} resources = { 0 };

typedef struct caster_vertex {
    struct { short x, y, z; } position;
    struct { BYTE x, y; } texcoord;
} castervertex_t;

castervertex_t casters[MAX_FOGOFWAR_CASTERS * NUM_SIGHT_SECIONS];

LPTEXTURE R_AllocateSightTexture(void) {
    LPTEXTURE texture = R_AllocateTexture(SIGHT_SIZE, SIGHT_SIZE);
    COLOR32 col[SIGHT_SIZE * SIGHT_SIZE];
    DWORD mid = SIGHT_SIZE/2;
    VECTOR2 center = {mid,mid};
    FOR_LOOP(x, SIGHT_SIZE) {
        FOR_LOOP(y, SIGHT_SIZE) {
            float const d = Vector2_distance(&center, &(VECTOR2){x,y});
            float const f = MAX(0, 1.0 - d / mid);
            DWORD c = MIN(1, f * 2.0) * 0xff;
            col[x+y*SIGHT_SIZE].r = 0xff;
            col[x+y*SIGHT_SIZE].g = 0xff;
            col[x+y*SIGHT_SIZE].b = 0xff;
            col[x+y*SIGHT_SIZE].a = c;
        }
    }
    R_LoadTextureMipLevel(texture, 0, col, SIGHT_SIZE, SIGHT_SIZE);
    return texture;
}

static void R_MakeSightMatrix(renderEntity_t *ent, LPMATRIX4 model_matrix) {
    Matrix4_identity(model_matrix);
    Matrix4_translate(model_matrix, &(VECTOR3) {
        ent->origin.x - tr.world->center.x - SIGHT_DISTANCE / 2,
        ent->origin.y - tr.world->center.y - SIGHT_DISTANCE / 2,
        0
    });
    Matrix4_scale(model_matrix, &(VECTOR3) {
        SIGHT_DISTANCE,
        SIGHT_DISTANCE,
        SIGHT_DISTANCE,
    });
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix->v);
}

static DWORD R_AddCastersToBuffer(LPCBUFFER buffer) {
    castervertex_t *caster_writer = casters;
    COLOR32 white = {255,255,255,255};
    VERTEX rect[NUM_RECT_VERTICES];
    FOR_LOOP(i, tr.viewDef.num_entities) {
        renderEntity_t *ent = &tr.viewDef.entities[i];
        if (ent->radius < 10)
            continue;
        RECT screen = { ent->origin.x, ent->origin.y, 0, 0, };
        FOR_LOOP(j, NUM_SIGHT_SECIONS) {
            RECT uv = {((float)j)/NUM_SIGHT_SECIONS,0,1.0/NUM_SIGHT_SECIONS,1};
            LPCVERTEX end = R_AddQuad(rect, &screen, &uv, white, ent->radius);
            for (LPCVERTEX v = rect; v != end; v++) {
                caster_writer->position.x = v->position.x;
                caster_writer->position.y = v->position.y;
                caster_writer->position.z = v->position.z;
                caster_writer->texcoord.x = v->texcoord.x * 0xff;
                caster_writer->texcoord.y = v->texcoord.y * 0xff;
                caster_writer++;
            }
        }
    }
    R_Call(glBindVertexArray, buffer->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buffer->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(castervertex_t) * (caster_writer - casters), casters, GL_STATIC_DRAW);
    return (DWORD)(caster_writer - casters);
}

static DWORD R_PushRectToBuffer(DWORD buffer_id, LPCRECT value, float alpha) {
    COLOR32 white = {255*alpha,255*alpha,255*alpha,255*alpha};
    RECT uv = {0,0,1,1};
    VERTEX rect[NUM_RECT_VERTICES];
    R_AddQuad(rect, value, &uv, white, 0);
    R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(vertex_t) * NUM_RECT_VERTICES, rect, GL_STATIC_DRAW);
    return NUM_RECT_VERTICES;
}

static void R_BlitTexture(GLuint texid, float alpha) {
    MATRIX4 model_matrix;
    MATRIX4 proj_matrix;
    RECT const uv = {0,0,1,1};
    
    Matrix4_ortho(&proj_matrix, 0, 1, 0, 1, -1, 1);
    Matrix4_identity(&model_matrix);

    // Set simple projection
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, proj_matrix.v);

    R_Call(glBindTexture, GL_TEXTURE_2D, texid);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, R_PushRectToBuffer(RBUF_TEMP1, &uv, alpha));
}

void R_RenderFogOfWar(void) {
    if (!tr.world)
        return;
    
    DWORD const texture_width = (tr.world->width - 1) * 4;
    DWORD const texture_height = (tr.world->height - 1) * 4;

    MATRIX4 model_matrix;
    MATRIX4 proj_matrix;
    VECTOR2 mapsize = GetWar3MapSize(tr.world);

    Matrix4_identity(&model_matrix);
    Matrix4_translate(&model_matrix, &(VECTOR3) { -tr.world->center.x, -tr.world->center.y, 0 });
    Matrix4_ortho(&proj_matrix, 0.0f, mapsize.x, 0.0f, mapsize.y, 0.0f, 100.0f);

    DWORD num_casters = R_AddCastersToBuffer(resources.casters);
    R_PushRectToBuffer(RBUF_TEMP1, &(RECT const){0,0,1,1}, 1);

    R_Call(glUseProgram, resources.shader[FOW_SHADER_RAYCAST]->progid);
    R_Call(glUniformMatrix4fv, resources.shader[FOW_SHADER_RAYCAST]->uViewProjectionMatrix, 1, GL_FALSE, proj_matrix.v);
    R_Call(glUniformMatrix4fv, resources.shader[FOW_SHADER_RAYCAST]->uModelMatrix, 1, GL_FALSE, model_matrix.v);

    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, proj_matrix.v);

    R_Call(glViewport, 0, 0, texture_width, texture_height);
    R_Call(glScissor, 0, 0, texture_width, texture_height);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, resources.rt[FOW_RT_IMMEDIATE]->buffer);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDepthMask, GL_FALSE);
    R_Call(glDepthFunc, GL_ALWAYS);
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glClearColor, 0, 0, 0, 0);
    R_Call(glClear, GL_COLOR_BUFFER_BIT);
    R_Call(glActiveTexture, GL_TEXTURE0);
    R_Call(glBindTexture, GL_TEXTURE_2D, resources.sight->texid);

    FOR_LOOP(p, tr.viewDef.num_entities) {
        renderEntity_t *ent = tr.viewDef.entities+p;

        if (ent->team != 1)
            continue;
        
        R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        R_Call(glColorMask, GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        R_Call(glClear, GL_COLOR_BUFFER_BIT);

        // Draw smooth circle into dst alpha
        R_MakeSightMatrix(ent, &model_matrix);
        R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
        R_Call(glBindTexture, GL_TEXTURE_2D, resources.sight->texid);
        R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
        R_Call(glDrawArrays, GL_TRIANGLES, 0, NUM_RECT_VERTICES);

        // Draw line of sight into dst alpha
        R_Call(glUseProgram, resources.shader[FOW_SHADER_RAYCAST]->progid);
        R_Call(glUniform2fv, resources.shader[FOW_SHADER_RAYCAST]->uEyePosition, 1, (GLfloat *)&ent->origin);
        R_Call(glBindVertexArray, resources.casters->vao);
        R_Call(glBlendFunc, GL_DST_ALPHA, GL_ZERO);
        R_Call(glBindBuffer, GL_ARRAY_BUFFER, resources.casters->vbo);
        R_Call(glDrawArrays, GL_TRIANGLES, 0, num_casters);
        
        // Draw white rect using dst alpha
        R_MakeSightMatrix(ent, &model_matrix);
        R_Call(glBindVertexArray, tr.buffer[RBUF_TEMP1]->vao);
        R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.buffer[RBUF_TEMP1]->vbo);
        R_Call(glColorMask, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        R_Call(glBlendFunc, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.texture[TEX_WHITE]->texid);
        R_Call(glDrawArrays, GL_TRIANGLES, 0, NUM_RECT_VERTICES);
    }
    
    Matrix4_ortho(&proj_matrix, 0, 1, 0, 1, -1, 1);
    Matrix4_identity(&model_matrix);

    // Set simple projection
    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, proj_matrix.v);

    // Add current state to history
    R_Call(glBlendEquation, GL_MAX);
    R_Call(glBlendFunc, GL_ONE, GL_ONE);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, resources.rt[FOW_RT_HISTORY]->buffer);
    R_BlitTexture(resources.rt[FOW_RT_IMMEDIATE]->texture, 1.0);
    
    // revert blend func
    R_Call(glBlendEquation, GL_FUNC_ADD);
    R_Call(glBlendFunc, GL_ONE, GL_ZERO);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, resources.rt[FOW_RT_RESULT]->buffer);
    R_BlitTexture(resources.rt[FOW_RT_HISTORY]->texture, 0.5);
    R_Call(glBlendFunc, GL_ONE, GL_ONE);
    R_BlitTexture(resources.rt[FOW_RT_IMMEDIATE]->texture, 0.5);

    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // revert changes
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}


LPBUFFER R_MakeCastersVertexArrayObject(void) {
    LPBUFFER buf = ri.MemAlloc(sizeof(BUFFER));

    R_Call(glGenVertexArrays, 1, &buf->vao);
    R_Call(glGenBuffers, 1, &buf->vbo);
    R_Call(glBindVertexArray, buf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buf->vbo);

    R_Call(glEnableVertexAttribArray, attrib_position);
    R_Call(glEnableVertexAttribArray, attrib_texcoord);

    R_Call(glVertexAttribPointer, attrib_position, 3, GL_SHORT, GL_FALSE, sizeof(struct caster_vertex), FOFS(caster_vertex, position));
    R_Call(glVertexAttribPointer, attrib_texcoord, 2, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct caster_vertex), FOFS(caster_vertex, texcoord));

    return buf;
}

void R_InitFogOfWar(DWORD width, DWORD height) {
    resources.rt[FOW_RT_IMMEDIATE] = R_AllocateRenderTexture(width, height, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
    resources.rt[FOW_RT_HISTORY] = R_AllocateRenderTexture(width, height, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
    resources.rt[FOW_RT_RESULT] = R_AllocateRenderTexture(width, height, GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
    resources.shader[FOW_SHADER_RAYCAST] = R_InitShader(vs_shadow, fs_shadow);
    resources.casters = R_MakeCastersVertexArrayObject();
    resources.sight = R_AllocateSightTexture();
}

void R_ShutdownFogOfWar(void) {
    FOR_LOOP(i, FOW_SHADER_COUNT) R_ReleaseShader(resources.shader[i]);
    FOR_LOOP(i, FOW_RT_COUNT) R_ReleaseRenderTexture(resources.rt[i]);
    R_ReleaseVertexArrayObject(resources.casters);
    R_ReleaseTexture(resources.sight);
}

DWORD R_GetFogOfWarTexture(void) {
//    if (resources.rt[FOW_RT_RESULT] && !(tr.viewDef.rdflags & RDF_NOFOG)) {
//        return resources.rt[FOW_RT_RESULT]->texture;
//    } else {
        return tr.texture[TEX_WHITE]->texid;
//    }
}
