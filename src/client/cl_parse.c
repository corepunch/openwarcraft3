#include "client.h"

static void CL_ReadPacketEntities(LPSIZEBUF msg) {
    while (true) {
        DWORD bits = 0;
        int nument = CL_ParseEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        clientEntity_t *edict = &cl.ents[nument];
        edict->prev = edict->current;
        MSG_ReadDeltaEntity(msg, &edict->current, nument, bits);
    }
    cl.num_entities = MAX_CLIENT_ENTITIES;
}

static void CL_ParseConfigString(LPSIZEBUF msg) {
    int const index = MSG_ReadShort(msg);
    MSG_ReadString(msg, cl.configstrings[index]);
}

static void CL_ParseBaseline(LPSIZEBUF msg) {
    DWORD bits = 0;
    DWORD index = CL_ParseEntityBits(msg, &bits);
    clientEntity_t *cent = &cl.ents[index];
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
    MSG_Read(msg, &cl.playerNumber, sizeof(DWORD));
    MSG_Read(msg, &cl.viewDef.camera.target, sizeof(VECTOR2));
    cl.viewDef.camera.target.z = 0;
}

void CL_ParseLayout(LPSIZEBUF msg) {
//    MSG_ReadString(msg, cl.layout);
    typedef char cmdarg_t[CMDARG_LEN];
    static cmdarg_t args[MAX_CMDARGS];
    static LPCSTR argv[MAX_CMDARGS];
    DWORD argc = 0;
    LPCSTR command = MSG_ReadString2(msg);
    parser_t p = { 0 };
    p.tok = p.token;
    p.str = command;
    for (LPCSTR tok = ParserGetToken(&p); tok; tok = ParserGetToken(&p)) {
        strcpy(args[argc], tok);
        argv[argc] = args[argc];
        argc++;
    }
    ui.ServerCommand(argc, argv);
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
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
