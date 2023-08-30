#include "../r_local.h"
#include "r_m3.h"

#define M3_FOR_EACH(TYPE, VAR, LIST) \
for (m3##TYPE##_t const *VAR = LIST; VAR < LIST + LIST##Num; VAR++)

#define M3_READ(BUFFER, VAR, VERSION) \
if ((BUFFER)->ent.version > VERSION || VERSION == 0) M3_Read(BUFFER, &VAR, sizeof(VAR));

#define READ_REFERENCE(TARGET, REF, TYPE) { \
    m3Reader_t reader##__LINE__ = M3_MakeSizeBuf(currentmodel, REF); \
    TARGET##Num = REF.nEntries; \
    TARGET = ri.MemAlloc(sizeof(m3##TYPE##_t) * REF.nEntries); \
    FOR_LOOP(n, REF.nEntries) { \
        M3_Read##TYPE(currentmodel, &reader##__LINE__, &TARGET[n]); \
    } \
}

#define M3_REFR(BUFFER, TARGET, TYPE, VERSION) \
if ((BUFFER)->ent.version > VERSION) { \
    Reference ref; \
    M3_Read(BUFFER, &ref, sizeof(ref)); \
    READ_REFERENCE(TARGET, ref, TYPE); \
}

#define M3_READER(TYPE) \
void M3_Read##TYPE(m3Model_t *model, m3Reader_t *sb, m3##TYPE##_t *data)

#define M3_SEQUENCE_DATA(TYPE) \
M3_READER(TYPE##SequenceData) { \
    M3_REFR(sb, data->keys, Uint32, 0); \
    M3_READ(sb, data->flags, 0); \
    M3_READ(sb, data->biggestKey, 0); \
    M3_REFR(sb, data->values, TYPE, 0); \
}

static LPCSTR vs =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"in vec3 i_normal;\n"
"in vec4 i_skin1;\n"
"in vec4 i_boneWeight1;\n"
"out vec4 v_color;\n"
"out vec4 v_shadow;\n"
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"out vec4 v_fragCoord;\n"
"uniform mat4 uBones[64];\n"
"uniform mat4 uViewProjectionMatrix;\n"
"uniform mat4 uTextureMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"uniform float uFirstBoneLookupIndex;\n"
"uniform float uBoneWeightPairsCount;\n"
"vec3 transform(vec4 bones, vec4 position) {\n"
"    vec4 value = vec4(0);\n"
"    for (int i = 0; i < 4; i++) {\n"
"        value += uBones[int(bones[i])] * position * i_boneWeight1[i];\n"
"    }\n"
"    return value.xyz;\n"
"}\n"
"void main() {\n"
"    vec4 bones = i_skin1 + vec4(uFirstBoneLookupIndex);\n"
"    vec4 position = vec4(transform(bones, vec4(i_position, 1.0)), 1.0);\n"
"    vec3 normal = transform(bones, vec4(i_normal * 2.0 - 1.0, 0.0));\n"
"    v_color = i_color;\n"
"    v_texcoord = i_texcoord / 2048.0;\n"
"    v_texcoord2 = (uTextureMatrix * uModelMatrix * position).xy;\n"
"    v_normal = normalize(uNormalMatrix * normal);\n"
"    v_shadow = uLightMatrix * uModelMatrix * position;\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[0][2], uLightMatrix[1][2], uLightMatrix[2][2]))*1.2;\n"
"    v_fragCoord = uViewProjectionMatrix * uModelMatrix * position;\n"
"    gl_Position = v_fragCoord;\n"
"}\n";


static LPCSTR fs =
"#version 140\n"
"in vec2 v_texcoord;\n"
"in vec2 v_texcoord2;\n"
"in vec4 v_shadow;\n"
"in vec3 v_normal;\n"
"in vec3 v_lightDir;\n"
//"in vec4 v_fragCoord;\n"
"out vec4 o_color;\n"

"uniform sampler2D uDiffuseMap;\n"
"uniform sampler2D uSpecularMap;\n"
"uniform sampler2D uNormalMap;\n"

"uniform sampler2D uTexture;\n"
"uniform sampler2D uShadowmap;\n"
"uniform sampler2D uFogOfWar;\n"

"vec3 calculateSpecular(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 specularColor) {\n"
"    vec3 reflectDir = reflect(-lightDir, normal);\n"
"    float specularFactor = pow(max(0.0, dot(viewDir, reflectDir)), specularColor.r);\n"
"    return specularColor * specularFactor;\n"
"}\n"

"vec3 applyBumpMap(vec3 color, vec4 bumpColor) {\n"
"    return color * bumpColor.rgb;\n"
"}\n"

"float get_light() {\n"
"    return dot(v_normal, v_lightDir);\n"
"}\n"

"float get_shadow() {\n"
"    float depth = texture(uShadowmap, vec2(v_shadow.x + 1.0, v_shadow.y + 1.0) * 0.5).r;\n"
"    return depth < (v_shadow.z + 0.99) * 0.5 ? 0.0 : 1.0;\n"
"}\n"

"float get_lighting() {\n"
"    return mix(0.35, 1.0, get_shadow() * get_light()) * 1.5;"
"}\n"

"float get_fogofwar() {\n"
"    return texture(uFogOfWar, v_texcoord2).r;\n"
"}\n"

"vec3 decodeNormal(sampler2D map, vec2 texcoord) {\n"
"    vec4 texel = texture(map, texcoord);\n"
"    vec3 normal = vec3(2.0 * texel.wy - 1.0, 0.0);\n"
"    normal.z = sqrt(max(0.0, 1.0 - dot(normal.xy, normal.xy)));\n"
"    return normal;\n"
"}\n"

"void main() {\n"
"    vec4 diffuseColor = texture(uDiffuseMap, v_texcoord);\n"
"    vec4 specularColor = texture(uSpecularMap, v_texcoord);\n"
//"    vec3 normal = decodeNormal(uNormalMap, v_texcoord);\n"
//"    vec4 col = texture(uTexture, v_texcoord);\n"
//"    col.rgb *= get_fogofwar() * get_lighting();\n"
//"    o_color = col;\n"
"    vec3 lightDir = normalize(-v_lightDir);\n"
"    vec3 diffuseLight = diffuseColor.rgb;\n"
//"    vec3 viewDir = normalize(-vec3(v_fragCoord.xy/v_fragCoord.w, 1.0));\n"
//"    vec3 specularLight = calculateSpecular(normal, viewDir, lightDir, specularColor.rgb);\n"
//"    vec3 ambientColor = vec3(0.1);  // Add ambient light\n"
"    vec3 finalColor = vec3(0);// ambientColor + diffuseLight + specularLight;\n"
"    diffuseLight.rgb *= get_fogofwar() * get_lighting();\n"
"    finalColor = diffuseLight;// applyBumpMap(finalColor, bumpColor);\n"
"    o_color = vec4(finalColor, 1.0);\n"
"}\n";


static MATRIX4 bonemats[MAX_NODES];
static MATRIX4 tmp[MAX_NODES];

m3Model_t *currentmodel;

static struct {
    LPSHADER shader;
    DWORD uFirstBoneLookupIndex;
    DWORD uBoneWeightPairsCount;
    DWORD uDiffuseMap;
    DWORD uSpecularMap;
    DWORD uNormalMap;
} m3 = { 0 };

typedef struct {
    struct ReferenceEntry ent;
    DWORD readcount;
    void *data;
} m3Reader_t;

void
R_EvalKeyframeValue(void const *left,
                    void const *right,
                    float t,
                    MODELKEYTRACKDATATYPE datatype,
                    MODELKEYTRACKTYPE linetype,
                    HANDLE out);

void M3_Read(m3Reader_t *buffer, void *dest, DWORD bytes) {
    memcpy(dest, buffer->data + buffer->readcount, bytes);
    buffer->readcount += bytes;
}

void PrintRef(m3Model_t const *model, Reference ref) {
    printf("%.4s\n", model->refs[ref.ref].id);
}

m3Reader_t M3_MakeSizeBuf(m3Model_t const *model, Reference ref) {
    return (m3Reader_t) {
        .data = model->buffer + model->refs[ref.ref].offset,
        .readcount = 0,
        .ent = model->refs[ref.ref],
    };
}

enum {
    kMaterialStandard,
    kMaterialDisplacement,
    kMaterialComposite,
    kMaterialTerrain,
    kMaterialVolume,
    kMaterialUnknown1,
    kMaterialCreep,
    kMaterialVolumeNoise,
    kMaterialSplatTerrainBake,
    kMaterialUnknown2,
    kMaterialLensFlare,
};

M3_READER(Int16) { M3_Read(sb, data, sizeof(m3Int16_t)); }
M3_READER(Uint16) { M3_Read(sb, data, sizeof(m3Uint16_t)); }
M3_READER(Int32) { M3_Read(sb, data, sizeof(m3Int32_t)); }
M3_READER(Uint32) { M3_Read(sb, data, sizeof(m3Uint32_t)); }
M3_READER(Float32) { M3_Read(sb, data, sizeof(m3Float32_t)); }
M3_READER(Vector2) { M3_Read(sb, data, sizeof(m3Vector2_t)); }
M3_READER(Vector3) { M3_Read(sb, data, sizeof(m3Vector3_t)); }
M3_READER(Vector4) { M3_Read(sb, data, sizeof(m3Vector4_t)); }
M3_READER(Matrix4) { M3_Read(sb, data, sizeof(m3Matrix4_t)); }
M3_READER(Face) { M3_Read(sb, data, sizeof(m3Face_t)); }
M3_READER(Pixel) { M3_Read(sb, data, sizeof(m3Pixel_t)); }

M3_READER(Char) {
    M3_Read(sb, data, sizeof(m3Char_t));
    if (*data == '/')
        *data = '\\';
}
M3_READER(Vertex) {
    M3_READ(sb, data->pos, 0);
    M3_READ(sb, data->boneWeight, 0);
    M3_READ(sb, data->boneIndex, 0);
    M3_READ(sb, data->normal, 0);
    M3_READ(sb, data->uv, 0);
    if ((currentmodel->vertexFlags & 0x40000) != 0) {
        M3_READ(sb, data->uv2, 0);
    }
    M3_READ(sb, data->tangent, 0);
}
M3_READER(MaterialReference) {
    M3_READ(sb, data->materialType, 0);
    M3_READ(sb, data->materialIndex, 0);
}

M3_READER(Layer) {
    M3_READ(sb, data->unknown0, 0);
    M3_REFR(sb, data->imagePath, Char, 0);
    M3_READ(sb, data->color, 0);
    M3_READ(sb, data->flags, 0);
    M3_READ(sb, data->uvSource1, 0);
    M3_READ(sb, data->colorChannelSetting, 0);
    M3_READ(sb, data->brightMult, 0);
    M3_READ(sb, data->midtoneOffset, 0);
    M3_READ(sb, data->unknown1, 0);
    M3_READ(sb, data->noise, 23);
    M3_READ(sb, data->rttChannel, 0);
    M3_READ(sb, data->video, 0);
    M3_READ(sb, data->flipBook, 0);
    M3_READ(sb, data->uv, 0);
    M3_READ(sb, data->brightness, 0);
    M3_READ(sb, data->triPlanarOffset, 23);
    M3_READ(sb, data->triPlanarScale, 23);
    M3_READ(sb, data->unknown4, 0);
    M3_READ(sb, data->fresnel, 0);
    M3_READ(sb, data->fresnel2, 24);
    
    if (*data->imagePath) {
        data->texture = R_LoadTexture(data->imagePath);
    }
}

M3_READER(Material) {
    M3_REFR(sb, data->name, Char, 0);
    M3_READ(sb, data->additionalFlags, 0);
    M3_READ(sb, data->flags, 0);
    M3_READ(sb, data->blendMode, 0);
    M3_READ(sb, data->priority, 0);
    M3_READ(sb, data->usedRTTChannels, 0);
    M3_READ(sb, data->specularity, 0);
    M3_READ(sb, data->depthBlendFalloff, 0);
    M3_READ(sb, data->cutoutThreshold, 0);
    M3_READ(sb, data->specMult, 0);
    M3_READ(sb, data->emisMult, 0);
    M3_REFR(sb, data->diffuseLayer, Layer, 0);
    M3_REFR(sb, data->decalLayer, Layer, 0);
    M3_REFR(sb, data->specularLayer, Layer, 0);
    M3_REFR(sb, data->glossLayer, Layer, 15);
    M3_REFR(sb, data->emissiveLayer, Layer, 0);
    M3_REFR(sb, data->emissive2Layer, Layer, 0);
    M3_REFR(sb, data->evioLayer, Layer, 0);
    M3_REFR(sb, data->evioMaskLayer, Layer, 0);
    M3_REFR(sb, data->alphaMaskLayer, Layer, 0);
    M3_REFR(sb, data->alphaMask2Layer, Layer, 0);
    M3_REFR(sb, data->normalLayer, Layer, 0);
    M3_REFR(sb, data->heightLayer, Layer, 0);
    M3_REFR(sb, data->lightMapLayer, Layer, 0);
    M3_REFR(sb, data->ambientOcclusionLayer, Layer, 0);
    M3_READ(sb, data->unknown4, 18);
    M3_READ(sb, data->unknown8, 0);
    M3_READ(sb, data->layerBlendType, 0);
    M3_READ(sb, data->emisBlendType, 0);
    M3_READ(sb, data->emisMode, 0);
    M3_READ(sb, data->specType, 0);
    M3_READ(sb, data->unknown9, 0);
    M3_READ(sb, data->unknown10, 0);
    M3_READ(sb, data->unknown11, 18);
}

M3_READER(Region) {
    M3_READ(sb, data->unknown0, 0);
    M3_READ(sb, data->unknown1, 0);
    M3_READ(sb, data->firstVertexIndex, 0);
    M3_READ(sb, data->verticesCount, 0);
    M3_READ(sb, data->firstTriangleIndex, 0);
    M3_READ(sb, data->triangleIndicesCount, 0);
    M3_READ(sb, data->bonesCount, 0);
    M3_READ(sb, data->firstBoneLookupIndex, 0);
    M3_READ(sb, data->boneLookupIndicesCount, 0);
    M3_READ(sb, data->unknown2, 0);
    M3_READ(sb, data->boneWeightPairsCount, 0);
    M3_READ(sb, data->unknown3, 0);
    M3_READ(sb, data->rootBoneIndex, 0);
    M3_READ(sb, data->unknown4, 3);
    M3_READ(sb, data->unknown5, 4);
}

M3_READER(Batch) {
    M3_READ(sb, data->unknown0, 0);
    M3_READ(sb, data->regionIndex, 0);
    M3_READ(sb, data->unknown1, 0);
    M3_READ(sb, data->materialReferenceIndex, 0);
    M3_READ(sb, data->unknown2, 0);
}

M3_READER(Divisions) {
    M3_REFR(sb, data->faces, Face, 0);
    M3_REFR(sb, data->regions, Region, 0);
    M3_REFR(sb, data->batches, Batch, 0);
    M3_READ(sb, data->MSEC, 0);
    
    R_Call(glGenBuffers, 1, &data->indicesBuffer);
    R_Call(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, data->indicesBuffer);
    R_Call(glBufferData, GL_ELEMENT_ARRAY_BUFFER, data->facesNum * sizeof(USHORT), data->faces, GL_STATIC_DRAW);
}

M3_READER(Sequence) {
    M3_READ(sb, data->unknown, 0);
    M3_REFR(sb, data->name, Char, 0);
    M3_READ(sb, data->interval, 0);
    M3_READ(sb, data->movementSpeed, 0);
    M3_READ(sb, data->flags, 0);
    M3_READ(sb, data->frequency, 0);
    M3_READ(sb, data->unk, 0);
    if (sb->ent.version < 2)
        M3_READ(sb, data->unk2, 0);
    M3_READ(sb, data->boundingSphere, 0);
    M3_READ(sb, data->d5, 0);
//    printf("  %s\n", data->name);
}

M3_READER(SequenceTimeline) {
    M3_REFR(sb, data->name, Char, 0);
    M3_READ(sb, data->runsConcurrent, 0);
    M3_READ(sb, data->priority, 0);
    M3_READ(sb, data->stsIndex, 0);
    M3_READ(sb, data->stsIndexCopy, 0);
    M3_REFR(sb, data->animIds, Uint32, 0);
    M3_REFR(sb, data->animRefs, Uint32, 0);
    M3_READ(sb, data->d3, 0);
    FOR_LOOP(i, 13) {
        M3_READ(sb, data->sd[i], 0);
    }
}

M3_READER(SequenceValidator) {
    M3_REFR(sb, data->animIds, Uint32, 0);
    M3_READ(sb, data->unk, 0);
}

M3_READER(SequenceGetter) {
    M3_REFR(sb, data->name, Char, 0);
    M3_REFR(sb, data->stcID, Uint32, 0);
}

M3_READER(Bone) {
    M3_READ(sb, data->unknown0, 0);
    M3_REFR(sb, data->name, Char, 0);
    M3_READ(sb, data->flags, 0);
    M3_READ(sb, data->parent, 0);
    M3_READ(sb, data->unknown1, 0);
    M3_READ(sb, data->position, 0);
    M3_READ(sb, data->rotation, 0);
    M3_READ(sb, data->scale, 0);
    M3_READ(sb, data->visibility, 0);
}

void M3_MakeBuffer(m3Model_t *model) {
    model->renbuf = ri.MemAlloc(sizeof(BUFFER));

    R_Call(glGenVertexArrays, 1, &model->renbuf->vao);
    R_Call(glBindVertexArray, model->renbuf->vao);

    R_Call(glGenBuffers, 1, &model->renbuf->vbo);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, model->renbuf->vbo);

    R_Call(glEnableVertexAttribArray, attrib_position);
    R_Call(glEnableVertexAttribArray, attrib_texcoord);
    R_Call(glEnableVertexAttribArray, attrib_skin1);
    R_Call(glEnableVertexAttribArray, attrib_boneWeight1);
    R_Call(glEnableVertexAttribArray, attrib_normal);

    R_Call(glVertexAttribPointer, attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(m3Vertex_t), FOFS(m3Vertex_s, pos));
    R_Call(glVertexAttribPointer, attrib_texcoord, 2, GL_SHORT, GL_FALSE, sizeof(m3Vertex_t), FOFS(m3Vertex_s, uv));
    R_Call(glVertexAttribPointer, attrib_skin1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(m3Vertex_t), FOFS(m3Vertex_s, boneIndex));
    R_Call(glVertexAttribPointer, attrib_boneWeight1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(m3Vertex_t), FOFS(m3Vertex_s, boneWeight));
    R_Call(glVertexAttribPointer, attrib_normal, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(m3Vertex_t), FOFS(m3Vertex_s, normal));

    R_Call(glBufferData, GL_ARRAY_BUFFER, model->verticesNum * sizeof(m3Vertex_t), model->vertices, GL_STATIC_DRAW);
}

void M3_InitMODL(m3Model_t *model, m3Reader_t sb) {
    M3_REFR(&sb, model->modelName, Char, 0);
    M3_READ(&sb, model->flags, 0);
    M3_REFR(&sb, model->sequences, Sequence, 0);
    M3_REFR(&sb, model->stc, SequenceTimeline, 0);
    M3_REFR(&sb, model->stg, SequenceGetter, 0);
    M3_READ(&sb, model->unknown0, 0);
    M3_REFR(&sb, model->sts, SequenceValidator, 0);
    M3_REFR(&sb, model->bones, Bone, 0);
    M3_READ(&sb, model->numberOfBonesToCheckForSkin, 0);
    M3_READ(&sb, model->vertexFlags, 0);
    M3_REFR(&sb, model->vertices, Vertex, 0);
    M3_REFR(&sb, model->divisions, Divisions, 0);
    M3_REFR(&sb, model->boneLookup, Uint16, 0);
    M3_READ(&sb, model->boundings, 0);
    M3_READ(&sb, model->unknown4, 0);
    M3_READ(&sb, model->attachmentPoints, 0);
    M3_READ(&sb, model->attachmentPointAddons, 0);
    M3_READ(&sb, model->ligts, 0);
    M3_READ(&sb, model->shbxData, 0);
    M3_READ(&sb, model->cameras, 0);
    M3_READ(&sb, model->unknown21, 0);
    // Materials
    M3_REFR(&sb, model->materialReferences, MaterialReference, 0);
    M3_REFR(&sb, model->materialStandard, Material, 0);
    M3_READ(&sb, model->materialDisplacement, 0);
    M3_READ(&sb, model->materialComposite, 0);
    M3_READ(&sb, model->materialTerrain, 0);
    M3_READ(&sb, model->materialVolume, 0);
    M3_READ(&sb, model->materialUnknown1, 0);
    M3_READ(&sb, model->materialCreep, 0);
    M3_READ(&sb, model->materialVolumeNoise, 24);
    M3_READ(&sb, model->materialSplatTerrainBake, 25);
    M3_READ(&sb, model->materialUnknown2, 27);
    M3_READ(&sb, model->materialLensFlare, 28);
    // Particles
    M3_READ(&sb, model->particleEmitters, 0);
    M3_READ(&sb, model->particleEmitterCopies, 0);
    M3_READ(&sb, model->ribbonEmitters, 0);
    M3_READ(&sb, model->projections, 0);
    M3_READ(&sb, model->forces, 0);
    M3_READ(&sb, model->warps, 0);
    M3_READ(&sb, model->unknown22, 0);
    M3_READ(&sb, model->rigidBodies, 0);
    M3_READ(&sb, model->unknown23, 0);
    M3_READ(&sb, model->physicsJoints, 0);
    M3_READ(&sb, model->clothBehavior, 27)
    M3_READ(&sb, model->unknown24, 0);
    M3_READ(&sb, model->ikjtData, 0);
    M3_READ(&sb, model->unknown25, 0);
    M3_READ(&sb, model->unknown26, 24);
    M3_READ(&sb, model->partsOfTurrentBehaviors, 0);
    M3_READ(&sb, model->turrentBehaviors, 0);
    M3_REFR(&sb, model->absoluteInverseBoneRestPositions, Matrix4, 0);
    M3_READ(&sb, model->tightHitTest, 0);
    M3_READ(&sb, model->fuzzyHitTestObjects, 0);
    M3_READ(&sb, model->attachmentVolumes, 0);
    M3_READ(&sb, model->attachmentVolumesAddon0, 0);
    M3_READ(&sb, model->attachmentVolumesAddon1, 0);
    M3_READ(&sb, model->billboardBehaviors, 0);
    M3_READ(&sb, model->tmdData, 0);
    M3_READ(&sb, model->unknown27, 0);
    M3_READ(&sb, model->unknown28, 0);

    M3_MakeBuffer(model);
}

m3Uint32_t M3_FindAnimRef(m3SequenceTimeline_t const *timeline, m3Uint32_t animID) {
    M3_FOR_EACH(Uint32, it, timeline->animIds) {
        if (animID == *it) {
            return timeline->animRefs[it - timeline->animIds];
        }
    }
    return 0;
}

DWORD M3_FindKeyAtTime(m3Uint32_t const *keys, DWORD numkeys, DWORD time, float *t) {
    if (numkeys == 0 || *keys > time)
        return 0;
    FOR_LOOP(b, numkeys) {
        if (keys[b] > time) {
            DWORD a = b - 1;
            *t = (float)(time - keys[a]) / (float)(keys[b] - keys[a]);
            return b;
        }
    }
    *t = 1;
    return numkeys - 1;
}

#define M3_GET_POINTER(MODEL, REF, TYPE) ((m3##TYPE##_t const *)M3_MakeSizeBuf(MODEL, REF).data)

#define M3_GET_ANIM_VALUE(ANIMREF, DATATYPE) \
M3_Get##ANIMREF##AnimValue(m3Model_t const *model, \
                           m3SequenceTimeline_t const *timeline, \
                           m3##ANIMREF##AnimRef_t const *animref, \
                           DWORD time) { \
    m3Uint32_t const anim = M3_FindAnimRef(timeline, animref->animId); \
    if (anim == 0) return animref->initValue; \
    m3SequenceData_t const *sd = M3_GET_POINTER(model,timeline->sd[anim >> 16],SequenceData)+(anim&0xffff); \
    m3##ANIMREF##_t output = animref->initValue; \
    m3Float32_t t = 0.f; \
    DWORD key = M3_FindKeyAtTime(M3_GET_POINTER(model, sd->keys, Uint32), sd->keys.nEntries, time, &t); \
    if (key > 0) { \
        m3##ANIMREF##_t const *values = M3_GET_POINTER(model, sd->values, ANIMREF); \
        R_EvalKeyframeValue(values+key-1, values+key, t, DATATYPE, TRACK_LINEAR, &output); \
        return output; \
    } else { \
        return animref->initValue; \
    } \
}

