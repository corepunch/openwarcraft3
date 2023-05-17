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
};

#define MODEL_READ_LIST(FILE, BLOCK, TYPE, TYPES) \
while (!FileIsAtEndOfBlock(FILE, &BLOCK)) { \
    struct tModel##TYPE *lp##TYPE = Read##TYPE(FILE, FileReadBlock(FILE)); \
    if (lpLast##TYPE) { \
        lpLast##TYPE->lpNext = lp##TYPE; \
    } else { \
        lpModel->lp##TYPES = lp##TYPE; \
    } \
    lpLast##TYPE = lp##TYPE; \
}

#define MODEL_READ_ARRAY(FILE, BLOCK, TYPE, TYPES) \
lpModel->lp##TYPES = ri.MemAlloc(BLOCK.dwSize); \
lpModel->num##TYPES = BLOCK.dwSize / sizeof(struct tModel##TYPE); \
SFileReadFile(FILE, lpModel->lp##TYPES, BLOCK.dwSize, NULL, NULL);

struct tFileBlock {
    DWORD dwHeader;
    DWORD dwSize;
    DWORD dwStart;
};

DWORD GetModelKeyTrackDataTypeSize(enum tModelKeyTrackDataType dwDataType) {
    switch (dwDataType) {
        case kModelKeyTrackDataTypeFloat: return 4;
        case kModelKeyTrackDataTypeVector3: return 12;
        case kModelKeyTrackDataTypeQuaternion: return 16;
        default: return 0;
    }
}

DWORD GetModelKeyTrackTypeSize(enum tModelKeyTrackType dwKeyTrackType) {
    switch (dwKeyTrackType) {
        case TRACK_NO_INTERP: return 1;
        case TRACK_LINEAR: return 1;
        case TRACK_HERMITE: return 3;
        case TRACK_BEZIER: return 3;
        default: return 0;
    }
}

DWORD GetModelKeyFrameSize(enum tModelKeyTrackDataType dwDataType,
                           enum tModelKeyTrackType dwKeyTrackType)
{
    return 4 + GetModelKeyTrackDataTypeSize(dwDataType) * GetModelKeyTrackTypeSize(dwKeyTrackType);
}

static struct tFileBlock FileReadBlock(HANDLE hFile) {
    struct tFileBlock block;
    block.dwHeader = 0;
    block.dwStart = SFileSetFilePointer(hFile, 0, NULL, FILE_CURRENT);;
    SFileReadFile(hFile, &block.dwSize, 4, NULL, NULL);
    return block;
}

static DWORD FileReadInt32(HANDLE hFile) {
    DWORD dwValue = 0;
    SFileReadFile(hFile, &dwValue, 4, NULL, NULL);
    return dwValue;
}

static bool FileIsAtEndOfBlock(HANDLE hFile, struct tFileBlock const *lpBlock) {
    DWORD const dwFilePointer = SFileSetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    DWORD const dwEndOfBlock = lpBlock->dwStart + lpBlock->dwSize;
    return dwFilePointer >= dwEndOfBlock;
}

static void FileMoveToEndOfBlock(HANDLE hFile, struct tFileBlock const *lpBlock) {
    SFileSetFilePointer(hFile, lpBlock->dwStart + lpBlock->dwSize, NULL, FILE_BEGIN);
}

static struct tModelGeoset *ReadGeoset(HANDLE hFile, struct tFileBlock const block) {
    struct tModelGeoset *lpGeoset = ri.MemAlloc(sizeof(struct tModelGeoset));
    while (!FileIsAtEndOfBlock(hFile, &block)) {
        int tag = FileReadInt32(hFile);
        switch (tag) {
            case ID_VRTX: SFileReadArray(hFile, lpGeoset, Vertices, sizeof(struct vector3)); break;
            case ID_NRMS: SFileReadArray(hFile, lpGeoset, Normals, sizeof(struct vector3)); break;
            case ID_UVBS: SFileReadArray(hFile, lpGeoset, Texcoord, sizeof(struct vector2)); break;
            case ID_PTYP: SFileReadArray(hFile, lpGeoset, PrimitiveTypes, sizeof(int)); break;
            case ID_PCNT: SFileReadArray(hFile, lpGeoset, PrimitiveCounts, sizeof(int)); break;
            case ID_PVTX: SFileReadArray(hFile, lpGeoset, Triangles, sizeof(short)); break;
            case ID_GNDX: SFileReadArray(hFile, lpGeoset, VertexGroups, sizeof(char)); break;
            case ID_MTGC: SFileReadArray(hFile, lpGeoset, MatrixGroupSizes, sizeof(int)); break;
            case ID_UVAS: SFileReadFile(hFile, &lpGeoset->numTexcoordChannels, sizeof(int), NULL, NULL); break;
            case ID_MATS:
                SFileReadArray(hFile, lpGeoset, Matrices, sizeof(int));
                SFileReadFile(hFile, &lpGeoset->material, sizeof(int), NULL, NULL);
                SFileReadFile(hFile, &lpGeoset->group, sizeof(int), NULL, NULL);
                SFileReadFile(hFile, &lpGeoset->selectable, sizeof(int), NULL, NULL);
                SFileReadFile(hFile, &lpGeoset->default_bounds, sizeof(struct tModelBounds), NULL, NULL);
                SFileReadArray(hFile, lpGeoset, Bounds, sizeof(struct tModelBounds));
                break;
            default:
                
                break;
        }
    };
    return lpGeoset;
}

static void ReadMaterialLayer(HANDLE hFile,
                              struct tModelMaterialLayer *lpLayer,
                              struct tFileBlock const block)
{
    lpLayer->blendMode = FileReadInt32(hFile);
    lpLayer->flags = FileReadInt32(hFile);
    lpLayer->textureId = FileReadInt32(hFile);
    lpLayer->transformId = FileReadInt32(hFile);
    lpLayer->coordId = FileReadInt32(hFile);
    lpLayer->staticAlpha = FileReadInt32(hFile);
    FileMoveToEndOfBlock(hFile, &block);
}

static struct tModelMaterial *ReadMaterial(HANDLE hFile, struct tFileBlock const block) {
    struct tModelMaterial *lpMaterial = ri.MemAlloc(sizeof(struct tModelMaterial));
    lpMaterial->priority = FileReadInt32(hFile);
    lpMaterial->flags = FileReadInt32(hFile);
    while (!FileIsAtEndOfBlock(hFile, &block)) {
        DWORD const dwBlockHeader = FileReadInt32(hFile);
        switch (dwBlockHeader) {
            case ID_LAYS:
                SFileReadFile(hFile, &lpMaterial->num_layers, 4, NULL, NULL);
                if (lpMaterial->num_layers > 0) {
                    lpMaterial->layers = ri.MemAlloc(sizeof(struct tModelMaterialLayer) * lpMaterial->num_layers);
                    FOR_LOOP(dwLayerID, lpMaterial->num_layers) {
                        ReadMaterialLayer(hFile, &lpMaterial->layers[dwLayerID], FileReadBlock(hFile));
                    }
                }
                break;
            default:
//                PrintTag(dwBlockHeader);
                FileMoveToEndOfBlock(hFile, &block);
                return lpMaterial;
        }
    };
    return lpMaterial;
}

static void *ReadModelKeyTrack(HANDLE hFile, enum tModelKeyTrackDataType dwDataType) {
    DWORD dwKeyframeCount = FileReadInt32(hFile);
    enum tModelKeyTrackType dwKeyTrackType = FileReadInt32(hFile);
    DWORD dwGlobalSeqId = FileReadInt32(hFile);
    DWORD const dwDataSize = GetModelKeyFrameSize(dwDataType, dwKeyTrackType) * dwKeyframeCount;
    struct tModelKeyTrack *lpKeytrack = ri.MemAlloc(MODELTRACKINFOSIZE + dwDataSize);
    lpKeytrack->dwKeyframeCount = dwKeyframeCount;
    lpKeytrack->datatype = dwDataType;
    lpKeytrack->type = dwKeyTrackType;
    lpKeytrack->globalSeqId = dwGlobalSeqId;
    SFileReadFile(hFile, lpKeytrack->values, dwDataSize, NULL, NULL);
    return lpKeytrack;
}

static void ReadNode(HANDLE hFile, struct tFileBlock const block, struct tModelNode *lpNode) {
    SFileReadFile(hFile, &lpNode->name, sizeof(tModelObjectName), NULL, NULL);
    lpNode->objectId = FileReadInt32(hFile);
    lpNode->parentId = FileReadInt32(hFile);
    lpNode->flags = FileReadInt32(hFile);
    while (!FileIsAtEndOfBlock(hFile, &block)) {
        DWORD const dwBlockHeader = FileReadInt32(hFile);
        switch (dwBlockHeader) {
            case ID_KGTR:
                lpNode->lpTranslation = ReadModelKeyTrack(hFile, kModelKeyTrackDataTypeVector3);
                break;
            case ID_KGRT:
                lpNode->lpRotation = ReadModelKeyTrack(hFile, kModelKeyTrackDataTypeQuaternion);
                break;
            case ID_KGSC:
                lpNode->lpScale = ReadModelKeyTrack(hFile, kModelKeyTrackDataTypeVector3);
                break;
        }
    }
}

static struct tModelBone *ReadBone(HANDLE hFile, struct tFileBlock const block) {
    struct tModelBone *lpBone = ri.MemAlloc(sizeof(struct tModelBone));
    ReadNode(hFile, block, &lpBone->node);
    lpBone->geoset_id = FileReadInt32(hFile);
    lpBone->geoset_animation_id = FileReadInt32(hFile);
    return lpBone;
}

static struct tModelHelper *ReadHelper(HANDLE hFile, struct tFileBlock const block) {
    struct tModelHelper *lpHelper = ri.MemAlloc(sizeof(struct tModelHelper));
    ReadNode(hFile, block, &lpHelper->node);
    return lpHelper;
}

static struct tModelGeosetAnim *ReadGeosetAnim(HANDLE hFile, struct tFileBlock const block) {
    struct tModelGeosetAnim *lpGeosetAnim = ri.MemAlloc(sizeof(struct tModelGeosetAnim));
    SFileReadFile(hFile, lpGeosetAnim, 24, NULL, NULL);
    while (!FileIsAtEndOfBlock(hFile, &block)) {
        DWORD const dwBlockHeader = FileReadInt32(hFile);
        switch (dwBlockHeader) {
            case ID_KGAO:
                lpGeosetAnim->lpAlphas = ReadModelKeyTrack(hFile, kModelKeyTrackDataTypeFloat);
                break;
            default:
//                PrintTag(dwBlockHeader);
                FileMoveToEndOfBlock(hFile, &block);
                return lpGeosetAnim;
        }
    };
    return lpGeosetAnim;
}

static DWORD R_ModelFindBiggestGroup(struct tModelGeoset const *lpGeoset) {
    DWORD dwBiggest = 0;
    FOR_LOOP(i, lpGeoset->numMatrixGroupSizes) {
        dwBiggest = MAX(lpGeoset->lpMatrixGroupSizes[i], dwBiggest);
    }
    return dwBiggest;
}

static void R_SetupGeoset(LPMODEL lpModel, struct tModelGeoset *lpGeoset) {
    DWORD dwBiggestGeoset = R_ModelFindBiggestGroup(lpGeoset);
    if (dwBiggestGeoset > 4) {
        fprintf(stderr, "Geosets with more that 4 bones skinning are not supported\n");
        return;
    }
    
    typedef uint8_t matrixGroup_t[4];
    struct vertex *lpVertices = ri.MemAlloc(sizeof(struct vertex) * lpGeoset->numTriangles);
    matrixGroup_t *lpMatrixGroups = ri.MemAlloc(sizeof(matrixGroup_t) * lpGeoset->numMatrixGroupSizes);
    DWORD dwIndexOffset = 0;
    
    FOR_LOOP(dwMatrixGroupIndex, lpGeoset->numMatrixGroupSizes) {
        FOR_LOOP(dwMatrixIndex, lpGeoset->lpMatrixGroupSizes[dwMatrixGroupIndex]) {
            lpMatrixGroups[dwMatrixGroupIndex][dwMatrixIndex] = lpGeoset->lpMatrices[dwIndexOffset++] + 1;
        }
    }

    FOR_LOOP(dwTriangle, lpGeoset->numTriangles) {
        int dwVertex = lpGeoset->lpTriangles[dwTriangle];
        int dwMatrixGroupIndex = lpGeoset->lpVertexGroups[dwVertex];
        int dwMatrixGroupSize = MAX(1, lpGeoset->lpMatrixGroupSizes[dwMatrixGroupIndex]);
        struct vertex *lpVertex = &lpVertices[dwTriangle];
        uint8_t *dwMatrixGroup = lpMatrixGroups[dwMatrixGroupIndex];
        lpVertex->color = (struct color32) { 255, 255, 255, 255 };
        lpVertex->position = lpGeoset->lpVertices[dwVertex];
        lpVertex->texcoord = lpGeoset->lpTexcoord[dwVertex];
        memcpy(lpVertex->skin, dwMatrixGroup, sizeof(matrixGroup_t));
        memset(lpVertex->boneWeight, 0, sizeof(matrixGroup_t));
        FOR_LOOP(dwMatrixIndex, dwMatrixGroupSize) {
            lpVertex->boneWeight[dwMatrixIndex] = (1.f / dwMatrixGroupSize) * 255;
        }
    }

    lpGeoset->lpBuffer = R_MakeVertexArrayObject(lpVertices, lpGeoset->numTriangles * sizeof(struct vertex));
    
    ri.MemFree(lpVertices);
    ri.MemFree(lpMatrixGroups);
}

static struct tModelNode *R_GetModelNodeWithObjectID(LPMODEL lpModel, DWORD dwObjectID) {
    if (dwObjectID == -1) {
        return NULL;
    }
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        if (lpBone->node.objectId == dwObjectID) {
            return &lpBone->node;
        }
    }
    FOR_EACH_LIST(struct tModelHelper, lpHelper, lpModel->lpHelpers) {
        if (lpHelper->node.objectId == dwObjectID) {
            return &lpHelper->node;
        }
    }
    return NULL;
}

