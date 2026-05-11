#include "r_mdx.h"
#include "../r_local.h"

extern bool is_rendering_lights;

#define GET_PARTICLE_ANIM_PARAM(MODEL, EMITTER, NAME) \
float NAME = EMITTER->NAME; \
if (EMITTER->keytracks.NAME) { \
    MDLX_GetModelKeytrackValue(MODEL, EMITTER->keytracks.NAME, frame, &NAME); \
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

static LPCTEXTURE MDLX_GetTexture(mdxModel_t const *model,
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

static COLOR32 MDLX_GetEmitterColor(mdxParticleEmitter_t const *emitter, DWORD seg) {
    return (COLOR32) {
        emitter->SegmentColor[seg*3+0] * 0xff,
        emitter->SegmentColor[seg*3+1] * 0xff,
        emitter->SegmentColor[seg*3+2] * 0xff,
        emitter->Alpha[seg],
    };
}

static void MDLX_RenderEmitter(mdxModel_t const *model,
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
    if (emitter->node.node_id >= MDX_MAX_NODES) {
        return;
    }
    LPCMATRIX4 nodeMatrix = &node_matrices[emitter->node.node_id];
    DWORD lastFrameTime = tr.viewDef.time - tr.viewDef.deltaTime;
    DWORD start = lastFrameTime - lastFrameTime % 1000;

    Matrix4_multiply(modelMatrix, nodeMatrix, &matrix);

    for (float time = start, spawning = 0;
         time < tr.viewDef.time;
         time += 1000 / EmissionRate)
    {
        if (time >= lastFrameTime) {
            spawning = 1;
        }
        if (spawning) {
            cparticle_t *p = R_SpawnParticle();
            if (!p) return;
            VECTOR3 origin = FX_GenerateRandomOrigin(emitter->Length, emitter->Width);
            VECTOR3 pivot = { 0, 0, 0 };
            if (emitter->node.node_id < (DWORD)model->num_pivots) {
                pivot = model->pivots[emitter->node.node_id];
            }
            VECTOR3 pivoted = Vector3_add(&origin, &pivot);
            VECTOR3 direction = FX_GenerateRandomDirection(emitter->Latitude * M_PI / 180);
            p->org = Matrix4_multiply_vector3(&matrix, &pivoted);
            p->vel = Vector3_scale(&direction, Speed);
            p->accel = Vector3_scale(&(VECTOR3){0,0,-1}, Gravity);
            p->lifespan = emitter->LifeSpan;
            p->time = 0;
            p->midtime = emitter->Time * 0xff;
            p->texture = MDLX_GetTexture(model, teamID, emitter->TextureID, emitter->ReplaceableId, NULL);
            p->columns = emitter->Columns;
            p->rows = emitter->Rows;
            p->color[0] = MDLX_GetEmitterColor(emitter, 0);
            p->color[1] = MDLX_GetEmitterColor(emitter, 1);
            p->color[2] = MDLX_GetEmitterColor(emitter, 2);
            p->size[0] = emitter->ParticleScaling[0];
            p->size[1] = emitter->ParticleScaling[1];
            p->size[2] = emitter->ParticleScaling[2];
        }
    }
}

static bool MDLX_SetBlendMode(const mdxMaterialLayer_t *layer, DWORD layerID) {
    switch (layer->blendMode) {
        case BLEND_MODE_NONE:
            if (layerID == 0) {
                R_Call(glBlendFunc, GL_ONE, GL_ZERO);
            } else {
                R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            R_Call(glDepthMask, GL_TRUE);
            break;
        case BLEND_MODE_ALPHAKEY:
            R_Call(glUniform1i, mdlx.shader->uUseDiscard, 1);
            R_Call(glBlendFunc, GL_ONE, GL_ZERO);
            R_Call(glDepthMask, GL_TRUE);
            break;
        case BLEND_MODE_BLEND:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            R_Call(glDepthMask, GL_FALSE);
            break;
#ifdef DEBUG_PATHFINDING
        default:
            return false;
#else
        case BLEND_MODE_ADD:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_ONE, GL_ONE);
            R_Call(glDepthMask, GL_FALSE);
            break;
        case BLEND_MODE_MODULATE:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_DST_COLOR, GL_ZERO);
            R_Call(glDepthMask, GL_FALSE);
            break;
        case BLEND_MODE_MODULATE_2X:
            if (is_rendering_lights)
                return false;
            R_Call(glBlendFunc, GL_DST_COLOR, GL_SRC_COLOR);
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

static void MDLX_ApplyLayerFlags(const mdxMaterialLayer_t *layer) {
    if (layer->flags & MODEL_GEO_TWOSIDED) {
        R_Call(glDisable, GL_CULL_FACE);
    }
    if (layer->flags & MODEL_GEO_NO_DEPTH_TEST) {
        R_Call(glDisable, GL_DEPTH_TEST);
    }
    if (layer->flags & MODEL_GEO_NO_DEPTH_SET) {
        R_Call(glDepthMask, GL_FALSE);
    }
}

static mdxMaterial_t *MDLX_GetMaterialAtIndex(mdxGeoset_t const *geoset, mdxModel_t const *model) {
    mdxMaterial_t *material = model->materials;
    for (DWORD materialID = geoset->materialID; materialID > 0; materialID--) {
        material = material->next;
    }
    return material;
}

static void MDLX_RenderGeoset(mdxModel_t const *model,
                             mdxGeoset_t const *geoset,
                             DWORD team,
                             LPCMATRIX4 modelMatrix,
                             LPCTEXTURE overrideTexture)
{
    MATRIX3 mNormalMatrix;
    BOOL force_two_sided = model && !model->cameras;
    LPSHADER shader = mdlx.shader;
    Matrix3_normal(&mNormalMatrix, modelMatrix);
    mdxMaterial_t const *material = MDLX_GetMaterialAtIndex(geoset, model);

    R_Call(glUseProgram, shader->progid);
    R_Call(glUniform1i, shader->uUseDiscard, 0);
    R_Call(glUniformMatrix4fv, shader->uModelMatrix, 1, GL_FALSE, modelMatrix->v);
    R_Call(glUniformMatrix3fv, shader->uNormalMatrix, 1, GL_TRUE, mNormalMatrix.v);

    FOR_LOOP(layerID, material->num_layers) {
        mdxMaterialLayer_t const *layer = &material->layers[layerID];
        R_Call(glEnable, GL_DEPTH_TEST);
        if (force_two_sided) {
            R_Call(glDisable, GL_CULL_FACE);
        } else {
            R_Call(glEnable, GL_CULL_FACE);
            R_Call(glCullFace, GL_BACK);
        }
        R_Call(glDepthMask, GL_TRUE);
        if (!MDLX_SetBlendMode(layer, layerID))
            continue;
        MDLX_ApplyLayerFlags(layer);
        mdxTexture_t const *modeltex = &model->textures[layer->textureId];
        LPCTEXTURE texture = MDLX_GetTexture(model, team, layer->textureId, modeltex->replaceableID, overrideTexture);
        R_BindTexture(texture, 0);
        R_Call(glBindVertexArray, geoset->vertexArrayBuffer);
        R_Call(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, *geoset->buffer);
        R_Call(glDrawElements, GL_TRIANGLES, geoset->num_triangles, GL_UNSIGNED_SHORT, NULL);
    }

    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glEnable, GL_CULL_FACE);
    R_Call(glCullFace, GL_BACK);
    R_Call(glDepthMask, GL_TRUE);
}

mdxSequence_t const *MDLX_FindSequenceByName(mdxModel_t const *model, LPCSTR name) {
    FOR_LOOP(i, model->num_sequences) {
        if (!strcmp(model->sequences[i].name, name)) {
            return &model->sequences[i];
        }
    }
    return NULL;
}

DWORD MDLX_RemapAnimation(mdxModel_t const *model, DWORD frame, LPCSTR str) {
    mdxSequence_t const *seq = R_FindSequenceAtTime(model, frame);
    if (!seq) return frame;
    char buffer[64] = { 0 };
    sprintf(buffer, "%s %s", seq->name, str);
    mdxSequence_t const *other = MDLX_FindSequenceByName(model, buffer);
    if (other) {
        return frame + other->interval[0] - seq->interval[0];
    } else {
        return frame;
    }
}

bool MDLX_TraceModel(renderEntity_t const *ent, LPCLINE3 line) {
    MATRIX4 invmodel, matmodel;
    R_GetEntityMatrix(ent, &matmodel);
    mdxModel_t const *model = ent->model->mdx;
    if (!model) return false;
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
                .min = collisionShape->vertex[0],
                .max = collisionShape->vertex[1],
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
            .min = *(LPCVECTOR3)&geoset->default_bounds.box.min,
            .max = *(LPCVECTOR3)&geoset->default_bounds.box.max,
        };
        if (Line3_intersect_box3(&linelocal, &box2, NULL))
            goto check_geometry;
    }
    return false;
check_geometry:
    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        FOR_LOOP(i, geoset->num_triangles / 3) {
            TRIANGLE3 tri = {
                .a = geoset->vertices[geoset->triangles[i*3+0]],
                .b = geoset->vertices[geoset->triangles[i*3+1]],
                .c = geoset->vertices[geoset->triangles[i*3+2]],
            };
            if (Line3_intersect_triangle(&linelocal, &tri, NULL))
                return true;
        }
    }
    return false;
}

