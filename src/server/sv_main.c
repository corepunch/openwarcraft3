#include "server.h"

//#define PRINT_ANIMATIONS

struct game_export *ge;
struct server sv;
struct server_static svs;

void SV_WriteConfigString(LPSIZEBUF msg, DWORD i) {
    MSG_WriteByte(msg, svc_configstring);
    MSG_WriteShort(msg, i);
    if (i == CS_STATUSBAR) {
        MSG_Write(msg, sv.configstrings[i], sizeof(*sv.configstrings));
    } else {
        MSG_WriteString(msg, ge->GetThemeValue(sv.configstrings[i]));
    }
    sv.syncstrings[i] = true;
}

static void SV_SendClientDatagram(LPCLIENT client) {
    SV_BuildClientFrame(client);
    SV_WriteFrameToClient(client);
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

static void SV_SendClientMessages(void) {
    FOR_LOOP(i, MAX_CONFIGSTRINGS) {
        if (*sv.configstrings[i] && !sv.syncstrings[i]) {
            SV_WriteConfigString(&sv.multicast, i);
            SV_Multicast(&(VECTOR3){0,0,0}, MULTICAST_ALL_R);
        }
    }
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        if (client->state == cs_spawned) {
            SV_SendClientDatagram(client);
        }
    }
}

static void SV_ReadPackets(void) {
    static BYTE net_message_buffer[MAX_MSGLEN];
    static sizeBuf_t net_message = {
        .data = net_message_buffer,
        .maxsize = MAX_MSGLEN,
        .cursize = 0,
        .readcount = 0,
    };
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        while (NET_GetPacket(NS_SERVER, client->netchan.sock, &net_message)) {
            SV_ParseClientMessage(&net_message, client);
        }
    }
}

static int SV_FindIndex(LPCSTR name, int start, int max, bool create) {
    if (!name || !name[0])
        return 0;
    int i;
    for (i=1 ; i<max && sv.configstrings[start+i][0] ; i++)
        if (!strcmp(sv.configstrings[start+i], name))
            return i;
    if (!create)
        return 0;
    strncpy(sv.configstrings[start+i], name, sizeof(*sv.configstrings));
    return i;
}

int SV_ModelIndex(LPCSTR name) {
//    if (!strcmp(name, "units\\human\\Peasant\\Peasant.mdx")) {
//        name = "Assets\\Units\\Terran\\MarineTychus\\MarineTychus.m3";
//    }
    PATHSTR model_filename = { 0 };
    strcpy(model_filename, name);
    if (!strstr(model_filename, ".mdx")) {
        LPSTR mdl = strstr(model_filename, ".mdl");
        mdl = mdl ? mdl : (model_filename + strlen(model_filename));
        strcpy(mdl, ".mdx");
    }
    int modelindex = SV_FindIndex(model_filename, CS_MODELS, MAX_MODELS, true);
    if (!sv.models[modelindex]) {
        sv.models[modelindex] = SV_LoadModel(sv.configstrings[CS_MODELS + modelindex]);
    }
#if 0
    if (!strstr(name, "Doodads\\")) {
        printf("%s\n", name);
        FOR_LOOP(i, sv.models[modelindex]->num_animations){
            LPANIMATION anim = &sv.models[modelindex]->animations[i];
            printf("    %s\n", anim->name);
        }
    }
#endif
    return modelindex;
}

void SV_LoadModels(void) {
    for (DWORD i = 2; i < MAX_MODELS && *sv.configstrings[CS_MODELS + i]; i++) {
        if (sv.models[i])
            continue;
//        LPCSTR filename = sv.configstrings[CS_MODELS + i];
        sv.models[i] = SV_LoadModel(sv.configstrings[CS_MODELS + i]);
    }
}

int SV_SoundIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_SOUNDS, MAX_SOUNDS, true);
}

int SV_ImageIndex(LPCSTR name) {
    return SV_FindIndex(name, CS_IMAGES, MAX_IMAGES, true);
}

int SV_FontIndex(LPCSTR name, DWORD fontSize) {
    PATHSTR fontspec;
    sprintf(fontspec, "%s,%d", name, fontSize);
    return SV_FindIndex(fontspec, CS_FONTS, MAX_FONTSTYLES, true);
}

void SV_RunGameFrame(void) {
    sv.framenum++;
    sv.time += FRAMETIME;
    ge->RunFrame();
}

void SV_Frame(DWORD msec) {
    svs.realtime += msec;
    
    if (svs.realtime < sv.time) {
        return;
    }

    SV_ReadPackets();

    SV_RunGameFrame();

    SV_SendClientMessages();
}