LPMODEL R_LoadModelMDX(HANDLE hFile) {
    DWORD dwBlockHeader, dwFileVersion = 0;
    LPMODEL lpModel = ri.MemAlloc(sizeof(struct tModel));
    struct tModelGeoset *lpLastGeoset = lpModel->lpGeosets;
    struct tModelMaterial *lpLastMaterial = lpModel->lpMaterials;
    struct tModelBone *lpLastBone = lpModel->lpBones;
    struct tModelHelper *lpLastHelper = lpModel->lpHelpers;
    struct tModelGeosetAnim *lpLastGeosetAnim = lpModel->lpGeosetAnims;
    struct tFileBlock block;
    while (SFileReadFile(hFile, &dwBlockHeader, 4, NULL, NULL)) {
        SFileReadFile(hFile, &block.dwSize, 4, NULL, NULL);
        block.dwHeader = 0;
        block.dwStart = SFileSetFilePointer(hFile, 0, NULL, FILE_CURRENT);;
        switch (dwBlockHeader) {
            case ID_VERS:
                if ((dwFileVersion = FileReadInt32(hFile)) != 800) {
                    fprintf(stderr, "Usupported MDLX version %d\n", dwFileVersion);
                    return NULL;
                }
                break;
            case ID_MODL: SFileReadFile(hFile, &lpModel->info, sizeof(struct tModelInfo), NULL, NULL); break;
            case ID_GEOS: MODEL_READ_LIST(hFile, block, Geoset, Geosets); break;
            case ID_MTLS: MODEL_READ_LIST(hFile, block, Material, Materials); break;
            case ID_BONE: MODEL_READ_LIST(hFile, block, Bone, Bones); break;
            case ID_GEOA: MODEL_READ_LIST(hFile, block, GeosetAnim, GeosetAnims); break;
            case ID_HELP: MODEL_READ_LIST(hFile, block, Helper, Helpers); break;
            case ID_SEQS: MODEL_READ_ARRAY(hFile, block, Sequence, Sequences); break;
            case ID_GLBS: MODEL_READ_ARRAY(hFile, block, GlobalSequence, GlobalSequences); break;
            case ID_PIVT: MODEL_READ_ARRAY(hFile, block, Pivot, Pivots); break;
            case ID_TEXS: MODEL_READ_ARRAY(hFile, block, Texture, Textures); break;
            default:
//                PrintTag(dwBlockHeader);
                SFileSetFilePointer(hFile, block.dwSize, NULL, FILE_CURRENT);
                break;
        }
    }
    FOR_EACH_LIST(struct tModelGeoset, lpGeoset, lpModel->lpGeosets) {
        R_SetupGeoset(lpModel, lpGeoset);
    }
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        lpBone->node.lpParent = R_GetModelNodeWithObjectID(lpModel, lpBone->node.parentId);
        lpBone->node.lpPivot = &lpModel->lpPivots[lpBone->node.objectId];
    }
    FOR_EACH_LIST(struct tModelHelper, lpHelper, lpModel->lpHelpers) {
        lpHelper->node.lpParent = R_GetModelNodeWithObjectID(lpModel, lpHelper->node.parentId);
        lpHelper->node.lpPivot = &lpModel->lpPivots[lpHelper->node.objectId];
    }
    FOR_LOOP(texid, lpModel->numTextures) {
        lpModel->lpTextures[texid].texid = R_RegisterTextureFile(lpModel->lpTextures[texid].path);
    }
    FOR_EACH_LIST(struct tModelGeosetAnim, lpGeosetAnim, lpModel->lpGeosetAnims) {
        struct tModelGeoset *lpGeoset = lpModel->lpGeosets;
        for (DWORD dwGeosetID = lpGeosetAnim->geosetId;
             lpGeoset && dwGeosetID > 0;
             dwGeosetID--)
        {
            lpGeoset = lpGeoset->lpNext;
        }
        if (lpGeoset) {
            lpGeoset->lpGeosetAnim = lpGeosetAnim;
        }
    }
    ri.FileClose(hFile);
    lpModel->lpCurrentAnimation = &lpModel->lpSequences[1];
    return lpModel;}