DWORD   M3_GET_ANIM_VALUE(Uint32,  TDATA_INT1);
float   M3_GET_ANIM_VALUE(Float32, TDATA_FLOAT1);
VECTOR3 M3_GET_ANIM_VALUE(Vector3, TDATA_FLOAT3);
VECTOR4 M3_GET_ANIM_VALUE(Vector4, TDATA_FLOAT4);

m3Model_t *R_LoadModelM3(void *data, DWORD size) {
    m3Model_t *model = ri.MemAlloc(sizeof(m3Model_t));
    model->buffer = malloc(size);
    model->size = size;
    memcpy(model->buffer, data, size);
    model->head = model->buffer;
    model->refs = model->buffer + model->head->ofsRefs;
    model->type = model->refs[model->head->MODL.ref].version;
    currentmodel = model;
    M3_InitMODL(model, M3_MakeSizeBuf(model, model->head->MODL));
    return model;
}

void M3_DrawDivisions(m3Model_t const *model, m3Divisions_t const *divisions) {
    R_Call(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, divisions->indicesBuffer);
    M3_FOR_EACH(Batch, batch, divisions->batches) {
        m3Region_t const *region = divisions->regions+batch->regionIndex;
        m3MaterialReference_t const *mref = model->materialReferences+batch->materialReferenceIndex;
        m3Material_t const *material = model->materialStandard+mref->materialIndex;
        DWORD const num_indices = region->triangleIndicesCount;
        DWORD const first_vertex = region->firstVertexIndex;
        HANDLE const indices = (HANDLE)(sizeof(USHORT) * region->firstTriangleIndex);
        R_Call(glUniform1f, m3.uFirstBoneLookupIndex, region->firstBoneLookupIndex);
        R_Call(glUniform1f, m3.uBoneWeightPairsCount, region->boneWeightPairsCount);
        
        R_Call(glActiveTexture, GL_TEXTURE0);
        R_Call(glBindTexture, GL_TEXTURE_2D, material->diffuseLayer->texture->texid);
        R_Call(glActiveTexture, GL_TEXTURE3);
        R_Call(glBindTexture, GL_TEXTURE_2D, material->specularLayer->texture->texid);
        R_Call(glActiveTexture, GL_TEXTURE4);
        R_Call(glBindTexture, GL_TEXTURE_2D, material->normalLayer->texture->texid);
        
        M3_FOR_EACH(Layer, layer, material->diffuseLayer) {
            if (!layer->texture)
                continue;
            R_Call(glDrawElementsBaseVertex, GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, indices, first_vertex);
        }
    }
}

