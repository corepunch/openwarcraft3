#include "r_mdx.h"
#include "r_local.h"

DWORD selCircles[NUM_SELECTION_CIRCLES] = { 100, 300, 100000 };

static MATRIX4 local_matrices[MAX_NODES];
static MATRIX4 global_matrices[MAX_NODES];

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

static void MDX_GetModelKeytrackValue(mdxModel_t const *model, mdxKeyTrack_t const *keytrack, DWORD time, HANDLE output) {
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
    LPCVECTOR3 pivot = (VECTOR3 const *)&model->pivots[node->node_id];
    if (frame0 != frame1) {
        if (node->translation) {
            VECTOR3 t0 = vTranslation, t1 = vTranslation;
            MDX_GetModelKeytrackValue(model, node->translation, frame0, &t0);
            MDX_GetModelKeytrackValue(model, node->translation, frame1, &t1);
            vTranslation = Vector3_lerp(&t0, &t1, tr.viewDef.lerpfrac);
        }
        if (node->rotation) {
            QUATERNION r0 = vRotation, r1 = vRotation;
            MDX_GetModelKeytrackValue(model, node->rotation, frame0, &r0);
            MDX_GetModelKeytrackValue(model, node->rotation, frame1, &r1);
            vRotation = Quaternion_slerp(&r0, &r1, tr.viewDef.lerpfrac);
        }
        if (node->scale) {
            VECTOR3 s0 = vScale, s1 = vScale;
            MDX_GetModelKeytrackValue(model, node->scale, frame0, &s0);
            MDX_GetModelKeytrackValue(model, node->scale, frame1, &s1);
            vScale = Vector3_lerp(&s0, &s1, tr.viewDef.lerpfrac);
        }
    } else {
        if (node->translation) {
            MDX_GetModelKeytrackValue(model, node->translation, frame1, &vTranslation);
        }
        if (node->rotation) {
            MDX_GetModelKeytrackValue(model, node->rotation, frame1, &vRotation);
        }
        if (node->scale) {
            MDX_GetModelKeytrackValue(model, node->scale, frame1, &vScale);
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

LPCMATRIX4 R_GetNodeGlobalMatrix(mdxModel_t const *model, LPCMATRIX4 model_matrix, mdxNode_t const *node) {
    LPMATRIX4 global_matrix = global_matrices+node->node_id;
    LPMATRIX4 local_matrix = local_matrices+node->node_id;
    if (global_matrix->v[15] == 0) {
        if (node->parent_id != -1) {
            LPCMATRIX4 parent_matrix = R_GetNodeGlobalMatrix(model, model_matrix, model->nodes[node->parent_id]);
            Matrix4_multiply(parent_matrix, local_matrix, global_matrix);
        } else {
            *global_matrix = *local_matrix;
        }
        if (node->flags & MDXNODE_Billboarded) {
            MATRIX4 tmp1, tmp2;
            VECTOR3 tmppvt = Matrix4_multiply_vector3(global_matrix, (LPCVECTOR3)(&model->pivots[node->node_id]));
            if (node->parent_id != -1) {
                LPCMATRIX4 parent_matrix = R_GetNodeGlobalMatrix(model, model_matrix, model->nodes[node->parent_id]);
                QUATERNION tmprot = Quaternion_fromMatrix(parent_matrix);
                tmprot = Quaternion_unm(&tmprot);
                Matrix4_from_rotation_origin(&tmp1, &tmprot, &tmppvt);
                Matrix4_multiply(&tmp1, global_matrix, &tmp2);
                *global_matrix = tmp2;
            }
            
            Matrix4_identity(&tmp1);
            Matrix4_rotate(&tmp1, &(VECTOR3){30,0,90}, ROTATE_XYZ);
            Matrix4_multiply(&tmp1, model_matrix, &tmp2);

            QUATERNION viewrot = Quaternion_fromMatrix(&tmp2);
            viewrot = Quaternion_unm(&viewrot);
            Matrix4_from_rotation_origin(&tmp1, &viewrot, &tmppvt);
            Matrix4_multiply(&tmp1, global_matrix, &tmp2);
            *global_matrix = tmp2;
        }
    }
    return global_matrix;
}

static void MDX_BindBoneMatrices(mdxModel_t const *model, LPCMATRIX4 model_matrix, DWORD frame1, DWORD frame0) {
    DWORD numBones = 1;
    memset(global_matrices, 0, sizeof(global_matrices));
    
    for (mdxNode_t *const *node = model->nodes; *node; node++) {
        R_CalculateNodeMatrix(model, *node, frame1, frame0, &local_matrices[(*node)->node_id]);
    }
    for (mdxNode_t *const *node = model->nodes; *node; node++) {
        R_GetNodeGlobalMatrix(model, model_matrix, *node);
    }

    FOR_EACH_LIST(mdxBone_t, bone, model->bones) {
        numBones++;
    }

    R_Call(glUseProgram, tr.shader[SHADER_SKIN]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_SKIN]->uBones, numBones, GL_FALSE, global_matrices->v);
}

extern bool is_rendering_lights;

#define GET_PARTICLE_ANIM_PARAM(MODEL, EMITTER, NAME) \
float NAME = EMITTER->NAME; \
if (EMITTER->keytracks.NAME) { \
    MDX_GetModelKeytrackValue(MODEL, EMITTER->keytracks.NAME, frame, &NAME); \
}

static VECTOR3 FX_GenerateRandomDirection(float latitude) {
    float theta = (float)rand() / RAND_MAX * 2 * M_PI;
    float phi = (float)rand() / RAND_MAX * latitude;
    VECTOR3 direction = {
        .x = sin(phi) * cos(theta),
        .y = sin(phi) * sin(theta),
        .z = cos(phi),
    };
    return direction;
}

static VECTOR3 FX_GenerateRandomOrigin(float length, float width) {
    VECTOR3 origin = {
        .x = ((float)rand() / RAND_MAX - 0.5) * length,
        .y = ((float)rand() / RAND_MAX - 0.5) * width,
        .z = 0,
    };
    return origin;
}

static LPCTEXTURE MDX_GetTexture(mdxModel_t const *model,
                                 DWORD teamID,
                                 DWORD textureID,
                                 DWORD replaceableID,
                                 LPCTEXTURE overrideTexture) {
    mdxTexture_t const *modeltex = &model->textures[textureID];
    switch (replaceableID) {
        case TEXREPL_TEAMCOLOR: return tr.texture[TEX_TEAM_COLOR + teamID];
        case TEXREPL_TEAMGLOW: return tr.texture[TEX_TEAM_GLOW + teamID];
        case TEXREPL_NONE: return R_FindTextureByID(modeltex->texid);
        case TEXREPL_TEXTURE: return overrideTexture;
        default:
//                printf("%d\n", modeltex->replaceableID);
            return NULL;
    }
}

static COLOR32 MDX_GetEmitterColor(mdxParticleEmitter_t const *emitter, DWORD seg) {
    return (COLOR32) {
        emitter->SegmentColor[seg*3+0] * 0xff,
        emitter->SegmentColor[seg*3+1] * 0xff,
        emitter->SegmentColor[seg*3+2] * 0xff,
        emitter->Alpha[seg],
    };
}

static void MDX_RenderEmitter(mdxModel_t const *model,
                              mdxParticleEmitter_t const *emitter,
                              LPCMATRIX4 modelMatrix,
                              float frame,
                              DWORD teamID)
{
    GET_PARTICLE_ANIM_PARAM(model, emitter, EmissionRate);
    GET_PARTICLE_ANIM_PARAM(model, emitter, Width);
    GET_PARTICLE_ANIM_PARAM(model, emitter, Length);
    GET_PARTICLE_ANIM_PARAM(model, emitter, Speed);
    GET_PARTICLE_ANIM_PARAM(model, emitter, Latitude);
    GET_PARTICLE_ANIM_PARAM(model, emitter, Gravity);
    GET_PARTICLE_ANIM_PARAM(model, emitter, Variation);
    
    if (EmissionRate <= 0)
        return;
    
    EmissionRate = MAX(1, EmissionRate);
    MATRIX4 matrix;
    LPCMATRIX4 nodeMatrix = &global_matrices[emitter->node.node_id];
    DWORD lastFrameTime = tr.viewDef.time - tr.viewDef.deltaTime;
    DWORD start = lastFrameTime - lastFrameTime % MSEC;

    Matrix4_multiply(modelMatrix, nodeMatrix, &matrix);

    for (float time = start, spawning = 0;
         time < tr.viewDef.time;
         time += MSEC / EmissionRate)
    {
        if (time >= lastFrameTime) {
            spawning = 1;
        }
        if (spawning) {
            cparticle_t *p = R_SpawnParticle();
            if (!p) return;
            VECTOR3 origin = FX_GenerateRandomOrigin(emitter->Length, emitter->Width);
            VECTOR3 pivoted = Vector3_add(&origin, (LPCVECTOR3)model->pivots[emitter->node.node_id]);
            VECTOR3 direction = FX_GenerateRandomDirection(emitter->Latitude * M_PI / 180);
            p->org = Matrix4_multiply_vector3(&matrix, &pivoted);
            p->vel = Vector3_scale(&direction, Speed);
            p->accel = Vector3_scale(&(VECTOR3){0,0,-1}, Gravity);
            p->lifespan = emitter->LifeSpan;
            p->time = 0;
            p->midtime = emitter->Time * 0xff;
            p->texture = MDX_GetTexture(model, teamID, emitter->TextureID, emitter->ReplaceableId, NULL);
            p->columns = emitter->Columns;
            p->rows = emitter->Rows;
            p->color[0] = MDX_GetEmitterColor(emitter, 0);
            p->color[1] = MDX_GetEmitterColor(emitter, 1);
            p->color[2] = MDX_GetEmitterColor(emitter, 2);
            p->size[0] = emitter->ParticleScaling[0];
            p->size[1] = emitter->ParticleScaling[1];
            p->size[2] = emitter->ParticleScaling[2];
        }
    }
}

static bool MDX_SetBlendMode(const mdxMaterialLayer_t *layer, DWORD layerID) {
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
            R_Call(glUniform1i, tr.shader[SHADER_SKIN]->uUseDiscard, 1);
            R_Call(glBlendFunc, GL_ONE, GL_ZERO);
            R_Call(glDepthMask, GL_TRUE);
            break;
        case TEXOP_BLEND:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            R_Call(glDepthMask, GL_FALSE);
            break;
#ifdef DEBUG_PATHFINDING
        default:
            return false;
#else
        case TEXOP_ADD:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_ONE, GL_ONE);
            R_Call(glDepthMask, GL_FALSE);
            break;
        case TEXOP_ADD_ALPHA:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE);
            R_Call(glDepthMask, GL_FALSE);
            break;
        default:
            R_Call(glBlendFunc, GL_ONE, GL_ZERO);
            R_Call(glDepthMask, GL_TRUE);
            break;
#endif
    }
    return true;
}