LPMODEL R_LoadModel(LPCSTR szModelFilename) {
    DWORD dwFileHeader;
    HANDLE hFile = ri.FileOpen(szModelFilename);
    LPMODEL model = NULL;
//    printf("%s\n", szModelFilename);
    if (hFile == NULL) {
        // try to load without *0.mdx
        PATHSTR szTempFileName;
        strncpy(szTempFileName, szModelFilename, strlen(szModelFilename) - 5);
        strcpy(szTempFileName + strlen(szModelFilename) - 5, ".mdx");
        hFile = ri.FileOpen(szTempFileName);
        if (!hFile) {
            fprintf(stderr, "Model not found: %s\n", szModelFilename);
            return NULL;
        }
    }
    SFileReadFile(hFile, &dwFileHeader, 4, NULL, NULL);
    switch (dwFileHeader) {
        case ID_MDLX:
            model = R_LoadModelMDX(hFile);
            break;
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (char *)&dwFileHeader, szModelFilename);
            break;
    }
    ri.FileClose(hFile);
    return model;
}

static void R_ReleaseModelNode(struct tModelNode *lpNode) {
    SAFE_DELETE(lpNode->lpTranslation, ri.MemFree);
    SAFE_DELETE(lpNode->lpRotation, ri.MemFree);
    SAFE_DELETE(lpNode->lpScale, ri.MemFree);
}
static void R_ReleaseModelGeoset(struct tModelGeoset *lpGeoset) {
    SAFE_DELETE(lpGeoset->lpNext, R_ReleaseModelGeoset);
    SAFE_DELETE(lpGeoset->lpBuffer, R_ReleaseVertexArrayObject);
    SAFE_DELETE(lpGeoset->lpVertices, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpNormals, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpTexcoord, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpMatrices, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpPrimitiveTypes, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpPrimitiveCounts, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpTriangles, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpVertexGroups, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpMatrixGroupSizes, ri.MemFree);
    SAFE_DELETE(lpGeoset->lpBounds, ri.MemFree);
    SAFE_DELETE(lpGeoset, ri.MemFree);
}
static void R_ReleaseModelMaterial(struct tModelMaterial *lpMaterial) {
    SAFE_DELETE(lpMaterial->lpNext, R_ReleaseModelMaterial);
    SAFE_DELETE(lpMaterial, ri.MemFree);
}
static void R_ReleaseModelBone(struct tModelBone *lpBone) {
    R_ReleaseModelNode(&lpBone->node);
    SAFE_DELETE(lpBone->lpNext, R_ReleaseModelBone);
    SAFE_DELETE(lpBone, ri.MemFree);
}
static void R_ReleaseModelGeosetAnim(struct tModelGeosetAnim *lpGeosetAnim) {
    SAFE_DELETE(lpGeosetAnim->lpNext, R_ReleaseModelGeosetAnim);
    SAFE_DELETE(lpGeosetAnim->lpAlphas, ri.MemFree);
    SAFE_DELETE(lpGeosetAnim, ri.MemFree);
}
static void R_ReleaseModelHelper(struct tModelHelper *lpHelper) {
    R_ReleaseModelNode(&lpHelper->node);
    SAFE_DELETE(lpHelper->lpNext, R_ReleaseModelHelper);
    SAFE_DELETE(lpHelper, ri.MemFree);
}
void R_ReleaseModel(LPMODEL lpModel) {
    SAFE_DELETE(lpModel->lpGeosets, R_ReleaseModelGeoset);
    SAFE_DELETE(lpModel->lpMaterials, R_ReleaseModelMaterial);
    SAFE_DELETE(lpModel->lpBones, R_ReleaseModelBone);
    SAFE_DELETE(lpModel->lpGeosetAnims, R_ReleaseModelGeosetAnim);
    SAFE_DELETE(lpModel->lpHelpers, R_ReleaseModelHelper);
    SAFE_DELETE(lpModel->lpTextures, ri.MemFree);
    SAFE_DELETE(lpModel->lpSequences, ri.MemFree);
    SAFE_DELETE(lpModel->lpGlobalSequences, ri.MemFree);
    SAFE_DELETE(lpModel->lpPivots, ri.MemFree);
}
