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
lpModel->lp##TYPES = MemAlloc(BLOCK.dwSize); \
lpModel->num##TYPES = BLOCK.dwSize / sizeof(struct tModel##TYPE); \
SFileReadFile(FILE, lpModel->lp##TYPES, BLOCK.dwSize, NULL, NULL);

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
    struct tModelGeoset *lpGeoset = MemAlloc(sizeof(struct tModelGeoset));
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
    struct tModelMaterial *lpMaterial = MemAlloc(sizeof(struct tModelMaterial));
    lpMaterial->priority = FileReadInt32(hFile);
    lpMaterial->flags = FileReadInt32(hFile);
    while (!FileIsAtEndOfBlock(hFile, &block)) {
        DWORD const dwBlockHeader = FileReadInt32(hFile);
        switch (dwBlockHeader) {
            case ID_LAYS:
                SFileReadFile(hFile, &lpMaterial->num_layers, 4, NULL, NULL);
                if (lpMaterial->num_layers > 0) {
                    lpMaterial->layers = MemAlloc(sizeof(struct tModelMaterialLayer) * lpMaterial->num_layers);
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

void *ReadModelKeyTrack(HANDLE hFile, int dwElemSize) {
    DWORD const dwCount = FileReadInt32(hFile);
    DWORD const dwSize = 12 + dwElemSize * dwCount;
    SFileSetFilePointer(hFile, -4, NULL, FILE_CURRENT);
    void *lpTrack = MemAlloc(dwSize);
    SFileReadFile(hFile, lpTrack, dwSize, NULL, NULL);
    return lpTrack;
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
                lpNode->lpTranslation = ReadModelKeyTrack(hFile, sizeof(struct tModelKeyframeVector3));
                break;
            case ID_KGRT:
                lpNode->lpRotation = ReadModelKeyTrack(hFile, sizeof(struct tModelKeyframeVector4));
                break;
            case ID_KGSC:
                lpNode->lpScale = ReadModelKeyTrack(hFile, sizeof(struct tModelKeyframeVector3));
                break;
        }
    }
}

struct tModelBone *ReadBone(HANDLE hFile, struct tFileBlock const block) {
    struct tModelBone *lpBone = MemAlloc(sizeof(struct tModelBone));
    ReadNode(hFile, block, &lpBone->node);
    lpBone->geoset_id = FileReadInt32(hFile);
    lpBone->geoset_animation_id = FileReadInt32(hFile);
    return lpBone;
}

struct tModelHelper *ReadHelper(HANDLE hFile, struct tFileBlock const block) {
    struct tModelHelper *lpHelper = MemAlloc(sizeof(struct tModelHelper));
    ReadNode(hFile, block, &lpHelper->node);
    return lpHelper;
}

struct tModelGeosetAnim *ReadGeosetAnim(HANDLE hFile, struct tFileBlock const block) {
    struct tModelGeosetAnim *lpGeosetAnim = MemAlloc(sizeof(struct tModelGeosetAnim));
    SFileReadFile(hFile, lpGeosetAnim, 24, NULL, NULL);
    while (!FileIsAtEndOfBlock(hFile, &block)) {
        DWORD const dwBlockHeader = FileReadInt32(hFile);
        switch (dwBlockHeader) {
            case ID_KGAO:
                lpGeosetAnim->lpAlphas = ReadModelKeyTrack(hFile, sizeof(struct tModelKeyframeFloat));
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

void R_SetupGeoset(struct tModelGeoset *lpGeoset) {
    if (lpGeoset->buf.vao != 0) {
        // already calculated
        return;
    }

    DWORD dwBiggestGeoset = R_ModelFindBiggestGroup(lpGeoset);
    if (dwBiggestGeoset > 4) {
        fprintf(stderr, "Geosets with more that 4 bones skinning are not supported\n");
        return;
    }
    
    typedef uint8_t matrixGroup_t[4];
    struct tVertex *lpVertices = MemAlloc(sizeof(struct tVertex) * lpGeoset->numTriangles);
    matrixGroup_t *lpMatrixGroups = MemAlloc(sizeof(matrixGroup_t) * lpGeoset->numMatrixGroups);
    DWORD dwIndexOffset = 0;
    
    FOR_LOOP(dwMatrixGroupIndex, lpGeoset->numMatrixGroups) {
        FOR_LOOP(dwMatrixIndex, lpGeoset->lpMatrixGroups[dwMatrixGroupIndex]) {
            lpMatrixGroups[dwMatrixGroupIndex][dwMatrixIndex] = lpGeoset->lpMatrices[dwIndexOffset++] + 1;
        }
    }

    FOR_LOOP(dwTriangle, lpGeoset->numTriangles) {
        int dwVertex = lpGeoset->lpTriangles[dwTriangle];
        int dwMatrixGroup = lpGeoset->lpMatrixGroups[lpGeoset->lpVertexGroups[dwVertex]];
        lpVertices[dwTriangle].color = (struct color32) { 255, 255, 255, 255 };
        lpVertices[dwTriangle].position = lpGeoset->lpVertices[dwVertex];
        lpVertices[dwTriangle].texcoord = lpGeoset->lpTexcoord[dwVertex];
        memcpy(lpVertices[dwTriangle].skin, lpMatrixGroups[dwMatrixGroup], sizeof(matrixGroup_t));
    }

    lpGeoset->buf = R_MakeVertexArrayObject(lpVertices, lpGeoset->numTriangles * sizeof(struct tVertex));
    
    MemFree(lpVertices);
    MemFree(lpMatrixGroups);
}

static struct vector3
R_GetKeytrackVector3(struct tModel const *lpModel,
                     struct tModelKeyTrackVector3 const *lpKeytrack,
                     DWORD dwSeqID,
                     DWORD dwFrameNumber,
                     DWORD dwCounter)
{
//    FOR_LOOP(index, lpKeytrack->count) {
//        printf("%f %f %f\n", lpKeytrack->values[index].value.x, lpKeytrack->values[index].value.y, lpKeytrack->values[index].value.z);
//    }
    return lpKeytrack->values[1].value;
//    if (lpModel->numGlobalSequences > 0 &&
//        lpKeytrack->globalSeqId != -1)
//    {
//        return (struct vector3) {};
//    } else {
//        struct tModelSequence const *lpSequence = &lpModel->lpSequences[dwSeqID];
//        return lpKeytrack->values[0].value;
//    }
}

static struct vector4
R_GetKeytrackVector4(struct tModel const *lpModel,
                     struct tModelKeyTrackVector4 const *lpKeytrack,
                     DWORD dwSeqID,
                     DWORD dwFrameNumber,
                     DWORD dwCounter)
{
//    FOR_LOOP(index, lpKeytrack->count) {
//        printf("%f %f %f %f\n", lpKeytrack->values[index].value.x, lpKeytrack->values[index].value.y, lpKeytrack->values[index].value.z, lpKeytrack->values[index].value.w);
//    }
    return lpKeytrack->values[1].value;
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
                                  struct matrix4 *lpMatrix)
{
    struct vector3 vTranslation = { 0, 0, 0 };
    struct vector4 vRotation = { 0, 0, 0, 1 };
    struct vector3 vScale = { 1, 1, 1 };
    if (lpNode->lpTranslation) {
        vTranslation = R_GetKeytrackVector3(lpModel, lpNode->lpTranslation, 0, 0, 0);
        if (lpNode->lpParent) {
            vTranslation.x += lpNode->lpParent->lpPivot->x;
            vTranslation.y += lpNode->lpParent->lpPivot->y;
            vTranslation.z += lpNode->lpParent->lpPivot->z;
        }
    }
    if (lpNode->lpRotation) {
        vRotation = R_GetKeytrackVector4(lpModel, lpNode->lpRotation, 0, 0, 0);
    }
    if (lpNode->lpScale) {
        vScale = R_GetKeytrackVector3(lpModel, lpNode->lpScale, 0, 0, 0);
    }
    matrix4_identity(lpMatrix);
    matrix4_translate(lpMatrix, &vTranslation);
    matrix4_rotate4(lpMatrix, &vRotation);
    matrix4_scale(lpMatrix, &vScale);
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
                                    struct matrix4 *lpModelMatrices)
{
    DWORD dwBoneIndex = 1;
    
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        memset(&lpBone->node.globalMatrix, 0, sizeof(struct matrix4));
        R_CalculateNodeMatrix(lpModel, &lpBone->node, &lpBone->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelHelper, lpHelper, lpModel->lpHelpers) {
        memset(&lpHelper->node.globalMatrix, 0, sizeof(struct matrix4));
        R_CalculateNodeMatrix(lpModel, &lpHelper->node, &lpHelper->node.localMatrix);
    }
    FOR_EACH_LIST(struct tModelBone, lpBone, lpModel->lpBones) {
        lpModelMatrices[dwBoneIndex++] = *R_GetNodeGlobalMatrix(&lpBone->node);
    }
}

#define LAYER_SIZE 500000

extern struct tVertex maplayer[LAYER_SIZE];

void R_RenderGeoset(struct tModel const *lpModel,
                    struct tModelGeoset *lpGeoset,
                    struct matrix4 const *lpModelMatrix)
{
    struct matrix4 aBoneMatrices[MAX_BONE_MATRICES];
    extern GLuint program;

    R_SetupGeoset(lpGeoset);
    R_CalculateBoneMatrices(lpModel, aBoneMatrices);
    
    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, lpModelMatrix->v );
    glBindVertexArray( lpGeoset->buf.vao );
    glBindBuffer( GL_ARRAY_BUFFER, lpGeoset->buf.vbo );
    glDrawArrays( GL_TRIANGLES, 0, lpGeoset->numTriangles );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
//    int counter = 0;
//    FOR_EACH_LIST(struct tModelBone, bone, lpModel->lpBones) {
//        if (bone->node.lpParent) {
//            struct tVertex *v = &maplayer[counter++];
//            v->position.x = bone->node.globalMatrix.v[12];
//            v->position.y = bone->node.globalMatrix.v[13];
//            v->position.z = bone->node.globalMatrix.v[14];
//            v = &maplayer[counter++];
//            v->position.x = bone->node.lpParent->globalMatrix.v[12];
//            v->position.y = bone->node.lpParent->globalMatrix.v[13];
//            v->position.z = bone->node.lpParent->globalMatrix.v[14];
//        }
//    }
//    
//    glBindVertexArray( tr.renbuf.vao );
//    glBindBuffer( GL_ARRAY_BUFFER, tr.renbuf.vbo );
//    glBufferData( GL_ARRAY_BUFFER, sizeof(maplayer), maplayer, GL_STATIC_DRAW );
//    glDrawArrays( GL_LINES, 0, counter );
}

struct tModel *R_LoadModelMDX(HANDLE hFile) {
    DWORD dwBlockHeader, dwFileVersion = 0;
    struct tModel *lpModel = MemAlloc(sizeof(struct tModel));
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
        R_SetupGeoset(lpGeoset);
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
    ri.FileClose(hFile);
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
