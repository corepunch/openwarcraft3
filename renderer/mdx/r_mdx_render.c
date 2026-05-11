#include "r_mdx.h"
#include "../r_local.h"

//typedef enum {
//    vertexattr_position,
//    vertexattr_color,
//    vertexattr_texcoord,
//    vertexattr_normal,
//    vertexattr_skin1,
//    vertexattr_skin2,
//    vertexattr_boneWeight1,
//    vertexattr_boneWeight2,
//} mdxVertexAttribute_t;

static LPCSTR mdx_vs =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"in vec3 i_normal;\n"
"in vec4 i_skin1;\n"
"in vec4 i_boneWeight1;\n"
#if MAX_SKIN_BONES > 4
"in vec4 i_boneWeight2;\n"
"in vec4 i_skin2;\n"
#endif
"out vec4 v_color;\n"
"out vec4 v_shadow;\n"
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"uniform mat4 uBones[128];\n"
"uniform mat4 uViewProjectionMatrix;\n"
"uniform mat4 uTextureMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"void main() {\n"
"    vec4 pos4 = vec4(i_position, 1.0);\n"
"    vec4 norm4 = vec4(i_normal, 0.0);\n"
"    vec4 position = vec4(0.0);\n"
"    vec4 normal = vec4(0.0);\n"
"    for (int i = 0; i < 4; ++i) {\n"
"        position += uBones[int(i_skin1[i])] * pos4 * i_boneWeight1[i];\n"
"        normal += uBones[int(i_skin1[i])] * norm4 * i_boneWeight1[i];\n"
#if MAX_SKIN_BONES > 4
"        position += uBones[int(i_skin2[i])] * pos4 * i_boneWeight2[i];\n"
"        normal += uBones[int(i_skin2[i])] * norm4 * i_boneWeight2[i];\n"
#endif
"    }\n"
"    position.w = 1.0;\n"
"    v_color = i_color;\n"
"    v_texcoord = i_texcoord;\n"
"    v_texcoord2 = (uTextureMatrix * uModelMatrix * position).xy;\n"
"    v_normal = normalize(uNormalMatrix * normal.xyz);\n"
"    v_shadow = uLightMatrix * uModelMatrix * position;\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[0][2], uLightMatrix[1][2], uLightMatrix[2][2]))*1.2;\n"
"    gl_Position = uViewProjectionMatrix * uModelMatrix * position;\n"
"}\n";

static LPCSTR mdx_fs =
"#version 140\n"
"in vec2 v_texcoord;\n"
"in vec2 v_texcoord2;\n"
"in vec4 v_shadow;\n"
"in vec3 v_normal;\n"
"in vec3 v_lightDir;\n"
"out vec4 o_color;\n"
"uniform sampler2D uTexture;\n"
"uniform sampler2D uShadowmap;\n"
"uniform sampler2D uFogOfWar;\n"
"uniform bool uUseDiscard;\n"
"float get_light() {\n"
"    return dot(v_normal, v_lightDir);\n"
"}\n"
"float get_shadow() {\n"
"    float depth = texture(uShadowmap, vec2(v_shadow.x + 1.0, v_shadow.y + 1.0) * 0.5).r;\n"
"    return depth < (v_shadow.z + 0.99) * 0.5 ? 0.0 : 1.0;\n"
"}\n"
"float get_lighting() {\n"
"    return min(1.0, mix(0.35, 1.0, get_shadow() * get_light()) * 1.1);"
"}\n"
"float get_fogofwar() {\n"
"    return texture(uFogOfWar, v_texcoord2).r;\n"
"}\n"
"void main() {\n"
"    vec4 col = texture(uTexture, v_texcoord);\n"
"    col.rgb *= get_fogofwar() * get_lighting();\n"
"    o_color = col;\n"
"    if (o_color.a < 0.5 && uUseDiscard) discard;\n"
"}\n";

static bool R_InitUIModelView(LPCMODEL model,
                              LPCRECT viewport,
                              viewDef_t *viewdef,
                              renderEntity_t *entity,
                              mdxSequence_t const *seq) {
    if (!model || !model->mdx) {
        return false;
    }

    memset(entity, 0, sizeof(*entity));
    memset(viewdef, 0, sizeof(*viewdef));

    entity->scale = 1;
    entity->model = model;
    if (!seq) {
        return false;
    }

    {
        DWORD seq_len = seq->interval[1] - seq->interval[0];
        if (seq_len == 0) {
            seq_len = 1;
        }
        entity->frame = seq->interval[0] + (tr.viewDef.time % seq_len);
        entity->oldframe = entity->frame;
    }

    viewdef->viewport = *viewport;
    viewdef->scissor = (RECT) { 0, 0, 1, 1 };
    viewdef->num_entities = 1;
    viewdef->entities = entity;
    viewdef->rdflags |= RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL;

    return true;
}

void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotate(output, angles, ROTATE_ZYX);
    Matrix4_translate(output, &vieworg);
}

void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, 100.0, 3500.0);
    Matrix4_fromViewAngles(target, sunangles, 1000, &view);
    Matrix4_multiply(&proj, &view, output);
}

void R_DrawPortrait(LPCMODEL model, LPCRECT viewport, LPCSTR anim) {
    VECTOR3 root;
    VECTOR3 lightAngles = { 10, 270, 0 };
    renderEntity_t entity;
    viewDef_t viewdef;
    mdxModel_t const *mdx;
    mdxSequence_t const *seq;
    float aspect;

    if (!model || !model->mdx) {
        return;
    }
    mdx = model->mdx;
    seq = (anim && *anim) ? MDLX_FindSequenceByName(mdx, anim) : NULL;
    if (!seq && mdx->sequences && mdx->num_sequences > 0) {
        seq = &mdx->sequences[0];
    }

    if (!R_InitUIModelView(model, viewport, &viewdef, &entity, seq)) {
        return;
    }

    aspect = viewport->h > 0.0f ? viewport->w / viewport->h : 1.0f;
    if (!R_GetModelCameraMatrix(mdx, aspect, &viewdef.viewProjectionMatrix, &root)) {
        if (!R_IsSpriteUIScreenSpace(mdx)) {
            VECTOR3 const center = Box3_Center(&mdx->bounds.box);
            entity.origin = Vector3_unm(&center);
        }
        R_GetSpriteOrthoCameraMatrix(mdx, aspect, &viewdef.viewProjectionMatrix, &root);
    }
    Matrix4_getLightMatrix(&lightAngles, &root, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);

    R_Call(glActiveTexture, GL_TEXTURE2);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.texture[TEX_WHITE]->texid);
    R_Call(glActiveTexture, GL_TEXTURE0);

    tr.viewDef = viewdef;

    R_RenderShadowMap();
    R_RenderView();
}

void MDLX_Init(void) {
    mdlx.shader = R_InitShader(mdx_vs, mdx_fs);
}

void MDLX_Shutdown(void) {
    R_ReleaseShader(mdlx.shader);
}