static mdxMaterial_t *MDX_GetMaterialAtIndex(mdxGeoset_t const *geoset, mdxModel_t const *model) {
    mdxMaterial_t *material = model->materials;
    for (DWORD materialID = geoset->materialID; materialID > 0; materialID--) {
        material = material->next;
    }
    return material;
}

static void MDX_RenderGeoset(mdxModel_t const *model,
                             mdxGeoset_t const *geoset,
                             DWORD team,
                             LPCMATRIX4 modelMatrix,
                             LPCTEXTURE overrideTexture)
{
    struct render_buffer *buf = geoset->userdata;
    if (!buf)
        return;

    MATRIX3 mNormalMatrix;
    Matrix3_normal(&mNormalMatrix, modelMatrix);
    mdxMaterial_t const *material = MDX_GetMaterialAtIndex(geoset, model);

    R_Call(glUseProgram, tr.shader[SHADER_SKIN]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_SKIN]->uModelMatrix, 1, GL_FALSE, modelMatrix->v);
    R_Call(glUniformMatrix3fv, tr.shader[SHADER_SKIN]->uNormalMatrix, 1, GL_TRUE, mNormalMatrix.v);
    R_Call(glUniform1i, tr.shader[SHADER_SKIN]->uUseDiscard, 0);

    FOR_LOOP(layerID, material->num_layers) {
        mdxMaterialLayer_t const *layer = &material->layers[layerID];
        if (!MDX_SetBlendMode(layer, layerID))
            continue;
        mdxTexture_t const *modeltex = &model->textures[layer->textureId];
        LPCTEXTURE texture = MDX_GetTexture(model, team, layer->textureId, modeltex->replaceableID, overrideTexture);
        R_BindTexture(texture, 0);
        R_Call(glBindVertexArray, buf->vao);
        R_Call(glBindBuffer, GL_ARRAY_BUFFER, buf->vbo);
        R_Call(glDrawArrays, GL_TRIANGLES, 0, geoset->num_triangles);
    }
}

