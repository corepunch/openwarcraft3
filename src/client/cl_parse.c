#include "client.h"

static void CL_ReadPacketEntities(LPSIZEBUF msg) {
    while (true) {
        DWORD bits = 0;
        int nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        centity_t *ent = &cl.ents[nument];
        if (bits & (1 << U_REMOVE)) {
            memset(ent, 0, sizeof(centity_t));
            continue;
        }
        ent->prev = ent->current;
        MSG_ReadDeltaEntity(msg, &ent->current, nument, bits);
        if (ent->serverframe != cl.frame.serverframe - 1) {
            ent->prev = ent->current;
        }
        ent->serverframe = cl.frame.serverframe;
    }
    cl.num_entities = MAX_CLIENT_ENTITIES;
}

static void CL_ParseConfigString(LPSIZEBUF msg) {
    int const index = MSG_ReadShort(msg);
    if (index == CS_STATUSBAR) {
        MSG_Read(msg, cl.configstrings[index], sizeof(*cl.configstrings));
    } else {
        MSG_ReadString(msg, cl.configstrings[index]);
    }
//    printf("%d %s\n", index, cl.configstrings[index]);
}

static void CL_ParseBaseline(LPSIZEBUF msg) {
    DWORD bits = 0;
    DWORD index = MSG_ReadEntityBits(msg, &bits);
    centity_t *cent = &cl.ents[index];
    memset(&cent->baseline, 0, sizeof(entityState_t));
    MSG_ReadDeltaEntity(msg, &cent->baseline, index, bits);
    memcpy(&cent->current, &cent->baseline, sizeof(entityState_t));
    memcpy(&cent->prev, &cent->baseline, sizeof(entityState_t));
}

void CL_ParseFrame(LPSIZEBUF msg) {
    cl.frame.serverframe = MSG_ReadLong(msg);
    cl.frame.servertime = MSG_ReadLong(msg);
    cl.frame.oldclientframe = MSG_ReadLong(msg);
    cl.time = cl.frame.servertime;
}

void CL_ParsePlayerInfo(LPSIZEBUF msg) {
    DWORD bits;
    DWORD plnum = MSG_ReadEntityBits(msg, &bits);
    MSG_ReadDeltaPlayerState(msg, &cl.playerstate, plnum, bits);
    cl.viewDef.camera.origin.x = cl.playerstate.origin.x;
    cl.viewDef.camera.origin.y = cl.playerstate.origin.y;
    cl.viewDef.camera.origin.z = 0;
    cl.viewDef.camera.viewangles = cl.playerstate.viewangles;
    cl.viewDef.camera.distance = cl.playerstate.distance;
    cl.viewDef.camera.fov = cl.playerstate.fov;
}

void CL_ParseLayout(LPSIZEBUF msg) {
    DWORD layer = MSG_ReadByte(msg);
    uiFrame_t *frames = cl.layout[layer];
    uiFrame_t *ent = NULL;
    memset(frames, 0, sizeof(uiLayoutLayer_t));
    frames[0].size.width = 8000;
    frames[0].size.height = 6000;
    frames[0].flags.type = FT_SCREEN;
    while (true) {
        DWORD bits = 0;
        int nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        ent = &frames[nument];
        ent->tex.coord[1] = 0xff;
        ent->tex.coord[3] = 0xff;
        MSG_ReadDeltaUIFrame(msg, ent, nument, bits);
    }
}

void CL_ParseCursor(LPSIZEBUF msg) {
    DWORD bits = 0;
    SAFE_DELETE(cl.cursorEntity, MemFree);
    cl.cursorEntity = MemAlloc(sizeof(entityState_t));
    MSG_ReadEntityBits(msg, &bits);
    MSG_ReadDeltaEntity(msg, cl.cursorEntity, 0, bits);
    if (cl.cursorEntity->model == 0) {
        SAFE_DELETE(cl.cursorEntity, MemFree);
    }
}

void CL_MirrorMessage(LPSIZEBUF msg) {
    char buf[256] = { 0 };
    MSG_ReadString(msg, buf);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, buf);
}

void CL_ParseServerMessage(LPSIZEBUF msg) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case svc_playerinfo:
                CL_ParsePlayerInfo(msg);
                break;
            case svc_spawnbaseline:
                CL_ParseBaseline(msg);
                break;
            case svc_packetentities:
                CL_ReadPacketEntities(msg);
                break;
            case svc_configstring:
                CL_ParseConfigString(msg);
                break;
            case svc_frame:
                CL_ParseFrame(msg);
                break;
            case svc_layout:
                CL_ParseLayout(msg);
                break;
            case svc_cursor:
                CL_ParseCursor(msg);
                break;
            case svc_mirror:
                CL_MirrorMessage(msg);
                break;
            case svc_temp_entity:
                CL_ParseTEnt(msg);
                break;
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
