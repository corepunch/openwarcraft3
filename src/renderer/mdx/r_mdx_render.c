#include "r_mdx.h"
#include "r_local.h"

static MATRIX4 local_matrices[MAX_NODES];
  MATRIX4 global_matrices[MAX_NODES];
static MATRIX4 aBoneMatrices[MAX_BONES];

DWORD GetModelKeyTrackDataTypeSize(MODELKEYTRACKDATATYPE dataType);
DWORD GetModelKeyTrackTypeSize(MODELKEYTRACKTYPE keyTrackType);
DWORD GetModelKeyFrameSize(MODELKEYTRACKDATATYPE dataType, MODELKEYTRACKTYPE keyTrackType);
void R_GetKeyframeValue(mdxKeyFrame_t const *left, mdxKeyFrame_t const *right, DWORD time, mdxKeyTrack_t const *keytrack, HANDLE out);

mdxSequence_t const *R_FindSequenceAtTime(mdxModel_t const *model, DWORD time) {
    FOR_LOOP(seqIndex, model->num_sequences) {
        mdxSequence_t const *seq = &model->sequences[seqIndex];
        if (seq->interval[0] <= time && seq->interval[1] > time) {
            return seq;
        }
    }
    return NULL;
}

static void R_GetModelKeytrackValue(mdxModel_t const *model, mdxKeyTrack_t const *keytrack, DWORD time, HANDLE output) {
    DWORD const keyframeSize = GetModelKeyFrameSize(keytrack->datatype, keytrack->type);
    LPCSTR keyFrames = (LPCSTR)keytrack->values;
    mdxKeyFrame_t *prevKeyFrame = NULL;
    DWORD interval[2] = { 0, 0 };
    if (keytrack->globalSeqId != -1) {
        interval[0] = 0;
        interval[1] = model->globalSequences[keytrack->globalSeqId].value;
        time = time % (model->globalSequences[keytrack->globalSeqId].value + 1);
    } else {
        mdxSequence_t const *seq = R_FindSequenceAtTime(model, time);
        if (!seq)
            return;
        interval[0] = seq->interval[0];
        interval[1] = seq->interval[1];
    }
    FOR_LOOP(keyframeIndex, keytrack->keyframeCount) {
        mdxKeyFrame_t *keyFrame = (HANDLE)(keyFrames + keyframeSize * keyframeIndex);
        if (keyFrame->time < interval[0])
            continue;
        if (keyFrame->time > interval[1]) {
            if (prevKeyFrame) {
                memcpy(output, prevKeyFrame->data, GetModelKeyTrackDataTypeSize(keytrack->datatype));
            }
            return;
        }
        if (keyFrame->time == time || (keyFrame->time > time && !prevKeyFrame)) {
            memcpy(output, keyFrame->data, GetModelKeyTrackDataTypeSize(keytrack->datatype));
            return;
        }
        if (keyFrame->time > time) {
            R_GetKeyframeValue(prevKeyFrame, keyFrame, time, keytrack, output);
            return;
        }
        prevKeyFrame = keyFrame;
    }
    if (prevKeyFrame) {
        memcpy(output, prevKeyFrame->data, GetModelKeyTrackDataTypeSize(keytrack->datatype));
    }
}

static void R_CalculateNodeMatrix(mdxModel_t const *model, mdxNode_t *node, DWORD frame1, DWORD frame0, LPMATRIX4 matrix) {
    VECTOR3 vTranslation = { 0, 0, 0 };
    QUATERNION vRotation = { 0, 0, 0, 1 };
    VECTOR3 vScale = { 1, 1, 1 };
    LPCVECTOR3 pivot = (VECTOR3 const *)&model->pivots[node->object_id];
    if (frame0 != frame1) {
        if (node->translation) {
            VECTOR3 t0 = vTranslation, t1 = vTranslation;
            R_GetModelKeytrackValue(model, node->translation, frame0, &t0);
            R_GetModelKeytrackValue(model, node->translation, frame1, &t1);
            vTranslation = Vector3_lerp(&t0, &t1, tr.viewDef.lerpfrac);
        }
        if (node->rotation) {
            QUATERNION r0 = vRotation, r1 = vRotation;
            R_GetModelKeytrackValue(model, node->rotation, frame0, &r0);
            R_GetModelKeytrackValue(model, node->rotation, frame1, &r1);
            vRotation = Quaternion_slerp(&r0, &r1, tr.viewDef.lerpfrac);
        }
        if (node->scale) {
            VECTOR3 s0 = vScale, s1 = vScale;
            R_GetModelKeytrackValue(model, node->scale, frame0, &s0);
            R_GetModelKeytrackValue(model, node->scale, frame1, &s1);
            vScale = Vector3_lerp(&s0, &s1, tr.viewDef.lerpfrac);
        }
    } else {
        if (node->translation) {
            R_GetModelKeytrackValue(model, node->translation, frame1, &vTranslation);
        }
        if (node->rotation) {
            R_GetModelKeytrackValue(model, node->rotation, frame1, &vRotation);
        }
        if (node->scale) {
            R_GetModelKeytrackValue(model, node->scale, frame1, &vScale);
        }
    }
    if (!node->translation && !node->rotation && !node->scale) {
        Matrix4_identity(matrix);
    } else if (node->translation && !node->rotation && !node->scale) {
        Matrix4_from_translation(matrix, &vTranslation);
    } else if (!node->translation && node->rotation && !node->scale) {
        Matrix4_from_rotation_origin(matrix,  &vRotation, pivot);
    } else {
        Matrix4_from_rotation_translation_scale_origin(matrix, &vRotation, &vTranslation, &vScale, pivot);
    }
}