void M3_MakeBoneMatrix(LPCVECTOR3 p, LPCVECTOR4 r, LPCVECTOR3 s, LPCMATRIX4 par, LPMATRIX4 m) {
    MATRIX4 matrix;
    Matrix4_identity(&matrix);
    Matrix4_translate(&matrix, p);
    Matrix4_rotate4(&matrix, r);
    Matrix4_scale(&matrix, s);
    Matrix4_multiply(par, &matrix, m);
}

//m3Sequence_t const *
//M3_FindSequenceByName(m3Model_t const *model,
//                      LPCSTR name)
//{
//    M3_FOR_EACH(Sequence, seq, model->sequences) {
//        if (!strcmp(seq->name, name)) {
//            return seq;
//        }
//    }
//    return NULL;
//}

m3SequenceTimeline_t const *
M3_FindSequenceTimeline(m3Model_t const *model,
                        m3Sequence_t const *seq)
{
    M3_FOR_EACH(SequenceTimeline, stc, model->stc) {
        if (stc->stsIndex == seq - model->sequences) {
            return stc;
        }
    }
    return NULL;
}

m3SequenceTimeline_t const *
M3_FindAnimationAtTime(m3Model_t const *model,
                       DWORD time,
                       DWORD *localtime)
{
    M3_FOR_EACH(Sequence, seq, model->sequences) {
        if (time < seq->interval[1]) {
            *localtime = time;
            return M3_FindSequenceTimeline(model, seq);
        } else {
            time -= seq->interval[1];
        }
    }
    return NULL;
}