static void MDLX_RenderGeosets(const renderEntity_t *entity, const mdxModel_t *model, LPCMATRIX4 model_matrix) {
    FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
        if (geoset->geosetAnim && geoset->geosetAnim->alphas) {
            float fAlpha = 1.f;
            MDLX_GetModelKeytrackValue(model, geoset->geosetAnim->alphas, entity->frame, &fAlpha);
            if (fAlpha < EPSILON)
                continue;
        }
        MDLX_RenderGeoset(model, geoset, entity->team&TEAM_MASK, model_matrix, entity->skin);
    }
}

static void MDLX_RenderParticleEmitters(const renderEntity_t *entity, const mdxModel_t *model, LPCMATRIX4 model_matrix) {
    FOR_EACH_LIST(mdxParticleEmitter_t, emitter, model->emitters) {
        if (emitter->keytracks.Visibility) {
            float fVisibility = 1.f;
            MDLX_GetModelKeytrackValue(model, emitter->keytracks.Visibility, entity->frame, &fVisibility);
            if (fVisibility < EPSILON)
                continue;
        }
        float const frame = LerpNumber(entity->oldframe, entity->frame, tr.viewDef.lerpfrac);
        MDLX_RenderEmitter(model, emitter, model_matrix, frame, entity->team&TEAM_MASK);
    }
}

