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
"uniform bool uUnshaded;\n"
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
"    if (!uUnshaded) {\n"
"        col.rgb *= get_fogofwar() * get_lighting();\n"
"    }\n"
"    o_color = col;\n"
"    if (o_color.a < 0.5 && uUseDiscard) discard;\n"
"}\n";

static bool R_InitUIModelView(LPCMODEL model,
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

static bool
R_GetModelCameraMatrix(mdxModel_t const *model, DWORD frame, float aspect, LPMATRIX4 output, LPVECTOR3 root)
{
    if (!model || !model->cameras) {
        return false;
    }

    mdxCamera_t const *camera = model->cameras;
    MATRIX4 projection, view;
    VECTOR3 eye = camera->pivot;
    VECTOR3 target = camera->targetPivot;
    VECTOR3 dir;
    VECTOR3 up = {0, 0, 1};
    float fov_deg = camera->fieldOfView * (180.0f / (float)M_PI);
    float near_clip = camera->nearClip;
    float far_clip = camera->farClip;
    float roll = 0.0f;

    if (!isfinite(fov_deg) || fov_deg <= 1.0f || fov_deg >= 179.0f) {
        fov_deg = 35.0f;
    }
    if (!isfinite(near_clip) || near_clip < 0.01f) {
        near_clip = 1.0f;
    }
    if (!isfinite(far_clip) || far_clip <= near_clip + 1.0f) {
        far_clip = near_clip + 5000.0f;
    }
    if (!isfinite(aspect) || aspect <= 0.0f) {
        aspect = 1.0f;
    }
    if (camera->translation) {
        VECTOR3 translation = {0, 0, 0};
        MDLX_GetModelKeytrackValue(model, camera->translation, frame, &translation);
        eye = Vector3_add(&eye, &translation);
    }
    if (camera->targetTranslation) {
        VECTOR3 targetTranslation = {0, 0, 0};
        MDLX_GetModelKeytrackValue(model, camera->targetTranslation, frame, &targetTranslation);
        target = Vector3_add(&target, &targetTranslation);
    }
    dir = Vector3_sub(&target, &eye);
    if (Vector3_len(&dir) < 0.001f) {
        return false;
    }
    if (camera->roll) {
        MDLX_GetModelKeytrackValue(model, camera->roll, frame, &roll);
        if (isfinite(roll) && fabsf(roll) > 0.0001f) {
            up = Vector3_rotateAroundAxis(&up, &dir, roll);
        }
    }
    float const camera_aspect = 1.66f;
    fov_deg = 2.0f * atanf(tanf(fov_deg * (float)M_PI / 360.0f) / camera_aspect) * 180.0f / (float)M_PI;
    Matrix4_perspective(&projection, fov_deg, aspect, near_clip, far_clip);
    Matrix4_lookAt(&view, &eye, &dir, &up);
    Matrix4_multiply(&projection, &view, output);
    *root = target;
    return true;
}

void R_DrawPortrait(LPCMODEL model, LPCRECT viewport, LPCSTR anim) {
    VECTOR3 root;
    VECTOR3 lightAngles = { 10, 270, 0 };
    renderEntity_t entity;
    viewDef_t viewdef;
    
    if (!model || !model->mdx) {
        return;
    }
    
    mdxModel_t const *mdx = model->mdx;
    mdxSequence_t const *seq = (anim && *anim) ? MDLX_FindSequenceByName(mdx, anim) : NULL;

    float viewport_width = viewport->w * tr.drawableSize.width;
    float viewport_height = viewport->h * tr.drawableSize.height;
    float aspect = viewport_height > 0.0f ? viewport_width / viewport_height : 1.0f;

    if (!seq && mdx->sequences && mdx->num_sequences > 0) {
        seq = &mdx->sequences[0];
    }

    if (!R_InitUIModelView(model, &viewdef, &entity, seq)) {
        return;
    }

    viewdef.viewport = *viewport;

    if (!R_GetModelCameraMatrix(mdx, entity.frame, aspect, &viewdef.viewProjectionMatrix, &root)) {
        return;
    }

    Matrix4_getLightMatrix(&lightAngles, &root, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);

    R_Call(glActiveTexture, GL_TEXTURE2);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.texture[TEX_WHITE]->texid);
    R_Call(glActiveTexture, GL_TEXTURE0);

    tr.viewDef = viewdef;

    R_RenderShadowMap();
    R_RenderView();
}

void R_DrawSprite(LPCMODEL model, LPCSTR anim, float x, float y) {
    renderEntity_t entity;
    viewDef_t viewdef;

    if (!model || !model->mdx) {
        return;
    }
    mdxModel_t const *mdx = model->mdx;
    mdxSequence_t const *seq = (anim && *anim) ? MDLX_FindSequenceByName(mdx, anim) : NULL;

    VECTOR3 const center = Box3_Center(&mdx->bounds.box);

    if (!seq && mdx->sequences && mdx->num_sequences > 0) {
        seq = &mdx->sequences[0];
    }

    if (!R_InitUIModelView(model, &viewdef, &entity, seq)) {
        return;
    }

    viewdef.viewport = (struct rect) {0,0,1,1};

    entity.flags |= RF_NO_FOGOFWAR | RF_NO_SHADOW | RF_NO_LIGHTING;
    // this only works for TOPLEFT/TOPRIGHT anchored sprites, but that's all we have for now
    entity.origin = (VECTOR3){x+center.x, 1.2-y+mdx->bounds.box.min.y, 0};
    // entity.origin = (VECTOR3){x+mdx->bounds.box.min.x, y-center.y, 0};
    // printf("%f\n", y);

    RECT screen = R_UISceneRect();
    Matrix4_ortho(&viewdef.viewProjectionMatrix, screen.x, screen.x + screen.w, screen.y, screen.y + screen.h, 0.0f, 100.0f);
    Matrix4_scale(&viewdef.viewProjectionMatrix, &(VECTOR3){1, 1, 0});

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
