#include "client.h"

#define READ_IF(flag, value, type) \
if (bits & (1 << kEntityChangeFlag_##flag)) \
    ent->value = MSG_Read##type(msg);

void
CL_ParseDeltaEntity(struct sizebuf *msg,
                    struct entity_state *ent,
                    int number,
                    int bits)
{
    ent->number = number;
    READ_IF(originX, origin.x, Short);
    READ_IF(originY, origin.y, Short);
    READ_IF(originZ, origin.z, Short);
    READ_IF(angle, angle, Short);
    READ_IF(scale, scale, Short);
    READ_IF(frame, frame, Short);
    READ_IF(model, model, Short);
    READ_IF(image, image, Short);
}

static void CL_ReadPacketEntities(struct sizebuf *msg) {
    int const num_ents = MSG_ReadShort(msg);
    FOR_LOOP(i, num_ents) {
        struct client_entity *ent = &cl.ents[i];
        uint32_t bits = 0;
        uint32_t nument = CL_ParseEntityBits(msg, &bits);
        CL_ParseDeltaEntity(msg, &ent->current, nument, bits);
    }
    cl.num_entities = num_ents;
}

static void CL_ParseConfigString(struct sizebuf *msg) {
    int const index = MSG_ReadShort(msg);
    MSG_ReadString(msg, cl.configstrings[index]);
}

static void CL_ParseBaseline(struct sizebuf *msg) {
    uint32_t bits = 0;
    uint32_t index = CL_ParseEntityBits(msg, &bits);
    struct client_entity *cent = &cl.ents[index];
    memset(&cent->baseline, 0, sizeof(struct entity_state));
    CL_ParseDeltaEntity(msg, &cent->baseline, index, bits);
    memcpy(&cent->current, &cent->baseline, sizeof(struct entity_state));
}

void CL_ParseServerMessage(struct sizebuf *msg) {
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
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