LPCMATRIX4 R_GetNodeGlobalMatrix(mdxModel_t const *model, mdxNode_t *node) {
    if (global_matrices[node->object_id].v[15] == 0) {
        if (node->parent_id != -1) {
            Matrix4_multiply(R_GetNodeGlobalMatrix(model, model->nodes[node->parent_id]),
                             &local_matrices[node->object_id],
                             &global_matrices[node->object_id]);
        } else {
            global_matrices[node->object_id] = local_matrices[node->object_id];
        }
    }
    return &global_matrices[node->object_id];
}

DWORD R_CalculateBoneMatrices(mdxModel_t const *model, LPMATRIX4 modelMatrices, DWORD frame1, DWORD frame0) {
    DWORD boneIndex = 1;
    
    memset(global_matrices, 0, sizeof(global_matrices));
    
    for (mdxNode_t *const *node = model->nodes; *node; node++) {
        R_CalculateNodeMatrix(model, *node, frame1, frame0, &local_matrices[(*node)->object_id]);
    }
    
    FOR_EACH_LIST(mdxBone_t, bone, model->bones) {
        modelMatrices[boneIndex++] = *R_GetNodeGlobalMatrix(model, &bone->node);
    }
    
    return boneIndex;
}

static void R_RenderGeoset(mdxModel_t const *model,
                           mdxGeoset_t const *geoset,
                           LPCMATRIX4 modelMatrix) {
    if (!geoset->userdata)
        return;
    
    struct render_buffer *buf = geoset->userdata;
    
    MATRIX3 mNormalMatrix;
    Matrix3_normal(&mNormalMatrix, modelMatrix);
    R_Call(glUseProgram, tr.shaderSkin->progid);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uModelMatrix, 1, GL_FALSE, modelMatrix->v);
    R_Call(glUniformMatrix3fv, tr.shaderSkin->uNormalMatrix, 1, GL_TRUE, mNormalMatrix.v);
    R_Call(glBindVertexArray, buf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buf->vbo);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, geoset->num_triangles);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void R_BindBoneMatrices(mdxModel_t const *model, DWORD frame1, DWORD frame0) {
    DWORD numBones = R_CalculateBoneMatrices(model, aBoneMatrices, frame1, frame0);
    
    R_Call(glUseProgram, tr.shaderSkin->progid);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uBones, numBones, GL_FALSE, global_matrices->v);
}

static void RenderGeoset(mdxModel_t const *model,
                         mdxGeoset_t const *geoset,
                         renderEntity_t const *entity,
                         LPCTEXTURE overrideTexture)
{
    mdxMaterial_t *material = model->materials;
    for (DWORD materialID = geoset->materialID; materialID > 0; materialID--) {
        material = material->next;
    }
    if (geoset->geosetAnim && geoset->geosetAnim->alphas) {
        float fAlpha = 1.f;
        R_GetModelKeytrackValue(model, geoset->geosetAnim->alphas, entity->frame, &fAlpha);
        if (fAlpha < EPSILON) {
            return;
        }
    }

    MATRIX4 mModelMatrix;
    Matrix4_identity(&mModelMatrix);
    Matrix4_translate(&mModelMatrix, &entity->origin);
    Matrix4_rotate(&mModelMatrix, &(VECTOR3){0, 0, entity->angle * 180 / 3.14f}, ROTATE_XYZ);
    Matrix4_scale(&mModelMatrix, &(VECTOR3){entity->scale, entity->scale, entity->scale});

    R_Call(glUniform1i, tr.shaderSkin->uUseDiscard, 0);

    extern bool is_rendering_lights;

    FOR_LOOP(layerID, material->num_layers) {
        mdxMaterialLayer_t const *layer = &material->layers[layerID];
        mdxTexture_t const *modeltex = &model->textures[layer->textureId];
        switch (modeltex->replaceableID) {
            case TEXREPL_TEAMCOLOR:
                R_BindTexture(tr.teamColor[entity->team & TEAM_MASK], 0);
                break;
            case TEXREPL_TEAMGLOW:
                R_BindTexture(tr.teamGlow[entity->team & TEAM_MASK], 0);
                break;
            case TEXREPL_NONE:
                R_BindTexture(R_FindTextureByID(modeltex->texid), 0);
                break;
            case TEXREPL_TEXTURE:
                R_BindTexture(overrideTexture, 0);
                break;
            default:
//                printf("%d\n", modeltex->replaceableID);
                break;
        }
        switch (layer->blendMode) {
            case TEXOP_LOAD:
                if (layerID == 0) {
                    R_Call(glBlendFunc, GL_ONE, GL_ZERO);
                } else {
                    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                R_Call(glDepthMask, GL_TRUE);
                break;
            case TEXOP_TRANSPARENT:
                R_Call(glUniform1i, tr.shaderSkin->uUseDiscard, 1);
                R_Call(glBlendFunc, GL_ONE, GL_ZERO);
                R_Call(glDepthMask, GL_TRUE);
                break;
            case TEXOP_BLEND:
                if (is_rendering_lights)
                    return;
                R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                R_Call(glDepthMask, GL_FALSE);
                break;
#ifdef DEBUG_PATHFINDING
            default:
                return;
#else
            case TEXOP_ADD:
                if (is_rendering_lights)
                    return;
                R_Call(glBlendFunc, GL_ONE, GL_ONE);
                R_Call(glDepthMask, GL_FALSE);
                break;
            case TEXOP_ADD_ALPHA:
                if (is_rendering_lights)
                    return;
                R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE);
                R_Call(glDepthMask, GL_FALSE);
                break;
            default:
                R_Call(glBlendFunc, GL_ONE, GL_ZERO);
                R_Call(glDepthMask, GL_TRUE);
                break;
#endif
        }
        R_RenderGeoset(model, geoset, &mModelMatrix);
    }
}

