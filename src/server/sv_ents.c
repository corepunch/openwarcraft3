#include "server.h"
#include <math.h>

#define VISUAL_DISTANCE 1000
#define HIGH_NUMBER 9999

static bool SV_CanClientSeeEntity(struct client const *client,
                                  LPCENTITYSTATE lpEdict)
{
    if (fabs(lpEdict->origin.x - client->camera_position.x) > VISUAL_DISTANCE)
        return false;
    if (fabs(lpEdict->origin.y - client->camera_position.y) > VISUAL_DISTANCE)
        return false;
    return true;
}

LPENTITYSTATE SV_NextClientEntity(void) {
    int index = svs.next_client_entities++ % svs.num_client_entities;
    return &svs.client_entities[index];
}

void SV_BuildClientFrame(struct client *client) {
    LPCLIENTFRAME frame = &client->frames[sv.framenum & UPDATE_MASK];
    frame->num_entities = 0;
    frame->first_entity = svs.next_client_entities;
    FOR_LOOP(index, ge->num_edicts) {
        LPEDICT lpEdict = EDICT_NUM(index);
        if (!lpEdict->s.model && !lpEdict->s.sound && !lpEdict->s.event)
            continue;
        if (!SV_CanClientSeeEntity(client, &lpEdict->s))
            continue;
        LPENTITYSTATE state = SV_NextClientEntity();
        *state = lpEdict->s;
        frame->num_entities++;
    }
}

void SV_EmitPacketEntities(LPCCLIENTFRAME from,
                           LPCCLIENTFRAME to,
                           LPSIZEBUF msg)
{
    int const from_num_entities = from ? from->num_entities : 0;

    MSG_WriteByte (msg, svc_packetentities);
    
    for (int newindex = 0, oldindex = 0;
         newindex < to->num_entities ||
         oldindex < from_num_entities;)
    {
        LPENTITYSTATE newent = NULL, oldent = NULL;
        int newnum = 0, oldnum = 0;
        if (newindex >= to->num_entities) {
            newnum = HIGH_NUMBER;
        } else{
            newent = &svs.client_entities[(to->first_entity+newindex)%svs.num_client_entities];
            newnum = newent->number;
        }
        if (oldindex >= from_num_entities) {
            oldnum = HIGH_NUMBER;
        } else {
            oldent = &svs.client_entities[(from->first_entity+oldindex)%svs.num_client_entities];
            oldnum = oldent->number;
        }
        if (newnum == oldnum) {
            MSG_WriteDeltaEntity(msg, oldent, newent);
            oldindex++;
            newindex++;
            continue;
        }
        if (newnum < oldnum) { // this is a new entity, send it from the baseline
            MSG_WriteDeltaEntity(msg, &sv.baselines[newnum], newent);
            newindex++;
            continue;
        }
        if (newnum > oldnum) { // the old entity isn't present in the new message
            MSG_WriteShort(msg, 1 << U_REMOVE);
            MSG_WriteShort(msg, oldnum);
            oldindex++;
            continue;
        }
    }
    MSG_WriteLong(msg, 0);    // end of packetentities
}

void SV_WriteFrameToClient(struct client *client) {
    LPCLIENTFRAME frame = &client->frames[sv.framenum & UPDATE_MASK];
    LPCLIENTFRAME oldframe = &client->frames[client->lastframe & UPDATE_MASK];
    
    MSG_WriteByte(&client->netchan.message, svc_frame);
    MSG_WriteLong(&client->netchan.message, sv.framenum);
    MSG_WriteLong(&client->netchan.message, client->lastframe);
    
    SV_EmitPacketEntities(oldframe, frame, &client->netchan.message);
    
    client->lastframe = sv.framenum;
    
    Netchan_Transmit(&client->netchan);
}
