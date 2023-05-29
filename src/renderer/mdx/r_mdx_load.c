#include "r_mdx.h"
#include "r_local.h"

#define LPCSTR const char *
#define FOR_EACH_LIST(type, property, list) \
for (type *property = list, *next = list ? (list)->next : NULL; \
property; \
property = next, next = next ? next->next : NULL)
#define SAFE_DELETE(x, func) if (x) { func(x); (x) = NULL; }
#define SFileReadArray2(buffer, object, variable, elemsize) \
MSG_Read(buffer, &object->num_##variable, 4); \
if (object->num_##variable > 0) {object->variable = ri.MemAlloc(object->num_##variable * elemsize); \
MSG_Read(buffer, object->variable, object->num_##variable * elemsize); }

#define MODEL_READ_LIST(BLOCK, TYPE, TYPES) \
while (!FileIsAtEndOfBlock(BLOCK)) { \
    struct sizebuf inner = FileReadBlock(BLOCK); \
    mdx##TYPE##_t *p_##TYPE = Read##TYPE(&inner); \
    PUSH_BACK(mdx##TYPE##_t, p_##TYPE, model->TYPES); \
    BLOCK->readcount += inner.readcount; \
}
#define MODEL_READ_ARRAY(BLOCK, TYPE, TYPES) \
model->TYPES = ri.MemAlloc(BLOCK->cursize); \
model->num_##TYPES = BLOCK->cursize / sizeof(mdx##TYPE##_t); \
MSG_Read(BLOCK, model->TYPES, BLOCK->cursize);

enum {
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
    ID_KGAC = MAKEFOURCC('K','G','A','C'),
    ID_KGTR = MAKEFOURCC('K','G','T','R'),
    ID_KGRT = MAKEFOURCC('K','G','R','T'),
    ID_KGSC = MAKEFOURCC('K','G','S','C'),
    ID_KEVT = MAKEFOURCC('K','E','V','T'),
    ID_KCTR = MAKEFOURCC('K','C','T','R'),
    ID_KTTR = MAKEFOURCC('K','T','T','R'),
    ID_KCRL = MAKEFOURCC('K','C','R','L'),
    ID_KMTE = MAKEFOURCC('K','M','T','E'),
    ID_KMTA = MAKEFOURCC('K','M','T','A'),
    ID_KMTF = MAKEFOURCC('K','M','T','F'),
};

typedef struct {
    DWORD header;
    DWORD size;
    DWORD start;
} tFileBlock_t;

DWORD GetModelKeyTrackDataTypeSize(MODELKEYTRACKDATATYPE dataType) {
    switch (dataType) {
        case TDATA_INT: return 4;
        case TDATA_FLOAT: return 4;
        case TDATA_VECTOR3: return 12;
        case TDATA_QUATERNION: return 16;
        default: return 0;
    }
}

DWORD GetModelKeyTrackTypeSize(MODELKEYTRACKTYPE keyTrackType) {
    switch (keyTrackType) {
        case TRACK_NO_INTERP: return 1;
        case TRACK_LINEAR: return 1;
        case TRACK_HERMITE: return 3;
        case TRACK_BEZIER: return 3;
        default: return 0;
    }
}

DWORD GetModelKeyFrameSize(MODELKEYTRACKDATATYPE dataType,
                           MODELKEYTRACKTYPE keyTrackType)
{
    return 4 + GetModelKeyTrackDataTypeSize(dataType) * GetModelKeyTrackTypeSize(keyTrackType);
}

DWORD R_ModelFindBiggestGroup(mdxGeoset_t const *geoset) {
    DWORD biggest = 0;
    FOR_LOOP(i, geoset->num_matrixGroupSizes) {
        biggest = MAX(geoset->matrixGroupSizes[i], biggest);
    }
    return biggest;
}

void R_SetupGeoset(mdxModel_t *model, mdxGeoset_t *geoset) {
    DWORD biggestGeoset = R_ModelFindBiggestGroup(geoset);
    if (biggestGeoset > MAX_SKIN_BONES) {
        ri.error("Geoset with more that %d bones skinning int\n", MAX_SKIN_BONES);
        biggestGeoset = MAX_SKIN_BONES;
    }

    typedef BYTE matrixGroup_t[MAX_SKIN_BONES];
    vertex_t *vertices = ri.MemAlloc(sizeof(vertex_t) * geoset->num_triangles);
    matrixGroup_t *matrixGroups = ri.MemAlloc(sizeof(matrixGroup_t) * geoset->num_matrixGroupSizes);
    DWORD indexOffset = 0;

    FOR_LOOP(matrixGroupIndex, geoset->num_matrixGroupSizes) {
        memset(&matrixGroups[matrixGroupIndex], MAX_BONES - 1, sizeof(matrixGroup_t));
        FOR_LOOP(matrixIndex, geoset->matrixGroupSizes[matrixGroupIndex]) {
            matrixGroups[matrixGroupIndex][matrixIndex] = geoset->matrices[indexOffset++];
        }
    }

    FOR_LOOP(triangle, geoset->num_triangles) {
        DWORD vertex = geoset->triangles[triangle];
        DWORD matrixGroupIndex = geoset->vertexGroups[vertex];
        DWORD matrixGroupSize = MAX(1, geoset->matrixGroupSizes[matrixGroupIndex]);
        vertex_t *vrtx = &vertices[triangle];
        BYTE *matrixGroup = matrixGroups[matrixGroupIndex];
        vrtx->color = (color32_t) { 255, 255, 255, 255 };
        if (geoset->vertices) vrtx->position = *(struct vector3 *)&geoset->vertices[vertex];
        if (geoset->texcoord) vrtx->texcoord = *(struct vector2 *)&geoset->texcoord[vertex];
        if (geoset->normals) vrtx->normal = *(struct vector3 *)&geoset->normals[vertex];
        memcpy(vrtx->skin, matrixGroup, sizeof(matrixGroup_t));
        memset(vrtx->boneWeight, 0, sizeof(matrixGroup_t));
        uint8_t leftover = 255;
        uint8_t leftoversize = matrixGroupSize;
        FOR_LOOP(matrixIndex, matrixGroupSize) {
            uint8_t value = (float)leftover / (float)leftoversize;
            vrtx->boneWeight[matrixIndex] = value;
            leftover = MAX(0, leftover - value);
            leftoversize = MAX(1, leftoversize - 1);
        }
    }

    geoset->userdata = R_MakeVertexArrayObject(vertices, geoset->num_triangles * sizeof(vertex_t));

    ri.MemFree(vertices);
    ri.MemFree(matrixGroups);
}

mdxModel_t *R_LoadModelMDX(void *buffer, DWORD size) {
    mdxModel_t *model = MDX_LoadBuffer(buffer, size);
    if (model) {
        FOR_EACH_LIST(mdxGeoset_t, geoset, model->geosets) {
            R_SetupGeoset(model, geoset);
        }
        FOR_LOOP(texid, model->num_textures) {
            model->textures[texid].texid = R_RegisterTextureFile(model->textures[texid].path);
        }
    }
    return model;
}

model_t *R_LoadModel(LPCSTR modelFilename) {
    DWORD fileHeader;
    HANDLE file = ri.FileOpen(modelFilename);
    model_t *model = NULL;
    if (file == NULL) {
        // try to load without *0.mdx
        PATHSTR tempFileName;
        strncpy(tempFileName, modelFilename, strlen(modelFilename) - 5);
        strcpy(tempFileName + strlen(modelFilename) - 5, ".mdx");
        file = ri.FileOpen(tempFileName);
        if (!file) {
            fprintf(stderr, "Model not found: %s\n", modelFilename);
            return NULL;
        }
    }
    DWORD fileSize = SFileGetFileSize(file, NULL);
    void *buffer = ri.MemAlloc(fileSize);
    SFileReadFile(file, buffer, fileSize, NULL, NULL);
    switch (*(DWORD *)buffer) {
        case ID_MDLX:
            model = ri.MemAlloc(sizeof(model_t));
            model->mdx = R_LoadModelMDX(buffer, fileSize);
            model->modeltype = ID_MDLX;
//            printf("%s\n", modelFilename);
//            FOR_LOOP(i, model->mdx->num_sequences) {
//                printf("%s\n", model->mdx->sequences[i].name);
//            }
            break;
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (LPSTR)&fileHeader, modelFilename);
            break;
    }
    ri.FileClose(file);
    ri.MemFree(buffer);
    return model;
}

void R_ReleaseModelNode(mdxNode_t *node) {
    SAFE_DELETE(node->translation, ri.MemFree);
    SAFE_DELETE(node->rotation, ri.MemFree);
    SAFE_DELETE(node->scale, ri.MemFree);
}

void R_ReleaseModel(model_t *model) {
    if (model->modeltype == ID_MDLX) {
        MDX_Release(model->mdx);
    }
    ri.MemFree(model);
}

typedef enum {
    BLOCKREAD_OK,
    BLOCKREAD_ERROR,
} blockReadCode_t;

typedef blockReadCode_t (*blockReaderFunc_t)(LPSIZEBUF sb, void *model);

typedef struct {
    LPCSTR block_id;
    blockReaderFunc_t read;
} blockReader_t;

blockReadCode_t MSG_ReadBlock(LPSIZEBUF buffer, blockReader_t const *readers, void *data) {
    DWORD blockHeader;
    while (MSG_Read(buffer, &blockHeader, 4)) {
        struct sizebuf block;
        memset(&block, 0, sizeof(struct sizebuf));
        MSG_Read(buffer, &block.cursize, 4);
        block.data = buffer->data + buffer->readcount;
        for (blockReader_t const *br = readers; br->read; br++) {
            if (*(int const *)br->block_id != blockHeader)
                continue;
            if (br->read(&block, data) != BLOCKREAD_OK)
                return BLOCKREAD_ERROR;
            buffer->readcount += block.readcount;
            goto next_block;
        }
        buffer->readcount += block.cursize;;
//        PrintTag(blockHeader);
    next_block:
        continue;
    }
    return BLOCKREAD_OK;
}

int MSG_Read(LPSIZEBUF buffer, void *dest, DWORD bytes) {
    if (buffer->readcount + bytes > buffer->cursize)
        return 0;
    memcpy(dest, (char *)buffer->data + buffer->readcount, bytes);
    buffer->readcount += bytes;
    return bytes;
}

int MSG_ReadLong(LPSIZEBUF buffer) {
    DWORD value = 0;
    MSG_Read(buffer, &value, 4);
    return value;
}

struct sizebuf FileReadBlock(LPSIZEBUF buffer) {
    struct sizebuf buf;
    buf.data = buffer->data + buffer->readcount;
    buf.readcount = 0;
    buf.cursize = 4;
    buf.cursize = MSG_ReadLong(&buf);
    return buf;
}

int FileIsAtEndOfBlock(LPSIZEBUF sb) {
    return sb->readcount >= sb->cursize;
}

void ReadGeosetMatrices(LPSIZEBUF buffer, mdxGeoset_t *geoset) {
    SFileReadArray2(buffer, geoset, matrices, sizeof(int));
    MSG_Read(buffer, &geoset->materialID, sizeof(int));
    MSG_Read(buffer, &geoset->group, sizeof(int));
    MSG_Read(buffer, &geoset->selectable, sizeof(int));
    MSG_Read(buffer, &geoset->default_bounds, sizeof(mdxBounds_t));
    SFileReadArray2(buffer, geoset, bounds, sizeof(mdxBounds_t));
}

mdxGeoset_t *ReadGeoset(LPSIZEBUF buffer) {
    DWORD header;
    mdxGeoset_t *geoset = ri.MemAlloc(sizeof(mdxGeoset_t));
    while (MSG_Read(buffer, &header, 4)) {
        switch (header) {
            case ID_VRTX: SFileReadArray2(buffer, geoset, vertices, sizeof(mdxVec3_t)); break;
            case ID_NRMS: SFileReadArray2(buffer, geoset, normals, sizeof(mdxVec3_t)); break;
            case ID_UVBS: SFileReadArray2(buffer, geoset, texcoord, sizeof(mdxVec2_t)); break;
            case ID_PTYP: SFileReadArray2(buffer, geoset, primitiveTypes, sizeof(int)); break;
            case ID_PCNT: SFileReadArray2(buffer, geoset, primitiveCounts, sizeof(int)); break;
            case ID_PVTX: SFileReadArray2(buffer, geoset, triangles, sizeof(short)); break;
            case ID_GNDX: SFileReadArray2(buffer, geoset, vertexGroups, sizeof(char)); break;
            case ID_MTGC: SFileReadArray2(buffer, geoset, matrixGroupSizes, sizeof(int)); break;
            case ID_UVAS: MSG_Read(buffer, &geoset->num_texcoordChannels, sizeof(int)); break;
            case ID_MATS: ReadGeosetMatrices(buffer, geoset); break;
            default:
                PrintTag(header);
                break;
        }
    };
    return geoset;
}

void ReadKeyTrack(LPSIZEBUF buffer, MODELKEYTRACKDATATYPE dataType, mdxKeyTrack_t **output) {
    DWORD keyframeCount = MSG_ReadLong(buffer);
    MODELKEYTRACKTYPE keyTrackType = MSG_ReadLong(buffer);
    DWORD globalSeqId = MSG_ReadLong(buffer);
    DWORD const dataSize = GetModelKeyFrameSize(dataType, keyTrackType) * keyframeCount;
    *output = ri.MemAlloc(sizeof(mdxKeyTrack_t) + dataSize);
    (*output)->keyframeCount = keyframeCount;
    (*output)->datatype = dataType;
    (*output)->type = keyTrackType;
    (*output)->globalSeqId = globalSeqId;
    MSG_Read(buffer, (*output)->values, dataSize);
}

void ReadMaterialLayer(LPSIZEBUF buffer, mdxMaterialLayer_t *layer) {
    DWORD blockHeader;
    MSG_Read(buffer, &layer->blendMode, 4);
    MSG_Read(buffer, &layer->flags, 4);
    MSG_Read(buffer, &layer->textureId, 4);
    MSG_Read(buffer, &layer->transformId, 4);
    MSG_Read(buffer, &layer->coordId, 4);
    MSG_Read(buffer, &layer->staticAlpha, 4);
    while (MSG_Read(buffer, &blockHeader, 4)) {
        switch (blockHeader) {
            case ID_KMTA: ReadKeyTrack(buffer, TDATA_FLOAT, &layer->alpha); break;
            case ID_KMTF: ReadKeyTrack(buffer, TDATA_INT, &layer->flipbook); break;
            default:
                PrintTag(blockHeader);
                break;
        }
    }
}

void ReadMaterialLayers(LPSIZEBUF buffer, mdxMaterial_t *material) {
    if (!(material->num_layers = MSG_ReadLong(buffer)))
        return;
    material->layers = ri.MemAlloc(sizeof(mdxMaterialLayer_t) * material->num_layers);
    FOR_LOOP(layerID, material->num_layers) {
        struct sizebuf layer = FileReadBlock(buffer);
        ReadMaterialLayer(&layer, &material->layers[layerID]);
        buffer->readcount += layer.readcount;
    }
}

mdxMaterial_t *ReadMaterial(LPSIZEBUF buffer) {
    DWORD blockHeader;
    mdxMaterial_t *material = ri.MemAlloc(sizeof(mdxMaterial_t));
    material->priority = MSG_ReadLong(buffer);
    material->flags = MSG_ReadLong(buffer);
    while (MSG_Read(buffer, &blockHeader, 4)) {
        switch (blockHeader) {
            case ID_LAYS: ReadMaterialLayers(buffer, material); break;
            case ID_KMTE: ReadKeyTrack(buffer, TDATA_FLOAT, &material->emission); break;
            case ID_KMTA: ReadKeyTrack(buffer, TDATA_FLOAT, &material->alpha); break;
            case ID_KMTF: ReadKeyTrack(buffer, TDATA_INT, &material->flipbook); break;
            default:
                PrintTag(blockHeader);
                return material;
        }
    };
    return material;
}

void ReadNode(LPSIZEBUF buffer, mdxNode_t *node) {
    DWORD blockHeader;
    MSG_Read(buffer, &node->name, sizeof(mdxObjectName_t));
    node->object_id = MSG_ReadLong(buffer);
    node->parent_id = MSG_ReadLong(buffer);
    node->flags = MSG_ReadLong(buffer);
    while (MSG_Read(buffer, &blockHeader, 4)) {
        switch (blockHeader) {
            case ID_KGTR: ReadKeyTrack(buffer, TDATA_VECTOR3, &node->translation); break;
            case ID_KGRT: ReadKeyTrack(buffer, TDATA_QUATERNION, &node->rotation); break;
            case ID_KGSC: ReadKeyTrack(buffer, TDATA_VECTOR3, &node->scale); break;
            default:
                break;
        }
    }
}

mdxBone_t *ReadBone(LPSIZEBUF buffer) {
    mdxBone_t *bone = ri.MemAlloc(sizeof(mdxBone_t));
    ReadNode(buffer, &bone->node);
    buffer->cursize += 8;
    bone->geoset_id = MSG_ReadLong(buffer);
    bone->geoset_animation_id = MSG_ReadLong(buffer);
    return bone;
}

mdxHelper_t *ReadHelper(LPSIZEBUF buffer) {
    mdxHelper_t *helper = ri.MemAlloc(sizeof(mdxHelper_t));
    ReadNode(buffer, &helper->node);
    return helper;
}

mdxCamera_t *ReadCamera(LPSIZEBUF buffer) {
    DWORD blockHeader;
    mdxCamera_t *camera = ri.MemAlloc(sizeof(mdxCamera_t));
    MSG_Read(buffer, &camera->name, sizeof(mdxObjectName_t));
    MSG_Read(buffer, &camera->pivot, sizeof(mdxVec3_t));
    MSG_Read(buffer, &camera->fieldOfView, sizeof(float));
    MSG_Read(buffer, &camera->farClip, sizeof(float));
    MSG_Read(buffer, &camera->nearClip, sizeof(float));
    MSG_Read(buffer, &camera->targetPivot, sizeof(mdxVec3_t));
    while (MSG_Read(buffer, &blockHeader, 4)) {
        switch (blockHeader) {
            case ID_KCTR: ReadKeyTrack(buffer, TDATA_VECTOR3, &camera->translation); break;
            case ID_KTTR: ReadKeyTrack(buffer, TDATA_VECTOR3, &camera->targetTranslation); break;
            case ID_KCRL: ReadKeyTrack(buffer, TDATA_FLOAT, &camera->roll); break;
            default:
                break;
        }
    }
    return camera;
}



mdxEvent_t *ReadEvent(LPSIZEBUF buffer) {
    mdxEvent_t *event = ri.MemAlloc(sizeof(mdxEvent_t));
    ReadNode(buffer, &event->node);
    buffer->cursize += 1000;
    DWORD blockHeader;
    MSG_Read(buffer, &blockHeader, 4);
    if (blockHeader == ID_KEVT) {
        event->num_keys = MSG_ReadLong(buffer);
        event->globalSeqId = MSG_ReadLong(buffer);
        event->keys = ri.MemAlloc(event->num_keys * sizeof(DWORD));
        MSG_Read(buffer, event->keys, event->num_keys * sizeof(DWORD));
        return event;
    } else {
        PrintTag(blockHeader);
        return NULL;
    }
}

mdxGeosetAnim_t *ReadGeosetAnim(LPSIZEBUF buffer) {
    DWORD blockHeader;
    mdxGeosetAnim_t *geosetAnim = ri.MemAlloc(sizeof(mdxGeosetAnim_t));
    MSG_Read(buffer, geosetAnim, 24);
    while (MSG_Read(buffer, &blockHeader, 4)) {
        switch (blockHeader) {
            case ID_KGAO: ReadKeyTrack(buffer, TDATA_FLOAT, &geosetAnim->alphas); break;
            case ID_KGAC: ReadKeyTrack(buffer, TDATA_VECTOR3, &geosetAnim->colors); break;
            default:
                PrintTag(blockHeader);
                return geosetAnim;
        }
    };
    return geosetAnim;
}

mdxNode_t *MDX_GetModelNodeWithObjectID(mdxModel_t *model, DWORD objectID) {
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

blockReadCode_t MDX_ReadMODL(LPSIZEBUF sb, mdxModel_t *model) {
    MSG_Read(sb, &model->info, sizeof(mdxInfo_t));
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadVERS(LPSIZEBUF sb, mdxModel_t *model) {
    if ((model->version = MSG_ReadLong(sb)) != 800) {
        fprintf(stderr, "Usupported MDLX version %d\n", model->version);
        return BLOCKREAD_ERROR;
    } else {
        return BLOCKREAD_OK;
    }
}

blockReadCode_t MDX_ReadEVTS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, Event, events);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadGEOS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, Geoset, geosets);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadMTLS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, Material, materials);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadBONE(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, Bone, bones);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadGEOA(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, GeosetAnim, geosetAnims);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadHELP(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, Helper, helpers);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadCAMS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_LIST(sb, Camera, cameras);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadSEQS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_ARRAY(sb, Sequence, sequences);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadGLBS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_ARRAY(sb, GlobalSequence, globalSequences);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadPIVT(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_ARRAY(sb, Vec3, pivots);
    return BLOCKREAD_OK;
}

blockReadCode_t MDX_ReadTEXS(LPSIZEBUF sb, mdxModel_t *model) {
    MODEL_READ_ARRAY(sb, Texture, textures);
    return BLOCKREAD_OK;
}

blockReader_t R_MDLX[] = {
    { "VERS", (blockReaderFunc_t)MDX_ReadVERS },
    { "MODL", (blockReaderFunc_t)MDX_ReadMODL },
    { "EVTS", (blockReaderFunc_t)MDX_ReadEVTS },
    { "GEOS", (blockReaderFunc_t)MDX_ReadGEOS },
    { "MTLS", (blockReaderFunc_t)MDX_ReadMTLS },
    { "BONE", (blockReaderFunc_t)MDX_ReadBONE },
    { "GEOA", (blockReaderFunc_t)MDX_ReadGEOA },
    { "HELP", (blockReaderFunc_t)MDX_ReadHELP },
    { "CAMS", (blockReaderFunc_t)MDX_ReadCAMS },
    { "SEQS", (blockReaderFunc_t)MDX_ReadSEQS },
    { "GLBS", (blockReaderFunc_t)MDX_ReadGLBS },
    { "PIVT", (blockReaderFunc_t)MDX_ReadPIVT },
    { "TEXS", (blockReaderFunc_t)MDX_ReadTEXS },
    { NULL },
};

mdxModel_t *MDX_LoadBuffer(void *data, DWORD size) {
    mdxModel_t *model = ri.MemAlloc(sizeof(mdxModel_t));
    struct sizebuf buffer = { .data = data, .cursize = size, .readcount = 4 };
    if (MSG_ReadBlock(&buffer, R_MDLX, model) != BLOCKREAD_OK) {
        MDX_Release(model);
        return NULL;
    }
    FOR_EACH_LIST(mdxBone_t, bone, model->bones) {
        model->nodes[bone->node.object_id] = &bone->node;
    }
    FOR_EACH_LIST(mdxHelper_t, helper, model->helpers) {
        model->nodes[helper->node.object_id] = &helper->node;
    }
    FOR_EACH_LIST(mdxGeosetAnim_t, geosetAnim, model->geosetAnims) {
        mdxGeoset_t *geoset = model->geosets;
        for (DWORD geosetID = geosetAnim->geosetId;
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
    SAFE_DELETE(node->translation, ri.MemFree);
    SAFE_DELETE(node->rotation, ri.MemFree);
    SAFE_DELETE(node->scale, ri.MemFree);
}

void MDX_ReleaseModelGeoset(mdxGeoset_t *geoset) {
    SAFE_DELETE(geoset->next, MDX_ReleaseModelGeoset);
    SAFE_DELETE(geoset->userdata, R_ReleaseVertexArrayObject);
    SAFE_DELETE(geoset->vertices, ri.MemFree);
    SAFE_DELETE(geoset->normals, ri.MemFree);
    SAFE_DELETE(geoset->texcoord, ri.MemFree);
    SAFE_DELETE(geoset->matrices, ri.MemFree);
    SAFE_DELETE(geoset->primitiveTypes, ri.MemFree);
    SAFE_DELETE(geoset->primitiveCounts, ri.MemFree);
    SAFE_DELETE(geoset->triangles, ri.MemFree);
    SAFE_DELETE(geoset->vertexGroups, ri.MemFree);
    SAFE_DELETE(geoset->matrixGroupSizes, ri.MemFree);
    SAFE_DELETE(geoset->bounds, ri.MemFree);
    SAFE_DELETE(geoset, ri.MemFree);
}

void MDX_ReleaseModelMaterial(mdxMaterial_t *material) {
    SAFE_DELETE(material->next, MDX_ReleaseModelMaterial);
    SAFE_DELETE(material, ri.MemFree);
}

void MDX_ReleaseModelBone(mdxBone_t *bone) {
    MDX_ReleaseModelNode(&bone->node);
    SAFE_DELETE(bone->next, MDX_ReleaseModelBone);
    SAFE_DELETE(bone, ri.MemFree);
}

void MDX_ReleaseModelGeosetAnim(mdxGeosetAnim_t *geosetAnim) {
    SAFE_DELETE(geosetAnim->next, MDX_ReleaseModelGeosetAnim);
    SAFE_DELETE(geosetAnim->alphas, ri.MemFree);
    SAFE_DELETE(geosetAnim, ri.MemFree);
}

void MDX_ReleaseModelHelper(mdxHelper_t *helper) {
    MDX_ReleaseModelNode(&helper->node);
    SAFE_DELETE(helper->next, MDX_ReleaseModelHelper);
    SAFE_DELETE(helper, ri.MemFree);
}

void MDX_Release(mdxModel_t *model) {
    SAFE_DELETE(model->geosets, MDX_ReleaseModelGeoset);
    SAFE_DELETE(model->materials, MDX_ReleaseModelMaterial);
    SAFE_DELETE(model->bones, MDX_ReleaseModelBone);
    SAFE_DELETE(model->geosetAnims, MDX_ReleaseModelGeosetAnim);
    SAFE_DELETE(model->helpers, MDX_ReleaseModelHelper);
    SAFE_DELETE(model->textures, ri.MemFree);
    SAFE_DELETE(model->sequences, ri.MemFree);
    SAFE_DELETE(model->globalSequences, ri.MemFree);
    SAFE_DELETE(model->pivots, ri.MemFree);
}

