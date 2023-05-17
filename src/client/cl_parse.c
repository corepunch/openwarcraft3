#include "client.h"

#define READ_IF(flag, value, type, scale) \
if (bits & (1 << kEntityChangeFlag_##flag)) \
    lpEdict->value = MSG_Read##type(msg) * scale;

void
CL_ParseDeltaEntity(LPSIZEBUF msg,
                    LPENTITYSTATE lpEdict,
                    int number,
                    int bits)
{
    lpEdict->number = number;
    READ_IF(originX, origin.x, Short, 1);
    READ_IF(originY, origin.y, Short, 1);
    READ_IF(originZ, origin.z, Short, 1);
    READ_IF(angle, angle, Short, 0.01f);
    READ_IF(scale, scale, Short, 0.01f);
    READ_IF(frame, frame, Short, 1);
    READ_IF(model, model, Short, 1);
    READ_IF(image, image, Short, 1);
}

static void CL_ReadPacketEntities(LPSIZEBUF msg) {
    while (true) {
        uint32_t bits = 0;
        int nument = CL_ParseEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        struct client_entity *lpEdict = &cl.ents[nument];
        CL_ParseDeltaEntity(msg, &lpEdict->current, nument, bits);
    }
    cl.num_entities = MAX_CLIENT_ENTITIES;
}

static void CL_ParseConfigString(LPSIZEBUF msg) {
    int const index = MSG_ReadShort(msg);
    MSG_ReadString(msg, cl.configstrings[index]);
}

static void CL_ParseBaseline(LPSIZEBUF msg) {
    uint32_t bits = 0;
    uint32_t index = CL_ParseEntityBits(msg, &bits);
    struct client_entity *cent = &cl.ents[index];
    memset(&cent->baseline, 0, sizeof(ENTITYSTATE));
    CL_ParseDeltaEntity(msg, &cent->baseline, index, bits);
    memcpy(&cent->current, &cent->baseline, sizeof(ENTITYSTATE));
}

void CL_ParseFrame(LPSIZEBUF msg) {
    cl.frame.serverframe = MSG_ReadLong(msg);
    cl.frame.oldclientframe = MSG_ReadLong(msg);
}

void CL_ParseServerMessage(LPSIZEBUF msg) {
    uint8_t pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
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
