#include "r_local.h"
#include "r_mdx.h"

static MATRIX4 node_matrices[MAX_BONE_MATRICES];

DWORD GetModelKeyTrackDataTypeSize(MODELKEYTRACKDATATYPE dataType);
DWORD GetModelKeyTrackTypeSize(MODELKEYTRACKTYPE keyTrackType);
DWORD GetModelKeyFrameSize(MODELKEYTRACKDATATYPE dataType, MODELKEYTRACKTYPE keyTrackType);
void R_GetKeyframeValue(LPCMODELKEYFRAME left, LPCMODELKEYFRAME right, DWORD time, LPCMODELKEYTRACK keytrack, HANDLE out);

LPCMODELSEQUENCE R_FindSequenceAtTime(LPCMODEL model, DWORD time) {
    FOR_LOOP(seqIndex, model->num_sequences) {
        LPCMODELSEQUENCE seq = &model->sequences[seqIndex];
        if (seq->interval[0] <= time && seq->interval[1] > time) {
            return seq;
        }
    }
    return NULL;
}

static void R_GetModelKeytrackValue(LPCMODEL model, LPCMODELKEYTRACK keytrack, DWORD time, HANDLE output) {
    DWORD const keyframeSize = GetModelKeyFrameSize(keytrack->datatype, keytrack->type);
    LPCSTR keyFrames = (LPCSTR)keytrack->values;
    LPMODELKEYFRAME prevKeyFrame = NULL;
    LPCMODELSEQUENCE seq = R_FindSequenceAtTime(model, time);
    if (!seq)
        return;
    FOR_LOOP(keyframeIndex, keytrack->keyframeCount) {
        LPMODELKEYFRAME keyFrame = (HANDLE)(keyFrames + keyframeSize * keyframeIndex);
        if (keyFrame->time < seq->interval[0])
            continue;
        if (keyFrame->time > seq->interval[1]) {
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

static void R_CalculateNodeMatrix(LPCMODEL model, LPMODELNODE node, DWORD frame1, DWORD frame0, LPMATRIX4 matrix) {
    VECTOR3 vTranslation = { 0, 0, 0 };
    QUATERNION vRotation = { 0, 0, 0, 1 };
    VECTOR3 vScale = { 1, 1, 1 };
    LPCVECTOR3 pivot = (VECTOR3 const *)node->pivot;
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

LPCMATRIX4 R_GetNodeGlobalMatrix(LPMODELNODE node) {
    if (node->globalMatrix.v[15] == 0) {
        if (node->parent) {
            Matrix4_multiply(R_GetNodeGlobalMatrix(node->parent),
                             &node->localMatrix,
                             &node->globalMatrix);
        } else {
            node->globalMatrix = node->localMatrix;
        }
    }
    return &node->globalMatrix;
}

static void R_CalculateBoneMatrices(LPCMODEL model, LPMATRIX4 modelMatrices, DWORD frame1, DWORD frame0) {
    DWORD boneIndex = 1;
    
    FOR_EACH_LIST(struct tModelBone, bone, model->bones) {
        memset(&bone->node.globalMatrix, 0, sizeof(MATRIX4));
        R_CalculateNodeMatrix(model, &bone->node, frame1, frame0, &bone->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelHelper, helper, model->helpers) {
        memset(&helper->node.globalMatrix, 0, sizeof(MATRIX4));
        R_CalculateNodeMatrix(model, &helper->node, frame1, frame0, &helper->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelBone, bone, model->bones) {
        modelMatrices[boneIndex++] = *R_GetNodeGlobalMatrix(&bone->node);
    }

}

static void R_RenderGeoset(LPCMODEL model, LPMODELGEOSET geoset, LPCMATRIX4 modelMatrix) {
    if (!geoset->buffer)
        return;
    MATRIX3 mNormalMatrix;
    Matrix3_normal(&mNormalMatrix, modelMatrix);
    glUseProgram(tr.shaderSkin->progid);
    glUniformMatrix4fv(tr.shaderSkin->uModelMatrix, 1, GL_FALSE, modelMatrix->v);
    glUniformMatrix3fv(tr.shaderSkin->uNormalMatrix, 1, GL_TRUE, mNormalMatrix.v);

    glBindVertexArray(geoset->buffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, geoset->buffer->vbo);
    glDrawArrays(GL_TRIANGLES, 0, geoset->num_triangles);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static MATRIX4 aBoneMatrices[MAX_BONE_MATRICES];

static void R_BindBoneMatrices(LPMODEL model, DWORD frame1, DWORD frame0) {
    R_CalculateBoneMatrices(model, aBoneMatrices, frame1, frame0);

    Matrix4_identity(node_matrices);

    FOR_EACH_LIST(struct tModelHelper, bone, model->helpers) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    FOR_EACH_LIST(struct tModelBone, bone, model->bones) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    glUseProgram(tr.shaderSkin->progid);
    glUniformMatrix4fv(tr.shaderSkin->uBones, 64, GL_FALSE, node_matrices->v);
}

static void RenderGeoset(LPCMODEL model,
                         LPMODELGEOSET geoset,
                         renderEntity_t const *entity,
                         LPCTEXTURE overrideTexture)
{
    LPMODELMATERIAL material = model->materials;
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

    glUniform1i(tr.shaderSkin->uUseDiscard, 0);

    extern bool is_rendering_lights;

    FOR_LOOP(layerID, material->num_layers) {
        LPCMODELLAYER layer = &material->layers[layerID];
        struct tModelTexture const *modeltex = &model->textures[layer->textureId];
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
                    glBlendFunc(GL_ONE, GL_ZERO);
                } else {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                glDepthMask(GL_TRUE);
                break;
            case TEXOP_TRANSPARENT:
                glUniform1i(tr.shaderSkin->uUseDiscard, 1);
                glBlendFunc(GL_ONE, GL_ZERO);
                glDepthMask(GL_TRUE);
                break;
            case TEXOP_BLEND:
                if (is_rendering_lights)
                    return;
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                break;
#ifdef DEBUG_PATHFINDING
            default:
                return;
#else
            case TEXOP_ADD:
                if (is_rendering_lights)
                    return;
                glBlendFunc(GL_ONE, GL_ONE);
                glDepthMask(GL_FALSE);
                break;
            case TEXOP_ADD_ALPHA:
                if (is_rendering_lights)
                    return;
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glDepthMask(GL_FALSE);
                break;
            default:
                glBlendFunc(GL_ONE, GL_ZERO);
                glDepthMask(GL_TRUE);
                break;
#endif
        }
        R_RenderGeoset(model, geoset, &mModelMatrix);
    }
}

void RenderModel(renderEntity_t const *entity) {
    LPCMODEL model = entity->model;

    R_BindBoneMatrices((LPMODEL)model, entity->frame, entity->oldframe);

    FOR_EACH_LIST(MODELGEOSET, geoset, model->geosets) {
        RenderGeoset(model, geoset, entity, entity->skin);
    }

    if (entity->flags & RF_SELECTED) {
        FOR_LOOP(boneIndex, MAX_BONE_MATRICES) {
            Matrix4_identity(&node_matrices[boneIndex]);
        }
        glUseProgram(tr.shaderSkin->progid);
        glUniformMatrix4fv(tr.shaderSkin->uBones, 64, GL_FALSE, node_matrices->v);
        renderEntity_t re = *entity;
        re.scale *= 1.5f;
        re.angle = tr.viewDef.time * 0.001;
        RenderGeoset(tr.selectionCircle, tr.selectionCircle->geosets, &re, NULL);
    }
}