void M3_RenderModel(renderEntity_t const *entity, m3Model_t const *model, LPCMATRIX4 transform) {
    MATRIX4 identity;
    Matrix4_identity(&identity);
    
    struct {
        m3SequenceTimeline_t const *stc;
        DWORD time;
    } a, b;
    
    a.stc = M3_FindAnimationAtTime(model, entity->oldframe, &a.time);
    b.stc = M3_FindAnimationAtTime(model, entity->frame, &b.time);

    M3_FOR_EACH(Bone, bone, model->bones) {
        LPCMATRIX4 parent = bone->parent >= 0 ? tmp+bone->parent : &identity;
        VECTOR3 a_p = M3_GetVector3AnimValue(model, a.stc, &bone->position, a.time);
        VECTOR4 a_r = M3_GetVector4AnimValue(model, a.stc, &bone->rotation, a.time);
        VECTOR3 a_s = M3_GetVector3AnimValue(model, a.stc, &bone->scale, a.time);
        VECTOR3 b_p = M3_GetVector3AnimValue(model, b.stc, &bone->position, b.time);
        VECTOR4 b_r = M3_GetVector4AnimValue(model, b.stc, &bone->rotation, b.time);
        VECTOR3 b_s = M3_GetVector3AnimValue(model, b.stc, &bone->scale, b.time);
        VECTOR3 p = Vector3_lerp(&a_p, &b_p, tr.viewDef.lerpfrac);
        QUATERNION r = Quaternion_slerp((LPCQUATERNION)&a_r, (LPCQUATERNION)&b_r, tr.viewDef.lerpfrac);
        VECTOR3 s = Vector3_lerp(&a_s, &b_s, tr.viewDef.lerpfrac);
//        float v = M3_GetUint32AnimValue(model, a.stc, &bone->visibility, a.time);
        M3_MakeBoneMatrix(&p, (LPCVECTOR4)&r, &s, parent, tmp+(bone-model->bones));
    }

    M3_FOR_EACH(Uint16, boneLookup, model->boneLookup) {
        m3Uint16_t boneIndex = *boneLookup;
        LPCMATRIX4 bonematrix = tmp+boneIndex;
        LPCMATRIX4 baseline = model->absoluteInverseBoneRestPositions+boneIndex;
        Matrix4_multiply(bonematrix, baseline, bonemats+(boneLookup-model->boneLookup));
    }

    MATRIX4 mScaledMatrix;
    MATRIX3 mNormalMatrix;

    memcpy(&mScaledMatrix, transform, sizeof(MATRIX4));
    Matrix4_rotate(&mScaledMatrix, &(VECTOR3){0,0,90/*tr.viewDef.time*0.05*/}, ROTATE_ZYX);
    Matrix4_scale(&mScaledMatrix, &(VECTOR3){100,100,100});
    Matrix3_normal(&mNormalMatrix, &mScaledMatrix);

    R_Call(glDisable, GL_BLEND);
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glUseProgram, m3.shader->progid);
    extern bool is_rendering_lights;
    if (is_rendering_lights) {
        R_Call(glUniformMatrix4fv, m3.shader->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    } else {
        R_Call(glUniformMatrix4fv, m3.shader->uViewProjectionMatrix, 1, GL_FALSE, tr.viewDef.viewProjectionMatrix.v);
    }
    R_Call(glUniformMatrix4fv, m3.shader->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix4fv, m3.shader->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniformMatrix4fv, m3.shader->uModelMatrix, 1, GL_FALSE, mScaledMatrix.v);
    R_Call(glUniformMatrix3fv, m3.shader->uNormalMatrix, 1, GL_TRUE, mNormalMatrix.v);
    R_Call(glUniformMatrix4fv, m3.shader->uBones, model->bonesNum, GL_FALSE, bonemats->v);
    R_Call(glBindVertexArray, model->renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, model->renbuf->vbo);
    
    R_BindTexture(tr.texture[TEX_WHITE], 0);
    
    R_Call(glDisable, GL_CULL_FACE);
    
    M3_FOR_EACH(Divisions, div, model->divisions) {
        M3_DrawDivisions(model, div);
    }
    
    R_Call(glEnable, GL_BLEND);
}

void M3_Init(void) {
    m3.shader = R_InitShader(vs, fs);
    m3.uFirstBoneLookupIndex = R_Call(glGetUniformLocation, m3.shader->progid, "uFirstBoneLookupIndex");
    m3.uBoneWeightPairsCount = R_Call(glGetUniformLocation, m3.shader->progid, "uBoneWeightPairsCount");
    m3.uDiffuseMap = R_Call(glGetUniformLocation, m3.shader->progid, "uDiffuseMap");
    m3.uSpecularMap = R_Call(glGetUniformLocation, m3.shader->progid, "uSpecularMap");
    m3.uNormalMap = R_Call(glGetUniformLocation, m3.shader->progid, "uNormalMap");
    
    R_Call(glUniform1i, m3.uDiffuseMap, 0);
    R_Call(glUniform1i, m3.uSpecularMap, 3);
    R_Call(glUniform1i, m3.uNormalMap, 4);
}

void M3_Shutdown(void) {
    R_ReleaseShader(m3.shader);
}