mdxSequence_t const *MDX_FindSequenceByName(mdxModel_t const *model, LPCSTR name) {
    FOR_LOOP(i, model->num_sequences) {
        if (!strcmp(model->sequences[i].name, name)) {
            return &model->sequences[i];
        }
    }
    return NULL;
}

DWORD MDX_RemapAnimation(mdxModel_t const *model, DWORD frame, LPCSTR str) {
    mdxSequence_t const *seq = R_FindSequenceAtTime(model, frame);
    if (!seq) return frame;
    char buffer[64] = { 0 };
    sprintf(buffer, "%s %s", seq->name, str);
    mdxSequence_t const *other = MDX_FindSequenceByName(model, buffer);
    if (other) {
        return frame + other->interval[0] - seq->interval[0];
    } else {
        return frame;
    }
}

bool MDX_TraceModel(renderEntity_t const *ent, LPCLINE3 line) {
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

static void MDX_RenderGeosets(const renderEntity_t *entity, const mdxModel_t *model, MATRIX4 *model_matrix) {
    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        if (geoset->geosetAnim && geoset->geosetAnim->alphas) {
            float fAlpha = 1.f;
            MDX_GetModelKeytrackValue(model, geoset->geosetAnim->alphas, entity->frame, &fAlpha);
            if (fAlpha < EPSILON)
                continue;
        }
        MDX_RenderGeoset(model, geoset, entity->team&TEAM_MASK, model_matrix, entity->skin);
    }
}

