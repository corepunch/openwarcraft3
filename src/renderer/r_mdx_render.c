#include "r_local.h"
#include "r_mdx.h"

static struct matrix4 node_matrices[MAX_BONE_MATRICES];

DWORD GetModelKeyTrackDataTypeSize(enum tModelKeyTrackDataType dwDataType);
DWORD GetModelKeyTrackTypeSize(enum tModelKeyTrackType dwKeyTrackType);
DWORD GetModelKeyFrameSize(enum tModelKeyTrackDataType dwDataType,
                           enum tModelKeyTrackType dwKeyTrackType);

static void
R_GetKeyframeValue(struct tModelKeyFrame const *lpKeyframe1,
                   struct tModelKeyFrame const *lpKeyframe2,
                   DWORD const dwTime,
                   enum tModelKeyTrackDataType dwDataType,
                   HANDLE out)
{
    float const t = (float)(dwTime - lpKeyframe1->time) / (float)(lpKeyframe2->time - lpKeyframe1->time);
    switch (dwDataType) {
        case kModelKeyTrackDataTypeFloat:
            *((float *)out) = (*(float *)lpKeyframe1->data) * (1 - t) + (*(float *)lpKeyframe2->data) * t;
            return;
        case kModelKeyTrackDataTypeVector3:
            *((VECTOR3 *)out) = Vector3_lerp((VECTOR3 *)lpKeyframe1->data, (VECTOR3 *)lpKeyframe2->data, t);
            return;
        case kModelKeyTrackDataTypeQuaternion:
            *((VECTOR4 *)out) = quaternion_lerp((VECTOR4 *)lpKeyframe1->data, (VECTOR4 *)lpKeyframe2->data, t);
            return;
    }
}

static void
R_GetKeytrackValue(LPCMODEL lpModel,
                   struct tModelKeyTrack const *lpKeytrack,
                   DWORD dwFrameNumber,
                   HANDLE out)
{
    DWORD const dwKeyframeSize = GetModelKeyFrameSize(lpKeytrack->datatype, lpKeytrack->type);
    char const *lpKeyFrames = (LPSTR)lpKeytrack->values;
    struct tModelKeyFrame *lpPrevKeyFrame = NULL;
    FOR_LOOP(dwKeyframeIndex, lpKeytrack->dwKeyframeCount) {
        struct tModelKeyFrame *lpKeyFrame = (HANDLE)(lpKeyFrames + dwKeyframeSize * dwKeyframeIndex);
        if (lpKeyFrame->time == dwFrameNumber || (lpKeyFrame->time > dwFrameNumber && !lpPrevKeyFrame)) {
            memcpy(out, lpKeyFrame->data, GetModelKeyTrackDataTypeSize(lpKeytrack->datatype));
            return;
        }
        if (lpKeyFrame->time > dwFrameNumber) {
            R_GetKeyframeValue(lpPrevKeyFrame, lpKeyFrame, dwFrameNumber, lpKeytrack->datatype, out);
            return;
        }
        lpPrevKeyFrame = lpKeyFrame;
    }
//    return lpKeytrack->values[0].value;
}

static void R_CalculateNodeMatrix(LPCMODEL lpModel,
                                  struct tModelNode const *lpNode,
                                  DWORD dwFrameNumber,
                                  LPMATRIX4 lpMatrix)
{
    VECTOR3 vTranslation = { 0, 0, 0 };
    VECTOR4 vRotation = { 0, 0, 0, 1 };
    VECTOR3 vScale = { 1, 1, 1 };
    LPCVECTOR3 lpPivot = (VECTOR3 const *)lpNode->lpPivot;
    if (lpNode->lpTranslation) {
        R_GetKeytrackValue(lpModel, lpNode->lpTranslation, dwFrameNumber, &vTranslation);
    }
    if (lpNode->lpRotation) {
        R_GetKeytrackValue(lpModel, lpNode->lpRotation, dwFrameNumber, &vRotation);
    }
    if (lpNode->lpScale) {
        R_GetKeytrackValue(lpModel, lpNode->lpScale, dwFrameNumber, &vScale);
    }
    if (!lpNode->lpTranslation && !lpNode->lpRotation && !lpNode->lpScale) {
        Matrix4_identity(lpMatrix);
    } else if (lpNode->lpTranslation && !lpNode->lpRotation && !lpNode->lpScale) {
        Matrix4_from_translation(lpMatrix, &vTranslation);
    } else if (!lpNode->lpTranslation && lpNode->lpRotation && !lpNode->lpScale) {
        Matrix4_from_rotation_origin(lpMatrix,  &vRotation, lpPivot);
    } else {
        Matrix4_from_rotation_translation_scale_origin(lpMatrix, &vRotation, &vTranslation, &vScale, lpPivot);
    }
}

LPCMATRIX4 R_GetNodeGlobalMatrix(struct tModelNode *lpNode) {
    if (lpNode->globalMatrix.v[15] == 0) {
        if (lpNode->lpParent) {
            Matrix4_multiply(R_GetNodeGlobalMatrix(lpNode->lpParent),
                             &lpNode->localMatrix,
                             &lpNode->globalMatrix);
        } else {
            lpNode->globalMatrix = lpNode->localMatrix;
        }
    }
    return &lpNode->globalMatrix;
}

