#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mdx.h"
#include "../common/common.h"

#define LPCSTR const char *
#define FOR_EACH_LIST(type, property, list) \
for (type *property = list, *next = list ? (list)->next : NULL; \
property; \
property = next, next = next ? next->next : NULL)
#define SAFE_DELETE(x, func) if (x) { func(x); (x) = NULL; }
#define SFileReadArray2(mload, object, variable, elemsize) \
ReadBuffer(mload, &object->num_##variable, 4); \
if (object->num_##variable > 0) {object->variable = MemAlloc(object->num_##variable * elemsize); \
ReadBuffer(mload, object->variable, object->num_##variable * elemsize); }
#define MODEL_READ_LIST(FILE, BLOCK, TYPE, TYPES) \
while (!FileIsAtEndOfBlock(FILE, &BLOCK)) { \
    mdx##TYPE##_t *p_##TYPE = Read##TYPE(FILE, FileReadBlock(FILE)); \
    if (model->TYPES) { \
        mdx##TYPE##_t *last##TYPE = model->TYPES; \
        while (last##TYPE->next) last##TYPE = last##TYPE->next; \
        last##TYPE->next = p_##TYPE; \
    } else { \
        model->TYPES = p_##TYPE; \
    } \
}
#define MODEL_READ_ARRAY(FILE, BLOCK, TYPE, TYPES) \
model->TYPES = MemAlloc(BLOCK.size); \
model->num_##TYPES = BLOCK.size / sizeof(mdx##TYPE##_t); \
ReadBuffer(FILE, model->TYPES, BLOCK.size);

enum {
    ID_VERS = MAKEFOURCC('V','E','R','S'),
    ID_MODL = MAKEFOURCC('M','O','D','L'),
    ID_SEQS = MAKEFOURCC('S','E','Q','S'),
    ID_PIVT = MAKEFOURCC('P','I','V','T'),
    ID_GEOS = MAKEFOURCC('G','E','O','S'),
    ID_GEOA = MAKEFOURCC('G','E','O','A'),
    ID_GLBS = MAKEFOURCC('G','L','B','S'),
    ID_MTLS = MAKEFOURCC('M','T','L','S'),
    ID_BONE = MAKEFOURCC('B','O','N','E'),
    ID_HELP = MAKEFOURCC('H','E','L','P'),
    ID_TEXS = MAKEFOURCC('T','E','X','S'),
    ID_VRTX = MAKEFOURCC('V','R','T','X'),
    ID_NRMS = MAKEFOURCC('N','R','M','S'),
    ID_UVBS = MAKEFOURCC('U','V','B','S'),
    ID_PTYP = MAKEFOURCC('P','T','Y','P'),
    ID_PCNT = MAKEFOURCC('P','C','N','T'),
    ID_PVTX = MAKEFOURCC('P','V','T','X'),
    ID_GNDX = MAKEFOURCC('G','N','D','X'),
    ID_MTGC = MAKEFOURCC('M','T','G','C'),
    ID_UVAS = MAKEFOURCC('U','V','A','S'),
    ID_MATS = MAKEFOURCC('M','A','T','S'),
    ID_LAYS = MAKEFOURCC('L','A','Y','S'),
    ID_KGAO = MAKEFOURCC('K','G','A','O'),
    ID_KGTR = MAKEFOURCC('K','G','T','R'),
    ID_KGRT = MAKEFOURCC('K','G','R','T'),
    ID_KGSC = MAKEFOURCC('K','G','S','C'),
    ID_EVTS = MAKEFOURCC('E','V','T','S'),
    ID_KEVT = MAKEFOURCC('K','E','V','T'),
    ID_CAMS = MAKEFOURCC('C','A','M','S'),
    ID_KCTR = MAKEFOURCC('K','C','T','R'),
    ID_KTTR = MAKEFOURCC('K','T','T','R'),
    ID_KCRL = MAKEFOURCC('K','C','R','L'),

};

typedef struct {
    void *data;
    dword_t cursize;
    dword_t readcount;
} buffer_t;

typedef struct {
    dword_t header;
    dword_t size;
    dword_t start;
} tFileBlock_t;

int MSG_Read(buffer_t *buffer, void *dest, dword_t bytes) {
    if (buffer->readcount + bytes > buffer->cursize)
        return 0;
    memcpy(dest, (char *)buffer->data + buffer->readcount, bytes);
    buffer->readcount += bytes;
    return bytes;
}

dword_t GetModelKeyTrackDataTypeSize(MODELKEYTRACKDATATYPE dataType) {
    switch (dataType) {
        case TDATA_FLOAT: return 4;
        case TDATA_VECTOR3: return 12;
        case TDATA_QUATERNION: return 16;
        default: return 0;
    }
}

dword_t GetModelKeyTrackTypeSize(MODELKEYTRACKTYPE keyTrackType) {
    switch (keyTrackType) {
        case TRACK_NO_INTERP: return 1;
        case TRACK_LINEAR: return 1;
        case TRACK_HERMITE: return 3;
        case TRACK_BEZIER: return 3;
        default: return 0;
    }
}

dword_t GetModelKeyFrameSize(MODELKEYTRACKDATATYPE dataType,
                             MODELKEYTRACKTYPE keyTrackType)
{
    return 4 + GetModelKeyTrackDataTypeSize(dataType) * GetModelKeyTrackTypeSize(keyTrackType);
}

tFileBlock_t FileReadBlock(buffer_t *mload) {
    tFileBlock_t block;
    block.header = 0;
    block.start = mload->readcount;
    MSG_Read(mload, &block.size, 4);
    return block;
}

dword_t MSG_ReadLong(buffer_t *mload) {
    dword_t value = 0;
    MSG_Read(mload, &value, 4);
    return value;
}

int FileIsAtEndOfBlock(buffer_t *mload, tFileBlock_t const *block) {
    dword_t const filePointer = mload->readcount;
    dword_t const endOfBlock = block->start + block->size;
    return filePointer >= endOfBlock;
}

void FileMoveToEndOfBlock(buffer_t *mload, tFileBlock_t const *block) {
    mload->readcount = block->start + block->size;
}

mdxGeoset_t *ReadGeoset(buffer_t *mload, tFileBlock_t const block) {
    mdxGeoset_t *geoset = MemAlloc(sizeof(mdxGeoset_t));
    while (!FileIsAtEndOfBlock(mload, &block)) {
        int tag = MSG_ReadLong(mload);
        switch (tag) {
            case ID_VRTX: SFileReadArray2(mload, geoset, vertices, sizeof(mdxVec3_t)); break;
            case ID_NRMS: SFileReadArray2(mload, geoset, normals, sizeof(mdxVec3_t)); break;
            case ID_UVBS: SFileReadArray2(mload, geoset, texcoord, sizeof(mdxVec2_t)); break;
            case ID_PTYP: SFileReadArray2(mload, geoset, primitiveTypes, sizeof(int)); break;
            case ID_PCNT: SFileReadArray2(mload, geoset, primitiveCounts, sizeof(int)); break;
            case ID_PVTX: SFileReadArray2(mload, geoset, triangles, sizeof(short)); break;
            case ID_GNDX: SFileReadArray2(mload, geoset, vertexGroups, sizeof(char)); break;
            case ID_MTGC: SFileReadArray2(mload, geoset, matrixGroupSizes, sizeof(int)); break;
            case ID_UVAS: MSG_Read(mload, &geoset->num_texcoordChannels, sizeof(int)); break;
            case ID_MATS:
                SFileReadArray2(mload, geoset, matrices, sizeof(int));
                MSG_Read(mload, &geoset->materialID, sizeof(int));
                MSG_Read(mload, &geoset->group, sizeof(int));
                MSG_Read(mload, &geoset->selectable, sizeof(int));
                MSG_Read(mload, &geoset->default_bounds, sizeof(mdxBounds_t));
                SFileReadArray2(mload, geoset, bounds, sizeof(mdxBounds_t));
                break;
            default:
                break;
        }
    };
    return geoset;
}

void ReadMaterialLayer(buffer_t *mload, mdxMaterialLayer_t *layer, tFileBlock_t const block) {
    layer->blendMode = MSG_ReadLong(mload);
    layer->flags = MSG_ReadLong(mload);
    layer->textureId = MSG_ReadLong(mload);
    layer->transformId = MSG_ReadLong(mload);
    layer->coordId = MSG_ReadLong(mload);
    layer->staticAlpha = MSG_ReadLong(mload);
    FileMoveToEndOfBlock(mload, &block);
}

mdxMaterial_t *ReadMaterial(buffer_t *mload, tFileBlock_t const block) {
    mdxMaterial_t *material = MemAlloc(sizeof(mdxMaterial_t));
    material->priority = MSG_ReadLong(mload);
    material->flags = MSG_ReadLong(mload);
    while (!FileIsAtEndOfBlock(mload, &block)) {
        dword_t const blockHeader = MSG_ReadLong(mload);
        switch (blockHeader) {
            case ID_LAYS:
                MSG_Read(mload, &material->num_layers, 4);
                if (material->num_layers > 0) {
                    material->layers = MemAlloc(sizeof(mdxMaterialLayer_t) * material->num_layers);
                    FOR_LOOP(layerID, material->num_layers) {
                        ReadMaterialLayer(mload, &material->layers[layerID], FileReadBlock(mload));
                    }
                }
                break;
            default:
//                PrintTag(blockHeader);
                FileMoveToEndOfBlock(mload, &block);
                return material;
        }
    };
    return material;
}

HANDLE ReadKeyTrack(buffer_t *mload, MODELKEYTRACKDATATYPE dataType) {
    dword_t keyframeCount = MSG_ReadLong(mload);
    MODELKEYTRACKTYPE keyTrackType = MSG_ReadLong(mload);
    dword_t globalSeqId = MSG_ReadLong(mload);
    dword_t const dataSize = GetModelKeyFrameSize(dataType, keyTrackType) * keyframeCount;
    mdxKeyTrack_t *keytrack = MemAlloc(sizeof(mdxKeyTrack_t) + dataSize);
    keytrack->keyframeCount = keyframeCount;
    keytrack->datatype = dataType;
    keytrack->type = keyTrackType;
    keytrack->globalSeqId = globalSeqId;
    MSG_Read(mload, keytrack->values, dataSize);
    return keytrack;
}

void ReadNode(buffer_t *mload, tFileBlock_t const block, mdxNode_t *node) {
    MSG_Read(mload, &node->name, sizeof(mdxObjectName_t));
    node->object_id = MSG_ReadLong(mload);
    node->parent_id = MSG_ReadLong(mload);
    node->flags = MSG_ReadLong(mload);
    while (!FileIsAtEndOfBlock(mload, &block)) {
        switch (MSG_ReadLong(mload)) {
            case ID_KGTR:
                node->translation = ReadKeyTrack(mload, TDATA_VECTOR3);
                break;
            case ID_KGRT:
                node->rotation = ReadKeyTrack(mload, TDATA_QUATERNION);
                break;
            case ID_KGSC:
                node->scale = ReadKeyTrack(mload, TDATA_VECTOR3);
                break;
            default:
                break;
        }
    }
}

mdxBone_t *ReadBone(buffer_t *mload, tFileBlock_t const block) {
    mdxBone_t *bone = MemAlloc(sizeof(mdxBone_t));
    ReadNode(mload, block, &bone->node);
    bone->geoset_id = MSG_ReadLong(mload);
    bone->geoset_animation_id = MSG_ReadLong(mload);
    return bone;
}

mdxHelper_t *ReadHelper(buffer_t *mload, tFileBlock_t const block) {
    mdxHelper_t *helper = MemAlloc(sizeof(mdxHelper_t));
    ReadNode(mload, block, &helper->node);
    return helper;
}

mdxCamera_t *ReadCamera(buffer_t *mload, tFileBlock_t const block) {
    mdxCamera_t *camera = MemAlloc(sizeof(mdxCamera_t));
    MSG_Read(mload, &camera->name, sizeof(mdxObjectName_t));
    MSG_Read(mload, &camera->pivot, sizeof(mdxVec3_t));
    MSG_Read(mload, &camera->fieldOfView, sizeof(float));
    MSG_Read(mload, &camera->farClip, sizeof(float));
    MSG_Read(mload, &camera->nearClip, sizeof(float));
    MSG_Read(mload, &camera->targetPivot, sizeof(mdxVec3_t));

    while (!FileIsAtEndOfBlock(mload, &block)) {
        switch (MSG_ReadLong(mload)) {
            case ID_KCTR:
                camera->translation = ReadKeyTrack(mload, TDATA_VECTOR3);
                break;
            case ID_KTTR:
                camera->targetTranslation = ReadKeyTrack(mload, TDATA_VECTOR3);
                break;
            case ID_KCRL:
                camera->roll = ReadKeyTrack(mload, TDATA_FLOAT);
                break;
            default:
                break;
        }
    }
    return camera;
}



mdxEvent_t *ReadEvent(buffer_t *mload, tFileBlock_t const block) {
    mdxEvent_t *event = MemAlloc(sizeof(mdxEvent_t));
    ReadNode(mload, block, &event->node);
    dword_t header = MSG_ReadLong(mload);
    switch (header) {
        case ID_KEVT:
            event->num_keys = MSG_ReadLong(mload);
            event->globalSeqId = MSG_ReadLong(mload);
            event->keys = MemAlloc(event->num_keys * sizeof(dword_t));
            MSG_Read(mload, event->keys, event->num_keys * sizeof(dword_t));
            break;
    }
    return event;
}

mdxGeosetAnim_t *ReadGeosetAnim(buffer_t *mload, tFileBlock_t const block) {
    mdxGeosetAnim_t *geosetAnim = MemAlloc(sizeof(mdxGeosetAnim_t));
    MSG_Read(mload, geosetAnim, 24);
    while (!FileIsAtEndOfBlock(mload, &block)) {
        dword_t const blockHeader = MSG_ReadLong(mload);
        switch (blockHeader) {
            case ID_KGAO:
                geosetAnim->alphas = ReadKeyTrack(mload, TDATA_FLOAT);
                break;
            default:
//                PrintTag(blockHeader);
                FileMoveToEndOfBlock(mload, &block);
                return geosetAnim;
        }
    };
    return geosetAnim;
}

mdxNode_t *MDX_GetModelNodeWithObjectID(mdxModel_t *model, dword_t objectID) {
    if (objectID == -1) {
        return NULL;
    }
    FOR_EACH_LIST(mdxBone_t, bone, model->bones) {
        if (bone->node.object_id == objectID) {
            return &bone->node;
        }
    }
    FOR_EACH_LIST(mdxHelper_t, helper, model->helpers) {
        if (helper->node.object_id == objectID) {
            return &helper->node;
        }
    }
    return NULL;
}

mdxModel_t *MDX_LoadBuffer(void *buffer, dword_t size) {
    dword_t blockHeader, fileVersion = 0;
    mdxModel_t *model = MemAlloc(sizeof(mdxModel_t));
    buffer_t mload2 = { .data = buffer, .cursize = size, .readcount = 4 };
    buffer_t *mload = &mload2;
    tFileBlock_t block;
    while (MSG_Read(mload, &blockHeader, 4)) {
        MSG_Read(mload, &block.size, 4);
        block.header = 0;
        block.start = mload->readcount;
        switch (blockHeader) {
            case ID_VERS:
                if ((fileVersion = MSG_ReadLong(mload)) != 800) {
                    fprintf(stderr, "Usupported MDLX version %d\n", fileVersion);
                    return NULL;
                }
                break;
            case ID_MODL: MSG_Read(mload, &model->info, sizeof(mdxInfo_t)); break;
            case ID_EVTS: MODEL_READ_LIST(mload, block, Event, events); break;
            case ID_GEOS: MODEL_READ_LIST(mload, block, Geoset, geosets); break;
            case ID_MTLS: MODEL_READ_LIST(mload, block, Material, materials); break;
            case ID_BONE: MODEL_READ_LIST(mload, block, Bone, bones); break;
            case ID_GEOA: MODEL_READ_LIST(mload, block, GeosetAnim, geosetAnims); break;
            case ID_HELP: MODEL_READ_LIST(mload, block, Helper, helpers); break;
            case ID_CAMS: MODEL_READ_LIST(mload, block, Camera, cameras); break;
            case ID_SEQS: MODEL_READ_ARRAY(mload, block, Sequence, sequences); break;
            case ID_GLBS: MODEL_READ_ARRAY(mload, block, GlobalSequence, globalSequences); break;
            case ID_PIVT: MODEL_READ_ARRAY(mload, block, Vec3, pivots); break;
            case ID_TEXS: MODEL_READ_ARRAY(mload, block, Texture, textures); break;
            default:
//                PrintTag(blockHeader);
                mload->readcount += block.size;;
                break;
        }
    }
    FOR_EACH_LIST(mdxBone_t, bone, model->bones) {
        model->nodes[bone->node.object_id] = &bone->node;
    }
    FOR_EACH_LIST(mdxHelper_t, helper, model->helpers) {
        model->nodes[helper->node.object_id] = &helper->node;
    }
    FOR_EACH_LIST(mdxGeosetAnim_t, geosetAnim, model->geosetAnims) {
        mdxGeoset_t *geoset = model->geosets;
        for (dword_t geosetID = geosetAnim->geosetId;
             geoset && geosetID > 0;
             geosetID--)
        {
            geoset = geoset->next;
        }
        if (geoset) {
            geoset->geosetAnim = geosetAnim;
        }
    }
    return model;
}

void MDX_ReleaseModelNode(mdxNode_t *node) {
    SAFE_DELETE(node->translation, free);
    SAFE_DELETE(node->rotation, free);
    SAFE_DELETE(node->scale, free);
}

void MDX_ReleaseModelGeoset(mdxGeoset_t *geoset) {
    SAFE_DELETE(geoset->next, MDX_ReleaseModelGeoset);
    SAFE_DELETE(geoset->vertices, free);
    SAFE_DELETE(geoset->normals, free);
    SAFE_DELETE(geoset->texcoord, free);
    SAFE_DELETE(geoset->matrices, free);
    SAFE_DELETE(geoset->primitiveTypes, free);
    SAFE_DELETE(geoset->primitiveCounts, free);
    SAFE_DELETE(geoset->triangles, free);
    SAFE_DELETE(geoset->vertexGroups, free);
    SAFE_DELETE(geoset->matrixGroupSizes, free);
    SAFE_DELETE(geoset->bounds, free);
    SAFE_DELETE(geoset, free);
}

void MDX_ReleaseModelMaterial(mdxMaterial_t *material) {
    SAFE_DELETE(material->next, MDX_ReleaseModelMaterial);
    SAFE_DELETE(material, free);
}

void MDX_ReleaseModelBone(mdxBone_t *bone) {
    MDX_ReleaseModelNode(&bone->node);
    SAFE_DELETE(bone->next, MDX_ReleaseModelBone);
    SAFE_DELETE(bone, free);
}

void MDX_ReleaseModelGeosetAnim(mdxGeosetAnim_t *geosetAnim) {
    SAFE_DELETE(geosetAnim->next, MDX_ReleaseModelGeosetAnim);
    SAFE_DELETE(geosetAnim->alphas, free);
    SAFE_DELETE(geosetAnim, free);
}

void MDX_ReleaseModelHelper(mdxHelper_t *helper) {
    MDX_ReleaseModelNode(&helper->node);
    SAFE_DELETE(helper->next, MDX_ReleaseModelHelper);
    SAFE_DELETE(helper, free);
}

void MDX_Release(mdxModel_t *model) {
    SAFE_DELETE(model->geosets, MDX_ReleaseModelGeoset);
    SAFE_DELETE(model->materials, MDX_ReleaseModelMaterial);
    SAFE_DELETE(model->bones, MDX_ReleaseModelBone);
    SAFE_DELETE(model->geosetAnims, MDX_ReleaseModelGeosetAnim);
    SAFE_DELETE(model->helpers, MDX_ReleaseModelHelper);
    SAFE_DELETE(model->textures, free);
    SAFE_DELETE(model->sequences, free);
    SAFE_DELETE(model->globalSequences, free);
    SAFE_DELETE(model->pivots, free);
}

