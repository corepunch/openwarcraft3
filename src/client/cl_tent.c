#include "client.h"

void CL_AllocateConfirmationObject(LPCVECTOR3 origin) {
    DWORD i = cl.confirmationCounter++;
    cl.confs[i].origin = *origin;
    cl.confs[i].timespamp = cl.time;
}

void CL_ParseTEnt(LPSIZEBUF msg) {
    VECTOR3 pos;//, pos2, dir;
    tempEvent_t evt = MSG_ReadByte(msg);
    switch (evt) {
        case TE_MOVE_CONFIRMATION:
            MSG_ReadPos(msg, &pos);
            CL_AllocateConfirmationObject(&pos);
            break;
        default:
            Com_Error(ERR_DROP, "CL_ParseTEnt: bad type %d", evt);
            break;
    }
}

