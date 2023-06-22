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

static void R_RenderGeoset(mdxGeoset_t const *geoset,
                           LPCMATRIX4 modelMatrix)
{
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

extern bool is_rendering_lights;

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
    R_GetEntityMatrix(entity, &mModelMatrix);

    R_Call(glUniform1i, tr.shaderSkin->uUseDiscard, 0);

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
        R_RenderGeoset(geoset, &mModelMatrix);
    }
#if 0
    if (!is_rendering_lights) {
        BOX3 box;
//        FOR_EACH_LIST(mdxCollisionShape_t, shape, model->collisionShapes) {
//            if (shape->type == SHAPETYPE_SPHERE) {
//                mdxVec3_t o;
//                memcpy(o, shape->vertex, 12);
//                shape->vertex[0][0] = o[0] - shape->radius;
//                shape->vertex[0][1] = o[1] - shape->radius;
//                shape->vertex[0][2] = o[2] - shape->radius;
//                shape->vertex[1][0] = o[0] + shape->radius;
//                shape->vertex[1][1] = o[1] + shape->radius;
//                shape->vertex[1][2] = o[2] + shape->radius;
//                shape->type = SHAPETYPE_BOX;
//            }
//                memcpy(&box.min, shape->vertex[0], 12);
//                memcpy(&box.max, shape->vertex[1], 12);
//                R_DrawBoundingBox(&box, &mModelMatrix,(COLOR32){255,0,0,255});
//        }
        mdxBounds_t b = geoset->default_bounds;
        memcpy(&box.min, b.min, 12);
        memcpy(&box.max, b.max, 12);

        R_DrawBoundingBox(&box, &mModelMatrix,(COLOR32){255,0,0,255});
//        FOR_LOOP(i, geoset->num_bounds) {
//            mdxBounds_t b = geoset->bounds[i];
//            memcpy(&box.min, b.min, 12);
//            memcpy(&box.max, b.max, 12);
//            R_DrawBoundingBox(&box, &mModelMatrix,(COLOR32){255,0,0,255});
//        }
    }
#endif
}

mdxSequence_t const *R_FindSequence(mdxModel_t const *model, LPCSTR name) {
    FOR_LOOP(i, model->num_sequences) {
        if (!strcmp(model->sequences[i].name, name)) {
            return &model->sequences[i];
        }
    }
    return NULL;
}

DWORD R_RemapAnimation(mdxModel_t const *model, DWORD frame, LPCSTR str) {
    mdxSequence_t const *seq = R_FindSequenceAtTime(model, frame);
    if (!seq) return frame;
    char buffer[64] = { 0 };
    sprintf(buffer, "%s %s", seq->name, str);
    mdxSequence_t const *other = R_FindSequence(model, buffer);
    if (other) {
        return frame + other->interval[0] - seq->interval[0];
    } else {
        return frame;
    }
}

VECTOR3 R_BoundsCenter(mdxBounds_t const *bounds) {
    return (VECTOR3) {
        .x = (bounds->min[0] + bounds->max[0]) / 2,
        .y = (bounds->min[1] + bounds->max[1]) / 2,
        .z = (bounds->min[2] + bounds->max[2]) / 2,
    };
}