static void R_CalculateBoneMatrices(LPCMODEL lpModel,
                                    LPMATRIX4 lpModelMatrices,
                                    DWORD dwFrameNumber)
{
    DWORD dwBoneIndex = 1;
    
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        memset(&lpBone->node.globalMatrix, 0, sizeof(struct matrix4));
        R_CalculateNodeMatrix(lpModel, &lpBone->node, dwFrameNumber, &lpBone->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelHelper, lpHelper, lpModel->lpHelpers) {
        memset(&lpHelper->node.globalMatrix, 0, sizeof(struct matrix4));
        R_CalculateNodeMatrix(lpModel, &lpHelper->node, dwFrameNumber, &lpHelper->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        lpModelMatrices[dwBoneIndex++] = *R_GetNodeGlobalMatrix(&lpBone->node);
    }
}

static void R_RenderGeoset(LPCMODEL lpModel,
                           struct tModelGeoset *lpGeoset,
                           LPCMATRIX4 lpModelMatrix)
{
    glUseProgram(tr.shaderSkin->progid);
    glUniformMatrix4fv(tr.shaderSkin->u_model_matrix, 1, GL_FALSE, lpModelMatrix->v);
    glBindVertexArray(lpGeoset->lpBuffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lpGeoset->lpBuffer->vbo);
    glDrawArrays(GL_TRIANGLES, 0, lpGeoset->numTriangles);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static void R_BindBoneMatrices(LPMODEL lpModel, DWORD dwFrameNumber)
{
    struct matrix4 aBoneMatrices[MAX_BONE_MATRICES];

    R_CalculateBoneMatrices(lpModel, aBoneMatrices, dwFrameNumber);
    
    Matrix4_identity(node_matrices);

    FOR_EACH_LIST(struct tModelHelper, bone, lpModel->lpHelpers) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    FOR_EACH_LIST(struct tModelBone, bone, lpModel->lpBones) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    glUseProgram(tr.shaderSkin->progid);
    glUniformMatrix4fv(tr.shaderSkin->u_bones, 64, GL_FALSE, node_matrices->v);
}

static void RenderGeoset(LPMODEL lpModel,
                         struct tModelGeoset *lpGeoset,
                         struct tModelMaterialLayer *lpLayer,
                         struct render_entity const *lpEntity)
{
    if (lpGeoset->lpGeosetAnim && lpGeoset->lpGeosetAnim->lpAlphas) {
        float fAlpha = 1.f;
        R_GetKeytrackValue(lpModel, lpGeoset->lpGeosetAnim->lpAlphas, lpEntity->frame, &fAlpha);
        if (fAlpha < 1e-6) {
            return;
        }
    }
    
    struct matrix4 model_matrix;
    Matrix4_identity(&model_matrix);
    Matrix4_translate(&model_matrix, &lpEntity->origin);
    Matrix4_rotate(&model_matrix, &(VECTOR3){0, 0, lpEntity->angle * 180 / 3.14f}, ROTATE_XYZ);
    Matrix4_scale(&model_matrix, &(VECTOR3){lpEntity->scale, lpEntity->scale, lpEntity->scale});

    switch (lpLayer->blendMode) {
        case TEXOP_LOAD:
            glBlendFunc(GL_ONE, GL_ZERO);
            break;
        case TEXOP_TRANSPARENT:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case TEXOP_BLEND:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case TEXOP_ADD:
            glBlendFunc(GL_ONE, GL_ONE);
            break;
        case TEXOP_ADD_ALPHA:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        default:
            glBlendFunc(GL_ONE, GL_ZERO);
            break;
    }
    
    R_RenderGeoset(lpModel, lpGeoset, &model_matrix);
}

void RenderModel(struct render_entity const *lpEdict) {
    struct tModelMaterial *lpMaterial = lpEdict->model->lpMaterials;
    struct tModelGeoset *lpGeoset = lpEdict->model->lpGeosets;
    LPMODEL lpModel = lpEdict->model;

    if (lpEdict->skin) {
        R_BindTexture(lpEdict->skin, 0);
    }
    
    R_BindBoneMatrices(lpModel, lpEdict->frame);
    
    for (DWORD dwGeosetID = 0;
         lpMaterial && lpGeoset;
         lpGeoset = lpGeoset->lpNext, dwGeosetID++)
    {
        if (dwGeosetID < lpEdict->model->numTextures) {
            struct tModelTexture const *mtex = &lpEdict->model->lpTextures[dwGeosetID];
            LPCTEXTURE tex = R_FindTextureByID(mtex->texid);
            if (tex) {
                R_BindTexture(tex, 0);
            }
        }
        FOR_LOOP(dwLayerID, lpMaterial->num_layers) {
            RenderGeoset(lpModel, lpGeoset, &lpMaterial->layers[dwLayerID], lpEdict);
        }
        if (lpMaterial->lpNext) {
            lpMaterial = lpMaterial->lpNext;
        }
    }
}