void RenderModel(renderEntity_t const *entity) {
    mdxModel_t const *model = entity->model->mdx;

    R_BindBoneMatrices(model, entity->frame, entity->oldframe);

    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        RenderGeoset(model, geoset, entity, entity->skin);
    }

    if (entity->flags & RF_SELECTED) {
        FOR_LOOP(boneIndex, MAX_BONES) {
            Matrix4_identity(&aBoneMatrices[boneIndex]);
        }
        R_Call(glUseProgram, tr.shaderSkin->progid);
        R_Call(glUniformMatrix4fv, tr.shaderSkin->uBones, MAX_BONES, GL_FALSE, aBoneMatrices->v);
        renderEntity_t re = *entity;
        re.scale *= 1.5f;
        re.angle = tr.viewDef.time * 0.001;
        re.frame = 0;
        re.oldframe = 0;
        RenderGeoset(tr.selectionCircle->mdx, tr.selectionCircle->mdx->geosets, &re, NULL);
    }
}

void R_Viewport(LPCRECT viewport) {
    SIZE2 const windowSize = R_GetWindowSize();
    glViewport(viewport->x * windowSize.width / 800,
               viewport->y * windowSize.height / 600,
               viewport->width * windowSize.width / 800,
               viewport->height * windowSize.height / 600);
}

bool R_GetModelCameraMatrix(mdxModel_t const *model, LPMATRIX4 output, LPVECTOR3 root) {
    mdxCamera_t const *camera = model->cameras;
    if (!camera) {
        return false;
    } else {
        MATRIX4 projection, view;
        Matrix4_perspective(&projection, 30, 1, 10.0, 1000.0);
        VECTOR3 dir = Vector3_sub((LPVECTOR3)camera->targetPivot, (LPVECTOR3)camera->pivot);
        Matrix4_lookAt(&view, (LPVECTOR3)camera->pivot, &dir, &(VECTOR3){0,0,1});
        Matrix4_multiply(&projection, &view, output);
        *root = *(LPCVECTOR3)model->pivots;
        return true;
    }
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

void R_DrawPortrait(model_t const *model, LPCRECT viewport) {
    VECTOR3 root;
    VECTOR3 lightAngles = { 10, 270, 0 };
    renderEntity_t entity;
    viewDef_t viewdef;
    mdxModel_t const *mdx = model->mdx;
    mdxSequence_t const *seq = &mdx->sequences[2];
    
    memset(&entity, 0, sizeof(renderEntity_t));
    memset(&viewdef, 0, sizeof(viewdef));
    
    entity.scale = 1;
    entity.model = model;
    entity.frame = seq->interval[0]  + tr.viewDef.time % (seq->interval[1] - seq->interval[0]);
    entity.oldframe = entity.frame;
    
    viewdef.viewport = *viewport;
    viewdef.scissor = (RECT) { 0, 0, 1, 1 };
    viewdef.num_entities = 1;
    viewdef.entities = &entity;
    viewdef.rdflags |= RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL;
    
    R_GetModelCameraMatrix(mdx, &viewdef.projectionMatrix, &root);
    
    Matrix4_getLightMatrix(&lightAngles, &root, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);

    R_RenderFrame(&viewdef);
}