bool R_TraceModel(renderEntity_t const *ent, LPCLINE3 line) {
    MATRIX4 invmodel, matmodel;
    R_GetEntityMatrix(ent, &matmodel);
    mdxModel_t const *model = ent->model->mdx;
    FOR_EACH_LIST(mdxCollisionShape_t, collisionShape, model->collisionShapes) {
        if (collisionShape->type != SHAPETYPE_SPHERE)
            continue;;
        VECTOR3 center;
        memcpy(&center, &collisionShape->vertex[0], sizeof(VECTOR3));
        SPHERE3 sphere = {
            .center = Matrix4_multiply_vector3(&matmodel, &center),
            .radius = collisionShape->radius * ent->scale,
        };
        if (Line3_intersect_sphere3(line, &sphere, NULL))
            return true;
    }
    Matrix4_inverse(&matmodel, &invmodel);
    LINE3 linelocal = {
        Matrix4_multiply_vector3(&invmodel, &line->a),
        Matrix4_multiply_vector3(&invmodel, &line->b),
    };
    if (!model->collisionShapes)
        goto check_geosets;
    FOR_EACH_LIST(mdxCollisionShape_t, collisionShape, model->collisionShapes) {
        if (collisionShape->type == SHAPETYPE_BOX) {
            BOX3 box = {
                .min = *(LPCVECTOR3)collisionShape->vertex[0],
                .max = *(LPCVECTOR3)collisionShape->vertex[1],
            };
            if (Line3_intersect_box3(&linelocal, &box, NULL))
                return true;
        } else if (collisionShape->type == SHAPETYPE_SPHERE) {
            VECTOR3 center;
            memcpy(&center, &collisionShape->vertex[0], sizeof(VECTOR3));
            SPHERE3 sphere = {
                .center = center,
                .radius = collisionShape->radius,
            };
            if (Line3_intersect_sphere3(&linelocal, &sphere, NULL))
                return true;
        }
    }
    return false;
check_geosets:
    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        BOX3 box2 = {
            .min = *(LPCVECTOR3)&geoset->default_bounds.min,
            .max = *(LPCVECTOR3)&geoset->default_bounds.max,
        };
        if (Line3_intersect_box3(&linelocal, &box2, NULL))
            goto check_geometry;
    }
    return false;
check_geometry:
    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        FOR_LOOP(i, geoset->num_triangles / 3) {
            TRIANGLE3 tri = {
                .a = *(LPCVECTOR3)geoset->vertices[geoset->triangles[i*3+0]],
                .b = *(LPCVECTOR3)geoset->vertices[geoset->triangles[i*3+1]],
                .c = *(LPCVECTOR3)geoset->vertices[geoset->triangles[i*3+2]],
            };
            if (Line3_intersect_triangle(&linelocal, &tri, NULL))
                return true;
        }
    }
    return false;
}

void R_RenderModel(renderEntity_t const *entity) {
    mdxModel_t const *model = entity->model->mdx;
    if (entity->flags & RF_INVISIBLE)
        return;
    
    if (entity->flags & RF_HAS_LUMBER) {
        renderEntity_t ent = *entity;
        ent.frame = R_RemapAnimation(model, ent.frame, "Lumber");
        ent.oldframe = R_RemapAnimation(model, ent.oldframe, "Lumber");
        entity = &ent;
    } else if (entity->flags & RF_HAS_GOLD) {
        renderEntity_t ent = *entity;
        ent.frame = R_RemapAnimation(model, ent.frame, "Gold");
        ent.oldframe = R_RemapAnimation(model, ent.oldframe, "Gold");
        entity = &ent;
    }

    R_BindBoneMatrices(model, entity->frame, entity->oldframe);

    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        RenderGeoset(model, geoset, entity, entity->skin);
    }

    if (is_rendering_lights)
        return;

    LPCVECTOR2 origin = (LPCVECTOR2)&entity->origin;

    if (entity->flags & RF_SELECTED) {
        COLOR32 color = { 0, 255, 0, 255 };
        float radius = entity->radius;
        if ((radius * 2) < 100) {
            R_RenderSplat(origin, radius, tr.selectionCircleSmall, color);
        } else if ((radius * 2) < 300) {
            R_RenderSplat(origin, radius, tr.selectionCircleMed, color);
        } else {
            R_RenderSplat(origin, radius, tr.selectionCircleLarge, color);
        }
    }
    
    if (entity->splat) {
        COLOR32 color = { 255, 255, 255, 255 };
        R_RenderSplat(origin, entity->splatsize, entity->splat, color);
    }
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

void R_DrawPortrait(LPCMODEL model, LPCRECT viewport) {
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
