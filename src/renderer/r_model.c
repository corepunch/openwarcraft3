#include "r_local.h"

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

static struct matrix4 node_matrices[MAX_BONE_MATRICES];

struct tFileBlock {
    DWORD dwHeader;
    DWORD dwSize;
    DWORD dwStart;
};

struct tFileBlock FileReadBlock(HANDLE hFile) {
    struct tFileBlock block;
    block.dwHeader = 0;
    block.dwStart = SFileSetFilePointer(hFile, 0, NULL, FILE_CURRENT);;
    SFileReadFile(hFile, &block.dwSize, 4, NULL, NULL);
    return block;
}

DWORD FileReadInt32(HANDLE hFile) {
    DWORD dwValue = 0;
    SFileReadFile(hFile, &dwValue, 4, NULL, NULL);
    return dwValue;
}

bool FileIsAtEndOfBlock(HANDLE hFile, struct tFileBlock const *lpBlock) {
    DWORD const dwFilePointer = SFileSetFilePointer(hFile, 0, NULL, FILE_CURRENT);
    DWORD const dwEndOfBlock = lpBlock->dwStart + lpBlock->dwSize;
    return dwFilePointer >= dwEndOfBlock;
}

void FileMoveToEndOfBlock(HANDLE hFile, struct tFileBlock const *lpBlock) {
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
            case ID_MTGC: SFileReadArray(hFile, lpGeoset, MatrixGroups, sizeof(int));break;
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

void ReadMaterialLayer(HANDLE hFile,
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

struct tModelMaterial *ReadMaterial(HANDLE hFile, struct tFileBlock const block) {
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

void *ReadModelKeyTrack(HANDLE hFile, enum tModelKeyTrackDataType dwDataType) {
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

void ReadNode(HANDLE hFile, struct tFileBlock const block, struct tModelNode *lpNode) {
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

struct tModelBone *ReadBone(HANDLE hFile, struct tFileBlock const block) {
    struct tModelBone *lpBone = ri.MemAlloc(sizeof(struct tModelBone));
    ReadNode(hFile, block, &lpBone->node);
    lpBone->geoset_id = FileReadInt32(hFile);
    lpBone->geoset_animation_id = FileReadInt32(hFile);
    return lpBone;
}

struct tModelHelper *ReadHelper(HANDLE hFile, struct tFileBlock const block) {
    struct tModelHelper *lpHelper = ri.MemAlloc(sizeof(struct tModelHelper));
    ReadNode(hFile, block, &lpHelper->node);
    return lpHelper;
}

struct tModelGeosetAnim *ReadGeosetAnim(HANDLE hFile, struct tFileBlock const block) {
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
    FOR_LOOP(i, lpGeoset->numMatrixGroups) {
        dwBiggest = MAX(lpGeoset->lpMatrixGroups[i], dwBiggest);
    }
    return dwBiggest;
}

void R_SetupGeoset(struct tModel *lpModel, struct tModelGeoset *lpGeoset) {
    DWORD dwBiggestGeoset = R_ModelFindBiggestGroup(lpGeoset);
    if (dwBiggestGeoset > 4) {
        fprintf(stderr, "Geosets with more that 4 bones skinning are not supported\n");
        return;
    }
    
    typedef uint8_t matrixGroup_t[4];
    struct tVertex *lpVertices = ri.MemAlloc(sizeof(struct tVertex) * lpGeoset->numTriangles);
    matrixGroup_t *lpMatrixGroups = ri.MemAlloc(sizeof(matrixGroup_t) * lpGeoset->numMatrixGroups);
    DWORD dwIndexOffset = 0;
    
    FOR_LOOP(dwMatrixGroupIndex, lpGeoset->numMatrixGroups) {
        FOR_LOOP(dwMatrixIndex, lpGeoset->lpMatrixGroups[dwMatrixGroupIndex]) {
            lpMatrixGroups[dwMatrixGroupIndex][dwMatrixIndex] = lpGeoset->lpMatrices[dwIndexOffset++] + 1;
        }
    }

    FOR_LOOP(dwTriangle, lpGeoset->numTriangles) {
        int dwVertex = lpGeoset->lpTriangles[dwTriangle];
        uint8_t *dwMatrixGroup = lpMatrixGroups[lpGeoset->lpVertexGroups[dwVertex]];
        lpVertices[dwTriangle].color = (struct color32) { 255, 255, 255, 255 };
        lpVertices[dwTriangle].position = lpGeoset->lpVertices[dwVertex];
        lpVertices[dwTriangle].texcoord = lpGeoset->lpTexcoord[dwVertex];
        memcpy(lpVertices[dwTriangle].skin, dwMatrixGroup, sizeof(matrixGroup_t));
    }

    lpGeoset->lpBuffer = R_MakeVertexArrayObject(lpVertices, lpGeoset->numTriangles * sizeof(struct tVertex));
    
    ri.MemFree(lpVertices);
    ri.MemFree(lpMatrixGroups);
}

//DWORD R_GetModelFrameNumber(struct tModel const *lpModel) {
//    struct tModelSequence const *lpSequence = lpModel->lpCurrentAnimation;
//    DWORD const dwLocalTime = ((DWORD)lpModel->dwAnimationFrame) % (lpSequence->interval[1] - lpSequence->interval[0]);
//    return lpSequence->interval[0] + dwLocalTime;
//}

static void
R_GetKeyframeValue(struct tModelKeyFrame const *lpKeyframe1,
                   struct tModelKeyFrame const *lpKeyframe2,
                   DWORD const dwTime,
                   enum tModelKeyTrackDataType dwDataType,
                   void *out)
{
    float const t = (float)(dwTime - lpKeyframe1->time) / (float)(lpKeyframe2->time - lpKeyframe1->time);
    switch (dwDataType) {
        case kModelKeyTrackDataTypeFloat:
            *((float *)out) = (*(float *)lpKeyframe1->data) * (1 - t) + (*(float *)lpKeyframe2->data) * t;
            return;
        case kModelKeyTrackDataTypeVector3:
            *((struct vector3 *)out) = vector3_lerp((struct vector3 *)lpKeyframe1->data, (struct vector3 *)lpKeyframe2->data, t);
            return;
        case kModelKeyTrackDataTypeQuaternion:
            *((struct vector4 *)out) = quaternion_lerp((struct vector4 *)lpKeyframe1->data, (struct vector4 *)lpKeyframe2->data, t);
            return;
    }
}

static void
R_GetKeytrackValue(struct tModel const *lpModel,
                   struct tModelKeyTrack const *lpKeytrack,
                   DWORD dwFrameNumber,
                   void *out)
{
    DWORD const dwKeyframeSize = GetModelKeyFrameSize(lpKeytrack->datatype, lpKeytrack->type);
    char const *lpKeyFrames = (char *)lpKeytrack->values;
    struct tModelKeyFrame *lpPrevKeyFrame = NULL;
    FOR_LOOP(dwKeyframeIndex, lpKeytrack->dwKeyframeCount) {
        struct tModelKeyFrame *lpKeyFrame = (void *)(lpKeyFrames + dwKeyframeSize * dwKeyframeIndex);
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

static struct tModelNode *R_GetModelNodeWithObjectID(struct tModel *lpModel, DWORD dwObjectID) {
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


static void R_CalculateNodeMatrix(struct tModel const *lpModel,
                                  struct tModelNode const *lpNode,
                                  DWORD dwFrameNumber,
                                  struct matrix4 *lpMatrix)
{
    struct vector3 vTranslation = { 0, 0, 0 };
    struct vector4 vRotation = { 0, 0, 0, 1 };
    struct vector3 vScale = { 1, 1, 1 };
    struct vector3 const *lpPivot = (struct vector3 const *)lpNode->lpPivot;
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
        matrix4_identity(lpMatrix);
    } else if (lpNode->lpTranslation && !lpNode->lpRotation && !lpNode->lpScale) {
        matrix4_from_translation(lpMatrix, &vTranslation);
    } else if (!lpNode->lpTranslation && lpNode->lpRotation && !lpNode->lpScale) {
        matrix4_from_rotation_origin(lpMatrix,  &vRotation, lpPivot);
    } else {
        matrix4_from_rotation_translation_scale_origin(lpMatrix, &vRotation, &vTranslation, &vScale, lpPivot);
    }
}

struct matrix4 const *R_GetNodeGlobalMatrix(struct tModelNode *lpNode) {
    if (lpNode->globalMatrix.v[15] == 0) {
        if (lpNode->lpParent) {
            matrix4_multiply(R_GetNodeGlobalMatrix(lpNode->lpParent),
                             &lpNode->localMatrix,
                             &lpNode->globalMatrix);
        } else {
            lpNode->globalMatrix = lpNode->localMatrix;
        }
    }
    return &lpNode->globalMatrix;
}

static void R_CalculateBoneMatrices(struct tModel const *lpModel,
                                    struct matrix4 *lpModelMatrices,
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

static void R_RenderGeoset(struct tModel const *lpModel,
                           struct tModelGeoset *lpGeoset,
                           struct matrix4 const *lpModelMatrix)
{
    extern GLuint program;

    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, lpModelMatrix->v );
    glBindVertexArray( lpGeoset->lpBuffer->vao );
    glBindBuffer( GL_ARRAY_BUFFER, lpGeoset->lpBuffer->vbo );
    glDrawArrays( GL_TRIANGLES, 0, lpGeoset->numTriangles );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

struct tModel *R_LoadModelMDX(HANDLE hFile) {
    DWORD dwBlockHeader, dwFileVersion = 0;
    struct tModel *lpModel = ri.MemAlloc(sizeof(struct tModel));
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

struct tModel *R_LoadModel(LPCSTR szModelFilename) {
    DWORD dwFileHeader;
    HANDLE hFile = ri.FileOpen(szModelFilename);
//    printf("%s\n", szModelFilename);
    if (hFile == NULL) {
        // try to load without *0.mdx
        path_t szTempFileName;
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
            return R_LoadModelMDX(hFile);
        default:
            fprintf(stderr, "Unknown model format %.5s in file %s\n", (char *)&dwFileHeader, szModelFilename);
            return NULL;
    }
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
    SAFE_DELETE(lpGeoset->lpMatrixGroups, ri.MemFree);
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
void R_ReleaseModel(struct tModel *lpModel) {
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

static void RenderGeoset(struct tModel *lpModel,
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
    matrix4_identity(&model_matrix);
    matrix4_translate(&model_matrix, &lpEntity->postion);
    matrix4_rotate(&model_matrix, &(struct vector3){0, 0, lpEntity->angle * 180 / 3.14f}, ROTATE_XYZ);
    matrix4_scale(&model_matrix, &(struct vector3){lpEntity->scale, lpEntity->scale, lpEntity->scale});

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

static void R_BindBoneMatrices(struct tModel *lpModel, DWORD dwFrameNumber)
{
    extern GLuint program;
    struct matrix4 aBoneMatrices[MAX_BONE_MATRICES];

    R_CalculateBoneMatrices(lpModel, aBoneMatrices, dwFrameNumber);
    
    matrix4_identity(node_matrices);

    FOR_EACH_LIST(struct tModelHelper, bone, lpModel->lpHelpers) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    FOR_EACH_LIST(struct tModelBone, bone, lpModel->lpBones) {
        node_matrices[bone->node.objectId + 1] = bone->node.globalMatrix;
    }

    glUniformMatrix4fv( glGetUniformLocation( program, "u_nodes_matrices" ), 64, GL_FALSE, node_matrices->v );
}

void RenderModel(struct render_entity const *ent) {
    struct tModelMaterial *lpMaterial = ent->model->lpMaterials;
    struct tModelGeoset *lpGeoset = ent->model->lpGeosets;
    struct tModel *lpModel = ent->model;

    if (ent->skin) {
        R_BindTexture(ent->skin, 0);
    }
    
    R_BindBoneMatrices(lpModel, ent->frame);
    
    for (DWORD dwGeosetID = 0;
         lpMaterial && lpGeoset;
         lpGeoset = lpGeoset->lpNext, dwGeosetID++)
    {
        if (dwGeosetID < ent->model->numTextures) {
            struct tModelTexture const *mtex = &ent->model->lpTextures[dwGeosetID];
            struct tTexture const *tex = R_FindTextureByID(mtex->texid);
            if (tex) {
                R_BindTexture(tex, 0);
            }
        }
        FOR_LOOP(dwLayerID, lpMaterial->num_layers) {
            RenderGeoset(lpModel, lpGeoset, &lpMaterial->layers[dwLayerID], ent);
        }
        if (lpMaterial->lpNext) {
            lpMaterial = lpMaterial->lpNext;
        }
    }
}
