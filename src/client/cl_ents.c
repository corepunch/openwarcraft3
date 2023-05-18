#include "client.h"

int CL_ParseEntityBits(LPSIZEBUF buf, DWORD *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadShort(buf);
}

void CL_GetMouseLine(float x, float y) {
    
}

void CL_SelectEntityAtScreenPoint(float x, float y) {
    FOR_LOOP(entindex, cl.num_entities) {
        struct client_entity *clent = &cl.ents[entindex];
        
    }
}
