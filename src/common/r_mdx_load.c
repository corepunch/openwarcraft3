#include "r_local.h"
#include "r_mdx.h"

enum {
    ID_MDLX = MAKEFOURCC('M','D','L','X'),
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
//    ID_SNDX = MAKEFOURCC('S','N','D','X'),
    ID_KEVT = MAKEFOURCC('K','E','V','T'),
};

#define MODEL_READ_LIST(FILE, BLOCK, TYPE, TYPES) \
while (!FileIsAtEndOfBlock(FILE, &BLOCK)) { \
    struct tModel##TYPE *lp##TYPE = Read##TYPE(FILE, FileReadBlock(FILE)); \
    if (last##TYPE) { \
        last##TYPE->next = lp##TYPE; \
    } else { \
        model->lp##TYPES = lp##TYPE; \
    } \
    last##TYPE = lp##TYPE; \
}

#define MODEL_READ_ARRAY(FILE, BLOCK, TYPE, TYPES) \
model->lp##TYPES = ri.MemAlloc(BLOCK.size); \
model->num##TYPES = BLOCK.size / sizeof(struct tModel##TYPE); \
SFileReadFile(FILE, model->lp##TYPES, BLOCK.size, NULL, NULL);

struct tFileBlock {
    DWORD header;
    DWORD size;
    DWORD start;
};

DWORD GetModelKeyTrackDataTypeSize(MODELKEYTRACKDATATYPE dataType) {
    switch (dataType) {
        case kModelKeyTrackDataTypeFloat: return 4;
        case kModelKeyTrackDataTypeVector3: return 12;
        case kModelKeyTrackDataTypeQuaternion: return 16;
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

static struct tFileBlock FileReadBlock(HANDLE file) {
    struct tFileBlock block;
    block.header = 0;
    block.start = SFileSetFilePointer(file, 0, NULL, FILE_CURRENT);;
    SFileReadFile(file, &block.size, 4, NULL, NULL);
    return block;
}

static DWORD FileReadInt32(HANDLE file) {
    DWORD value = 0;
    SFileReadFile(file, &value, 4, NULL, NULL);
    return value;
}

static bool FileIsAtEndOfBlock(HANDLE file, struct tFileBlock const *block) {
    DWORD const filePointer = SFileSetFilePointer(file, 0, NULL, FILE_CURRENT);
    DWORD const endOfBlock = block->start + block->size;
    return filePointer >= endOfBlock;
}

static void FileMoveToEndOfBlock(HANDLE file, struct tFileBlock const *block) {
    SFileSetFilePointer(file, block->start + block->size, NULL, FILE_BEGIN);
}

static LPMODELGEOSET ReadGeoset(HANDLE file, struct tFileBlock const block) {
    LPMODELGEOSET geoset = ri.MemAlloc(sizeof(struct tModelGeoset));
    while (!FileIsAtEndOfBlock(file, &block)) {
        int tag = FileReadInt32(file);
        switch (tag) {
            case ID_VRTX: num_\L$1(file, geoset, vertices, sizeof(VECTOR3), ri.MemAlloc); break;
            case ID_NRMS: num_\L$1(file, geoset, normals, sizeof(VECTOR3), ri.MemAlloc); break;
            case ID_UVBS: num_\L$1(file, geoset, texcoord, sizeof(VECTOR2), ri.MemAlloc); break;
            case ID_PTYP: num_\L$1(file, geoset, primitiveTypes, sizeof(int), ri.MemAlloc); break;
            case ID_PCNT: num_\L$1(file, geoset, primitiveCounts, sizeof(int), ri.MemAlloc); break;
            case ID_PVTX: num_\L$1(file, geoset, triangles, sizeof(short), ri.MemAlloc); break;
            case ID_GNDX: num_\L$1(file, geoset, vertexGroups, sizeof(char), ri.MemAlloc); break;
            case ID_MTGC: num_\L$1(file, geoset, matrixGroupSizes, sizeof(int), ri.MemAlloc); break;
            case ID_UVAS: SFileReadFile(file, &geoset->num_texcoordChannels, sizeof(int), NULL, NULL); break;
            case ID_MATS:
                num_\L$1(file, geoset, matrices, sizeof(int), ri.MemAlloc);
                SFileReadFile(file, &geoset->materialID, sizeof(int), NULL, NULL);
                SFileReadFile(file, &geoset->group, sizeof(int), NULL, NULL);
                SFileReadFile(file, &geoset->selectable, sizeof(int), NULL, NULL);
                SFileReadFile(file, &geoset->default_bounds, sizeof(struct tModelBounds), NULL, NULL);
                num_\L$1(file, geoset, bounds, sizeof(struct tModelBounds), ri.MemAlloc);
                break;
            default:

                break;
        }
    };
    return geoset;
}

static void ReadMaterialLayer(HANDLE file,
                              LPMODELLAYER layer,
                              struct tFileBlock const block)
{
    layer->blendMode = FileReadInt32(file);
    layer->flags = FileReadInt32(file);
    layer->textureId = FileReadInt32(file);
    layer->transformId = FileReadInt32(file);
    layer->coordId = FileReadInt32(file);
    layer->staticAlpha = FileReadInt32(file);
    FileMoveToEndOfBlock(file, &block);
}

static LPMODELMATERIAL ReadMaterial(HANDLE file, struct tFileBlock const block) {
    LPMODELMATERIAL material = ri.MemAlloc(sizeof(struct tModelMaterial));
    material->priority = FileReadInt32(file);
    material->flags = FileReadInt32(file);
    while (!FileIsAtEndOfBlock(file, &block)) {
        DWORD const blockHeader = FileReadInt32(file);
        switch (blockHeader) {
            case ID_LAYS:
                SFileReadFile(file, &material->num_layers, 4, NULL, NULL);
                if (material->num_layers > 0) {
                    material->layers = ri.MemAlloc(sizeof(struct tModelMaterialLayer) * material->num_layers);
                    FOR_LOOP(layerID, material->num_layers) {
                        ReadMaterialLayer(file, &material->layers[layerID], FileReadBlock(file));
                    }
                }
                break;
            default:
//                PrintTag(blockHeader);
                FileMoveToEndOfBlock(file, &block);
                return material;
        }
    };
    return material;
}

static HANDLE ReadModelKeyTrack(HANDLE file, MODELKEYTRACKDATATYPE dataType) {
    DWORD keyframeCount = FileReadInt32(file);
    MODELKEYTRACKTYPE keyTrackType = FileReadInt32(file);
    DWORD globalSeqId = FileReadInt32(file);
    DWORD const dataSize = GetModelKeyFrameSize(dataType, keyTrackType) * keyframeCount;
    LPMODELKEYTRACK keytrack = ri.MemAlloc(MODELTRACKINFOSIZE + dataSize);
    keytrack->keyframeCount = keyframeCount;
    keytrack->datatype = dataType;
    keytrack->type = keyTrackType;
    keytrack->globalSeqId = globalSeqId;
    SFileReadFile(file, keytrack->values, dataSize, NULL, NULL);
    return keytrack;
}

static void ReadNode(HANDLE file, struct tFileBlock const block, LPMODELNODE node) {
    SFileReadFile(file, &node->name, sizeof(tModelObjectName), NULL, NULL);
    node->objectId = FileReadInt32(file);
    node->parentId = FileReadInt32(file);
    node->flags = FileReadInt32(file);
    while (!FileIsAtEndOfBlock(file, &block)) {
        DWORD const blockHeader = FileReadInt32(file);
        switch (blockHeader) {
            case ID_KGTR:
                node->translation = ReadModelKeyTrack(file, kModelKeyTrackDataTypeVector3);
                break;
            case ID_KGRT:
                node->rotation = ReadModelKeyTrack(file, kModelKeyTrackDataTypeQuaternion);
                break;
            case ID_KGSC:
                node->scale = ReadModelKeyTrack(file, kModelKeyTrackDataTypeVector3);
                break;
        }
    }
}

static struct tModelBone *ReadBone(HANDLE file, struct tFileBlock const block) {
    struct tModelBone *bone = ri.MemAlloc(sizeof(struct tModelBone));
    ReadNode(file, block, &bone->node);
    bone->geoset_id = FileReadInt32(file);
    bone->geoset_animation_id = FileReadInt32(file);
    return bone;
}

static struct tModelHelper *ReadHelper(HANDLE file, struct tFileBlock const block) {
    struct tModelHelper *helper = ri.MemAlloc(sizeof(struct tModelHelper));
    ReadNode(file, block, &helper->node);
    return helper;
}


static struct tModelEvent *ReadEvent(HANDLE file, struct tFileBlock const block) {
    struct tModelEvent *event = ri.MemAlloc(sizeof(struct tModelEvent));
    ReadNode(file, block, &event->node);
    DWORD header = FileReadInt32(file);
    switch (header) {
        case ID_KEVT:
            event->num_keys = FileReadInt32(file);
            event->globalSeqId = FileReadInt32(file);
            event->keys = ri.MemAlloc(event->num_keys * sizeof(DWORD));
            SFileReadFile(file, event->keys, event->num_keys * sizeof(DWORD), NULL, NULL);
            break;
    }
    return event;
}

static struct tModelGeosetAnim *ReadGeosetAnim(HANDLE file, struct tFileBlock const block) {
    struct tModelGeosetAnim *geosetAnim = ri.MemAlloc(sizeof(struct tModelGeosetAnim));
    SFileReadFile(file, geosetAnim, 24, NULL, NULL);
    while (!FileIsAtEndOfBlock(file, &block)) {
        DWORD const blockHeader = FileReadInt32(file);
        switch (blockHeader) {
            case ID_KGAO:
                geosetAnim->alphas = ReadModelKeyTrack(file, kModelKeyTrackDataTypeFloat);
                break;
            default:
//                PrintTag(blockHeader);
                FileMoveToEndOfBlock(file, &block);
                return geosetAnim;
        }
    };
    return geosetAnim;
}

static DWORD R_ModelFindBiggestGroup(LPCMODELGEOSET geoset) {
    DWORD biggest = 0;
    FOR_LOOP(i, geoset->num_matrixGroupSizes) {
        biggest = MAX(geoset->matrixGroupSizes[i], biggest);
    }
    return biggest;
}

static void R_SetupGeoset(LPMODEL model, LPMODELGEOSET geoset, LPCSTR modelFilename) {
    DWORD biggestGeoset = R_ModelFindBiggestGroup(geoset);
    if (biggestGeoset > 4) {
        ri.error("Geoset with more that 4 bones skinning int %s\n", modelFilename);
        biggestGeoset = 4;
    }

    typedef BYTE matrixGroup_t[4];
    struct vertex *vertices = ri.MemAlloc(sizeof(struct vertex) * geoset->num_triangles);
    matrixGroup_t *matrixGroups = ri.MemAlloc(sizeof(matrixGroup_t) * geoset->num_matrixGroupSizes);
    DWORD indexOffset = 0;

    FOR_LOOP(matrixGroupIndex, geoset->num_matrixGroupSizes) {
        FOR_LOOP(matrixIndex, geoset->matrixGroupSizes[matrixGroupIndex]) {
            matrixGroups[matrixGroupIndex][matrixIndex] = geoset->matrices[indexOffset++] + 1;
        }
    }

    FOR_LOOP(triangle, geoset->num_triangles) {
        DWORD vertex = geoset->triangles[triangle];
        DWORD matrixGroupIndex = geoset->vertexGroups[vertex];
        DWORD matrixGroupSize = MAX(1, geoset->matrixGroupSizes[matrixGroupIndex]);
        struct vertex *vertex = &vertices[triangle];
        BYTE *matrixGroup = matrixGroups[matrixGroupIndex];
        vertex->color = (struct color32) { 255, 255, 255, 255 };
        if (geoset->vertices) vertex->position = geoset->vertices[vertex];
        if (geoset->texcoord) vertex->texcoord = geoset->texcoord[vertex];
        if (geoset->normals) vertex->normal = geoset->normals[vertex];
        memcpy(vertex->skin, matrixGroup, sizeof(matrixGroup_t));
        memset(vertex->boneWeight, 0, sizeof(matrixGroup_t));
        FOR_LOOP(matrixIndex, matrixGroupSize) {
            vertex->boneWeight[matrixIndex] = (1.f / matrixGroupSize) * 255;
        }
    }

    geoset->buffer = R_MakeVertexArrayObject(vertices, geoset->num_triangles * sizeof(struct vertex));

    ri.MemFree(vertices);
    ri.MemFree(matrixGroups);
}

static LPMODELNODE R_GetModelNodeWithObjectID(LPMODEL model, DWORD objectID) {
    if (objectID == -1) {
        return NULL;
    }
    FOR_EACH_LIST(struct tModelBone, bone, model->bones) {
        if (bone->node.objectId == objectID) {
            return &bone->node;
        }
    }
    FOR_EACH_LIST(struct tModelHelper, helper, model->helpers) {
        if (helper->node.objectId == objectID) {
            return &helper->node;
        }
    }
    return NULL;
}

LPMODEL R_LoadModelMDX(HANDLE file, LPCSTR modelFilename) {
    DWORD blockHeader, fileVersion = 0;
    LPMODEL model = ri.MemAlloc(sizeof(struct tModel));
    LPMODELGEOSET lastGeoset = model->geosets;
    LPMODELMATERIAL lastMaterial = model->materials;
    struct tModelBone *lastBone = model->bones;
    struct tModelHelper *lastHelper = model->helpers;
    struct tModelGeosetAnim *lastGeosetAnim = model->geosetAnims;
    struct tModelEvent *lastEvent = model->events;
    struct tFileBlock block;
    while (SFileReadFile(file, &blockHeader, 4, NULL, NULL)) {
        SFileReadFile(file, &block.size, 4, NULL, NULL);
        block.header = 0;
        block.start = SFileSetFilePointer(file, 0, NULL, FILE_CURRENT);;
        switch (blockHeader) {
            case ID_VERS:
                if ((fileVersion = FileReadInt32(file)) != 800) {
                    fprintf(stderr, "Usupported MDLX version %d\n", fileVersion);
                    return NULL;
                }
                break;
            case ID_EVTS: MODEL_READ_LIST(file, block, Event, Events); break;
            case ID_MODL: SFileReadFile(file, &model->info, sizeof(struct tModelInfo), NULL, NULL); break;
            case ID_GEOS: MODEL_READ_LIST(file, block, Geoset, Geosets); break;
            case ID_MTLS: MODEL_READ_LIST(file, block, Material, Materials); break;
            case ID_BONE: MODEL_READ_LIST(file, block, Bone, Bones); break;
            case ID_GEOA: MODEL_READ_LIST(file, block, GeosetAnim, GeosetAnims); break;
            case ID_HELP: MODEL_READ_LIST(file, block, Helper, Helpers); break;
            case ID_SEQS: MODEL_READ_ARRAY(file, block, Sequence, Sequences); break;
            case ID_GLBS: MODEL_READ_ARRAY(file, block, GlobalSequence, GlobalSequences); break;
            case ID_PIVT: MODEL_READ_ARRAY(file, block, Pivot, Pivots); break;
            case ID_TEXS: MODEL_READ_ARRAY(file, block, Texture, Textures); break;
            default:
//                PrintTag(blockHeader);
                SFileSetFilePointer(file, block.size, NULL, FILE_CURRENT);
                break;
        }
    }
    FOR_EACH_LIST(struct tModelGeoset, geoset, model->geosets) {
        R_SetupGeoset(model, geoset, modelFilename);
    }
    FOR_EACH_LIST(struct tModelBone, bone, model->bones) {
        bone->node.parent = R_GetModelNodeWithObjectID(model, bone->node.parentId);
        bone->node.pivot = &model->pivots[bone->node.objectId];
    }
    FOR_EACH_LIST(struct tModelHelper, helper, model->helpers) {
        helper->node.parent = R_GetModelNodeWithObjectID(model, helper->node.parentId);
        helper->node.pivot = &model->pivots[helper->node.objectId];
    }
    FOR_LOOP(texid, model->num_textures) {
        model->textures[texid].texid = R_RegisterTextureFile(model->textures[texid].path);
    }
    FOR_EACH_LIST(struct tModelGeosetAnim, geosetAnim, model->geosetAnims) {
        LPMODELGEOSET geoset = model->geosets;
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
//    LPCMODELSEQUENCE R_FindSequenceAtTime(LPCMODEL model, DWORD time);
//    FOR_EACH_LIST(struct tModelEvent, event, model->events) {
//        printf("%s %d %d %s\n", event->node.name, event->keys[0], event->num_keys, R_FindSequenceAtTime(model, event->keys[0])->name);
//    }
    ri.FileClose(file);
    model->currentAnimation = &model->sequences[1];
    return model;}

LPMODEL R_LoadModel(LPCSTR modelFilename) {
    DWORD fileHeader;
    HANDLE file = ri.FileOpen(modelFilename);
    LPMODEL model = NULL;
//    printf("%s\n", modelFilename);
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
    SFileReadFile(file, &fileHeader, 4, NULL, NULL);
    switch (fileHeader) {
        case ID_MDLX:
            model = R_LoadModelMDX(file, modelFilename);
            break;
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (LPSTR)&fileHeader, modelFilename);
            break;
    }
    ri.FileClose(file);
    return model;
}

static void R_ReleaseModelNode(LPMODELNODE node) {
    SAFE_DELETE(node->translation, ri.MemFree);
    SAFE_DELETE(node->rotation, ri.MemFree);
    SAFE_DELETE(node->scale, ri.MemFree);
}
static void R_ReleaseModelGeoset(LPMODELGEOSET geoset) {
    SAFE_DELETE(geoset->next, R_ReleaseModelGeoset);
    SAFE_DELETE(geoset->buffer, R_ReleaseVertexArrayObject);
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
static void R_ReleaseModelMaterial(LPMODELMATERIAL material) {
    SAFE_DELETE(material->next, R_ReleaseModelMaterial);
    SAFE_DELETE(material, ri.MemFree);
}
static void R_ReleaseModelBone(struct tModelBone *bone) {
    R_ReleaseModelNode(&bone->node);
    SAFE_DELETE(bone->next, R_ReleaseModelBone);
    SAFE_DELETE(bone, ri.MemFree);
}
static void R_ReleaseModelGeosetAnim(struct tModelGeosetAnim *geosetAnim) {
    SAFE_DELETE(geosetAnim->next, R_ReleaseModelGeosetAnim);
    SAFE_DELETE(geosetAnim->alphas, ri.MemFree);
    SAFE_DELETE(geosetAnim, ri.MemFree);
}
static void R_ReleaseModelHelper(struct tModelHelper *helper) {
    R_ReleaseModelNode(&helper->node);
    SAFE_DELETE(helper->next, R_ReleaseModelHelper);
    SAFE_DELETE(helper, ri.MemFree);
}
void R_ReleaseModel(LPMODEL model) {
    SAFE_DELETE(model->geosets, R_ReleaseModelGeoset);
    SAFE_DELETE(model->materials, R_ReleaseModelMaterial);
    SAFE_DELETE(model->bones, R_ReleaseModelBone);
    SAFE_DELETE(model->geosetAnims, R_ReleaseModelGeosetAnim);
    SAFE_DELETE(model->helpers, R_ReleaseModelHelper);
    SAFE_DELETE(model->textures, ri.MemFree);
    SAFE_DELETE(model->sequences, ri.MemFree);
    SAFE_DELETE(model->globalSequences, ri.MemFree);
    SAFE_DELETE(model->pivots, ri.MemFree);
}
