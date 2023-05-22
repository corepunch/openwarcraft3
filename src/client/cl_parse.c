#include "client.h"

#define READ_IF(FLAG, VALUE, TYPE, SCALE) \
if (bits & (1 << FLAG)) \
    edict->VALUE = MSG_Read##TYPE(msg) * SCALE;

void
CL_ParseDeltaEntity(LPSIZEBUF msg,
                    entityState_t *edict,
                    int number,
                    int bits)
{
    edict->number = number;
    READ_IF(U_ORIGIN1, origin.x, Short, 1);
    READ_IF(U_ORIGIN2, origin.y, Short, 1);
    READ_IF(U_ORIGIN3, origin.z, Short, 1);
    READ_IF(U_ANGLE, angle, Short, 0.01f);
    READ_IF(U_SCALE, scale, Short, 0.01f);
    READ_IF(U_FRAME, frame, Long, 1);
    READ_IF(U_MODEL, model, Short, 1);
    READ_IF(U_IMAGE, image, Short, 1);
    READ_IF(U_PLAYER, player, Byte, 1);
}

static void CL_ReadPacketEntities(LPSIZEBUF msg) {
    while (true) {
        DWORD bits = 0;
        int nument = CL_ParseEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        clientEntity_t *edict = &cl.ents[nument];
        edict->prev = edict->current;
        CL_ParseDeltaEntity(msg, &edict->current, nument, bits);
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
    CL_ParseDeltaEntity(msg, &cent->baseline, index, bits);
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
    MSG_Read(msg, &cl.startingPosition, sizeof(VECTOR2));
        
    cl.viewDef.vieworg.x = cl.startingPosition.x;
    cl.viewDef.vieworg.y = cl.startingPosition.y - 800;
    cl.viewDef.vieworg.z = 1200;
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
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
