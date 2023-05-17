#include "r_local.h"
#include "r_mdx.h"

static MATRIX4 node_matrices[MAX_BONE_MATRICES];

DWORD GetModelKeyTrackDataTypeSize(MODELKEYTRACKDATATYPE dwDataType);
DWORD GetModelKeyTrackTypeSize(MODELKEYTRACKTYPE dwKeyTrackType);
DWORD GetModelKeyFrameSize(MODELKEYTRACKDATATYPE dwDataType, MODELKEYTRACKTYPE dwKeyTrackType);
void R_GetKeyframeValue(LPCMODELKEYFRAME lpLeft, LPCMODELKEYFRAME lpRight, DWORD dwTime, LPCMODELKEYTRACK lpKeytrack, HANDLE out);

static LPCMODELSEQUENCE R_FindSequenceAtTime(LPCMODEL lpModel, DWORD dwTime) {
    FOR_LOOP(dwSeqIndex, lpModel->numSequences) {
        LPCMODELSEQUENCE lpSeq = &lpModel->lpSequences[dwSeqIndex];
        if (lpSeq->interval[0] <= dwTime && lpSeq->interval[1] > dwTime) {
            return lpSeq;
        }
    }
    return NULL;
}

//static void R_FindKeyframes(LPCMODEL lpModel, LPCMODELKEYTRACK lpKeytrack, DWORD dwTime, LPCMODELKEYFRAME *lpLeft, LPCMODELKEYFRAME *lpRight) {
//}
//
static void R_GetModelKeytrackValue(LPCMODEL lpModel, LPCMODELKEYTRACK lpKeytrack, DWORD dwTime, HANDLE lpOutput) {
    DWORD const dwKeyframeSize = GetModelKeyFrameSize(lpKeytrack->datatype, lpKeytrack->type);
    LPCSTR lpKeyFrames = (LPCSTR)lpKeytrack->values;
    LPMODELKEYFRAME lpPrevKeyFrame = NULL;
    LPCMODELSEQUENCE lpSeq = R_FindSequenceAtTime(lpModel, dwTime);
    if (!lpSeq)
        return;
    FOR_LOOP(dwKeyframeIndex, lpKeytrack->dwKeyframeCount) {
        LPMODELKEYFRAME lpKeyFrame = (HANDLE)(lpKeyFrames + dwKeyframeSize * dwKeyframeIndex);
        if (lpKeyFrame->time < lpSeq->interval[0])
            continue;
        if (lpKeyFrame->time > lpSeq->interval[1]) {
            if (lpPrevKeyFrame) {
                memcpy(lpOutput, lpPrevKeyFrame->data, GetModelKeyTrackDataTypeSize(lpKeytrack->datatype));
            }
            return;
        }
        if (lpKeyFrame->time == dwTime || (lpKeyFrame->time > dwTime && !lpPrevKeyFrame)) {
            memcpy(lpOutput, lpKeyFrame->data, GetModelKeyTrackDataTypeSize(lpKeytrack->datatype));
            return;
        }
        if (lpKeyFrame->time > dwTime) {
            R_GetKeyframeValue(lpPrevKeyFrame, lpKeyFrame, dwTime, lpKeytrack, lpOutput);
            return;
        }
        lpPrevKeyFrame = lpKeyFrame;
    }
//    return lpKeytrack->values[0].value;
}

