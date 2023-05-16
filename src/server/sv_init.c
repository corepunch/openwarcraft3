#include "server.h"

static struct size2 sv_pathmapSize;
static struct PathMapNode *sv_pathmap;

void SV_CreateBaseline(void) {
    sv.baselines = MemAlloc(sizeof(struct entity_state) * ge->max_edicts);
    FOR_LOOP(entnum, ge->num_edicts) {
        struct edict *svent = EDICT_NUM(entnum);
        sv.baselines[entnum] = svent->s;
        svent->s.number = entnum;
    }
}

struct PathMapNode const *SV_PathMapNode(struct terrain const *heightmap, int x, int y) {
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
    
    struct Doodad *lpDoodads = MemAlloc(dwNumDoodads * DOODAD_SIZE);
    
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
    SV_CreateBaseline();
    SFileCloseArchive(hMapArchive);
}

void SV_InitGame(void) {
    if (svs.initialized) {
        SV_Shutdown();
    }
    svs.initialized = true;
    svs.num_client_entities = UPDATE_BACKUP * MAX_CLIENTS * MAX_PACKET_ENTITIES;
    svs.client_entities = MemAlloc(sizeof(struct entity_state) * svs.num_client_entities);
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

void SV_Init(void) {
    ge = GetGameAPI(&(struct game_import) {
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ModelIndex = SV_ModelIndex,
        .ImageIndex = SV_ImageIndex,
        .SoundIndex = SV_SoundIndex,
    });
    ge->Init();
    memset(&svs, 0, sizeof(struct server_static));
    memset(&sv, 0, sizeof(struct server));
    __netchan_init(&svs.clients[0].netchan);
    svs.num_clients = 1;
    sv.baselines = MemAlloc(sizeof(struct entity_state) * ge->max_edicts);
}