static void MDX_RenderParticleEmitters(const renderEntity_t *entity, const mdxModel_t *model, MATRIX4 *model_matrix) {
    FOR_EACH_LIST(mdxParticleEmitter_t, emitter, model->emitters) {
        if (emitter->keytracks.Visibility) {
            float fVisibility = 1.f;
            MDX_GetModelKeytrackValue(model, emitter->keytracks.Visibility, entity->frame, &fVisibility);
            if (fVisibility < EPSILON)
                continue;
        }
        float const frame = LerpNumber(entity->oldframe, entity->frame, tr.viewDef.lerpfrac);
        MDX_RenderEmitter(model, emitter, model_matrix, frame, entity->team&TEAM_MASK);
    }
}

static void MDX_RenderUberSplat(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (entity->splat && !(entity->flags & RF_NO_UBERSPLAT)) {
        COLOR32 color = { 255, 255, 255, 255 };
        R_RenderSplat(origin, entity->splatsize, entity->splat, tr.shader[SHADER_DEFAULT], color);
    }
}

static void MDX_RenderSelectedCircle(const renderEntity_t *entity, LPCVECTOR2 origin) {
    if (entity->flags & RF_SELECTED) {
        COLOR32 color = { 0, 255, 0, 255 };
        float radius = entity->radius;
        FOR_LOOP(i, NUM_SELECTION_CIRCLES) {
            if ((radius * 2) > selCircles[i])
                continue;
            R_RenderSplat(origin, radius, tr.texture[TEX_SELECTION_CIRCLE+i], tr.shader[SHADER_UI], color);
            break;
        }
    }
}

void R_RenderModel(renderEntity_t const *entity) {
    MATRIX4 model_matrix;
    mdxModel_t const *model = entity->model->mdx;
    LPCVECTOR2 origin = (LPCVECTOR2)&entity->origin;
    
    if (entity->flags & RF_HIDDEN)
        return;
    
    if (entity->flags & RF_HAS_LUMBER) {
        renderEntity_t ent = *entity;
        ent.frame = MDX_RemapAnimation(model, ent.frame, "Lumber");
        ent.oldframe = MDX_RemapAnimation(model, ent.oldframe, "Lumber");
        entity = &ent;
    } else if (entity->flags & RF_HAS_GOLD) {
        renderEntity_t ent = *entity;
        ent.frame = MDX_RemapAnimation(model, ent.frame, "Gold");
        ent.oldframe = MDX_RemapAnimation(model, ent.oldframe, "Gold");
        entity = &ent;
    }

    R_GetEntityMatrix(entity, &model_matrix);
    
    MDX_BindBoneMatrices(model, &model_matrix, entity->frame, entity->oldframe);

    if (entity->flags & RF_NO_FOGOFWAR) {
        R_Call(glActiveTexture, GL_TEXTURE2);
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.texture[TEX_WHITE]->texid);
        R_Call(glActiveTexture, GL_TEXTURE0);
    }
    
    MDX_RenderGeosets(entity, model, &model_matrix);
    
    MDX_RenderParticleEmitters(entity, model, &model_matrix);
    
    if ((entity->flags & RF_NO_FOGOFWAR) && tr.world) {
        R_Call(glActiveTexture, GL_TEXTURE2);
        R_Call(glBindTexture, GL_TEXTURE_2D, R_GetFogOfWarTexture());
        R_Call(glActiveTexture, GL_TEXTURE0);
    }

    if (is_rendering_lights)
        return;
    
    MDX_RenderUberSplat(entity, origin);
    
    MDX_RenderSelectedCircle(entity, origin);
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
    
    R_Call(glActiveTexture, GL_TEXTURE2);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.texture[TEX_WHITE]->texid);
    R_Call(glActiveTexture, GL_TEXTURE0);
    
    R_GetModelCameraMatrix(mdx, &viewdef.viewProjectionMatrix, &root);
    
    Matrix4_getLightMatrix(&lightAngles, &root, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);

    tr.viewDef = viewdef;

    R_RenderShadowMap();
    
    R_RenderView();
}