static void R_CalculateNodeMatrix(LPCMODEL lpModel, LPMODELNODE lpNode, DWORD dwFrameNumber, LPMATRIX4 lpMatrix) {
    VECTOR3 vTranslation = { 0, 0, 0 };
    VECTOR4 vRotation = { 0, 0, 0, 1 };
    VECTOR3 vScale = { 1, 1, 1 };
    LPCVECTOR3 lpPivot = (VECTOR3 const *)lpNode->lpPivot;
    if (lpNode->lpTranslation) {
        R_GetModelKeytrackValue(lpModel, lpNode->lpTranslation, dwFrameNumber, &vTranslation);
    }
    if (lpNode->lpRotation) {
        R_GetModelKeytrackValue(lpModel, lpNode->lpRotation, dwFrameNumber, &vRotation);
    }
    if (lpNode->lpScale) {
        R_GetModelKeytrackValue(lpModel, lpNode->lpScale, dwFrameNumber, &vScale);
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

LPCMATRIX4 R_GetNodeGlobalMatrix(LPMODELNODE lpNode) {
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

static void R_CalculateBoneMatrices(LPCMODEL lpModel, LPMATRIX4 lpModelMatrices, DWORD dwFrameNumber) {
    DWORD dwBoneIndex = 1;
    
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        memset(&lpBone->node.globalMatrix, 0, sizeof(MATRIX4));
        R_CalculateNodeMatrix(lpModel, &lpBone->node, dwFrameNumber, &lpBone->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelHelper, lpHelper, lpModel->lpHelpers) {
        memset(&lpHelper->node.globalMatrix, 0, sizeof(MATRIX4));
        R_CalculateNodeMatrix(lpModel, &lpHelper->node, dwFrameNumber, &lpHelper->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        lpModelMatrices[dwBoneIndex++] = *R_GetNodeGlobalMatrix(&lpBone->node);
    }
}

static void R_RenderGeoset(LPCMODEL lpModel, LPMODELGEOSET lpGeoset, LPCMATRIX4 lpModelMatrix) {
    MATRIX3 mNormalMatrix;
    Matrix3_normal(&mNormalMatrix, lpModelMatrix);
    glUseProgram(tr.shaderSkin->progid);
    glUniformMatrix4fv(tr.shaderSkin->uModelMatrix, 1, GL_FALSE, lpModelMatrix->v);
    glUniformMatrix3fv(tr.shaderSkin->uNormalMatrix, 1, GL_TRUE, mNormalMatrix.v);

    glBindVertexArray(lpGeoset->lpBuffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, lpGeoset->lpBuffer->vbo);
    glDrawArrays(GL_TRIANGLES, 0, lpGeoset->numTriangles);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


static void R_BindBoneMatrices(LPMODEL lpModel, DWORD dwFrameNumber) {
    MATRIX4 aBoneMatrices[MAX_BONE_MATRICES];

    R_CalculateBoneMatrices(lpModel, aBoneMatrices, dwFrameNumber);
    
    Matrix4_identity(node_matrices);

    FOR_EACH_LIST(struct tModelHelper, bone, lpModel->lpHelpers) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    FOR_EACH_LIST(struct tModelBone, bone, lpModel->lpBones) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    glUseProgram(tr.shaderSkin->progid);
    glUniformMatrix4fv(tr.shaderSkin->uBones, 64, GL_FALSE, node_matrices->v);
}

static void RenderGeoset(LPCMODEL lpModel,
                         LPMODELGEOSET lpGeoset,
                         LPCMODELLAYER lpLayer,
                         LPCRENDERENTITY lpEntity)
{
    if (lpGeoset->lpGeosetAnim && lpGeoset->lpGeosetAnim->lpAlphas) {
        float fAlpha = 1.f;
        R_GetModelKeytrackValue(lpModel, lpGeoset->lpGeosetAnim->lpAlphas, lpEntity->frame, &fAlpha);
        if (fAlpha < EPSILON) {
            return;
        }
    }
    
    MATRIX4 mModelMatrix;
    Matrix4_identity(&mModelMatrix);
    Matrix4_translate(&mModelMatrix, &lpEntity->origin);
    Matrix4_rotate(&mModelMatrix, &(VECTOR3){0, 0, lpEntity->angle * 180 / 3.14f}, ROTATE_XYZ);
    Matrix4_scale(&mModelMatrix, &(VECTOR3){lpEntity->scale, lpEntity->scale, lpEntity->scale});
    
    glUniform1i(tr.shaderSkin->uUseDiscard, 0);

    switch (lpLayer->blendMode) {
        case TEXOP_LOAD:
            glBlendFunc(GL_ONE, GL_ZERO);
            break;
        case TEXOP_TRANSPARENT:
            glUniform1i(tr.shaderSkin->uUseDiscard, 1);
            glBlendFunc(GL_ONE, GL_ZERO);
//            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
    
    R_RenderGeoset(lpModel, lpGeoset, &mModelMatrix);
}

void RenderModel(LPCRENDERENTITY lpEdict) {
    struct tModelMaterial *lpMaterial = lpEdict->model->lpMaterials;
    LPMODELGEOSET lpGeoset = lpEdict->model->lpGeosets;
    LPCMODEL lpModel = lpEdict->model;

    if (lpEdict->skin) {
        R_BindTexture(lpEdict->skin, 0);
    }
    
    R_BindBoneMatrices((LPMODEL)lpModel, lpEdict->frame);
    
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