void MDX_RenderModel(renderEntity_t const *entity,
                     mdxModel_t const *model,
                     LPCMATRIX4 transform)
{
    if (!(tr.viewDef.rdflags & RDF_NOFRUSTUMCULL)) {
        VECTOR3 const center = Box3_Center(&model->bounds.box);
        SPHERE3 const sphere = {
            .center = Matrix4_multiply_vector3(transform, &center),
            .radius = model->bounds.radius * entity->scale,
        };
        if (!Frustum_ContainsSphere(&tr.viewDef.frustum, &sphere))
            return;
        if (!Frustum_ContainsBox(&tr.viewDef.frustum, &model->bounds.box, transform))
            return;
    }

    if (entity->flags & RF_HAS_LUMBER) {
        renderEntity_t ent = *entity;
        ent.frame = MDLX_RemapAnimation(model, ent.frame, "Lumber");
        ent.oldframe = MDLX_RemapAnimation(model, ent.oldframe, "Lumber");
        entity = &ent;
    } else if (entity->flags & RF_HAS_GOLD) {
        renderEntity_t ent = *entity;
        ent.frame = MDLX_RemapAnimation(model, ent.frame, "Gold");
        ent.oldframe = MDLX_RemapAnimation(model, ent.oldframe, "Gold");
        entity = &ent;
    }
    
    LPSHADER shader = mdlx.shader;
    R_Call(glUseProgram, shader->progid);
    R_Call(glUniformMatrix4fv, shader->uViewProjectionMatrix, 1, GL_FALSE, is_rendering_lights ? tr.viewDef.lightMatrix.v : tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, shader->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniformMatrix4fv, shader->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);

    MDLX_BindBoneMatrices(model, transform, entity->frame, entity->oldframe);

    if (entity->flags & RF_NO_FOGOFWAR) {
        R_Call(glActiveTexture, GL_TEXTURE2);
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.texture[TEX_WHITE]->texid);
        R_Call(glActiveTexture, GL_TEXTURE0);
    }
    
    MDLX_RenderGeosets(entity, model, transform);
    
    MDLX_RenderParticleEmitters(entity, model, transform);

    if ((entity->flags & RF_NO_FOGOFWAR) && tr.world) {
        R_Call(glActiveTexture, GL_TEXTURE2);
        R_Call(glBindTexture, GL_TEXTURE_2D, R_GetFogOfWarTexture());
        R_Call(glActiveTexture, GL_TEXTURE0);
    }
}

