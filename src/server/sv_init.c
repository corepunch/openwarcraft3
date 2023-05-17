#include "server.h"

#include <math.h>

static struct size2 sv_pathmapSize;
static struct PathMapNode *sv_pathmap;

LPWAR3MAP lpMap;

LPWAR3MAP FileReadWar3Map(HANDLE hArchive);

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(ENTITYSTATE) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        LPEDICT svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

struct PathMapNode const *SV_PathMapNode(LPCWAR3MAP lpTerrain, DWORD x, DWORD y) {
    int const index = x + y * sv_pathmapSize.width;
    return &sv_pathmap[index];
}

static void SV_ReadDoodads(HANDLE hArchive) {
    HANDLE hFile;
    DWORD dwFileHeader, dwVersion, dwUnknown, dwNumDoodads;

    SFileOpenFileEx(hArchive, "war3map.doo", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &dwFileHeader, 4, NULL, NULL);
    SFileReadFile(hFile, &dwVersion, 4, NULL, NULL);
    SFileReadFile(hFile, &dwUnknown, 4, NULL, NULL);
    SFileReadFile(hFile, &dwNumDoodads, 4, NULL, NULL);
    
    LPDOODAD lpDoodads = MemAlloc(dwNumDoodads * DOODAD_SIZE);
    
    SFileReadFile(hFile, lpDoodads, dwNumDoodads * DOODAD_SIZE, NULL, NULL);
    SFileCloseFile(hFile);
    
    ge->SpawnEntities(lpDoodads, dwNumDoodads);
    
    MemFree(lpDoodads);
}

static void SV_ReadPathMap(HANDLE hArchive) {
    HANDLE hFile;
    DWORD header, version;
    SFileOpenFileEx(hArchive, "war3map.wpm", SFILE_OPEN_FROM_MPQ, &hFile);
    SFileReadFile(hFile, &header, 4, NULL, NULL);
    SFileReadFile(hFile, &version, 4, NULL, NULL);
    SFileReadFile(hFile, &sv_pathmapSize, 8, NULL, NULL);
    int const pathmapblocksize = sv_pathmapSize.width * sv_pathmapSize.height;
    sv_pathmap = MemAlloc(pathmapblocksize);
    SFileReadFile(hFile, sv_pathmap, pathmapblocksize, 0, 0);
    SFileCloseFile(hFile);
}

void SV_Map(LPCSTR szMapFilename) {
    SV_InitGame();
    
    HANDLE hMapArchive;
    memset(&sv, 0, sizeof(struct server));
    strcpy(sv.configstrings[CS_MODELS+1], szMapFilename);
    FS_ExtractFile(szMapFilename, TMP_MAP);
    SFileOpenArchive(TMP_MAP, 0, 0, &hMapArchive);
    SV_ReadPathMap(hMapArchive);
    SV_ReadDoodads(hMapArchive);
    lpMap = FileReadWar3Map(hMapArchive);
    SV_CreateBaseline();
    SFileCloseArchive(hMapArchive);
}

void SV_InitGame(void) {
    if (svs.initialized) {
        SV_Shutdown();
    }
    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * MAX_CLIENTS * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(ENTITYSTATE) * svs.num_client_entities);
}

void SV_Shutdown(void) {
    SAFE_DELETE(sv.baselines, MemFree);
    SAFE_DELETE(svs.client_entities, MemFree);
    ge->Shutdown();
}

void __netchan_init(struct netchan *netchan){
    memset(netchan, 0, sizeof(struct netchan));
    netchan->message.data = netchan->message_buf;
    netchan->message.maxsize = sizeof(netchan->message_buf);
}

static struct AnimationInfo SV_GetAnimation(int modelindex, LPCSTR animname) {
    struct cmodel *model = sv.models[modelindex];
    if (!model) {
        return (struct AnimationInfo) { 0 };
    }
//    FOR_LOOP(i, model->num_animations){
//        struct mdx_sequence *anim = &model->animations[i];
//        printf("%s %d %d\n", anim->name, anim->interval[0], anim->interval[1]);
//    }
    FOR_LOOP(i, model->num_animations){
        struct mdx_sequence *anim = &model->animations[i];
        if (!strcmp(anim->name, animname)) {
            return (struct AnimationInfo) {
                .start_frame = anim->interval[0],
                .end_frame = anim->interval[1],
//                .framerate = anim->,
            };
        }
    }
    return (struct AnimationInfo) {};
}

static float LerpNumber(float a, float b, float t) {
    return a * (1 - t) + b * t;
}

static float SV_GetHeightAtPoint(float sx, float sy) {
    extern LPWAR3MAP lpMap;
    float x = (sx - lpMap->center.x) / TILESIZE;
    float y = (sy - lpMap->center.y) / TILESIZE;
    float fx = floorf(x);
    float fy = floorf(y);
    float a = GetWar3MapVertexHeight(GetWar3MapVertex(lpMap, fx, fy));
    float b = GetWar3MapVertexHeight(GetWar3MapVertex(lpMap, fx + 1, fy));
    float c = GetWar3MapVertexHeight(GetWar3MapVertex(lpMap, fx, fy + 1));
    float d = GetWar3MapVertexHeight(GetWar3MapVertex(lpMap, fx + 1, fy + 1));
    float ab = LerpNumber(a, b, x - fx);
    float cd = LerpNumber(c, d, x - fx);
    return LerpNumber(ab, cd, y - fy);
}

void SV_Init(void) {
    ge = GetGameAPI(&(struct game_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = SV_ModelIndex,
        .ImageIndex = SV_ImageIndex,
        .SoundIndex = SV_SoundIndex,
        .ParseSheet = FS_ParseSheet,
        .GetAnimation = SV_GetAnimation,
        .GetHeightAtPoint = SV_GetHeightAtPoint,
    });
    ge->Init();
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));
    __netchan_init(&svs.clients[0].netchan);
    svs.num_clients = 1;
    sv.baselines = MemAlloc(sizeof(ENTITYSTATE) * ge->max_edicts);
}

