#include "client.h"

moveConfirmation_t cl_confs[MAX_CONFIRMATION_OBJECTS] = { 0 };
DWORD cl_confcounter = 0;

void CL_AllocateConfirmationObject(LPCVECTOR3 origin) {
    DWORD i = cl_confcounter++;
    cl_confs[i].origin = *origin;
    cl_confs[i].timespamp = cl.time;
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

static void CL_AddConfirmationObject(moveConfirmation_t const *mc) {
    renderEntity_t ent;
    memset(&ent, 0, sizeof(ent));
    ent.origin = mc->origin;
    ent.scale = 1;
    ent.frame = cl.time - mc->timespamp;
    ent.oldframe = cl.time - mc->timespamp;
    ent.model = cl.moveConfirmation;
    V_AddEntity(&ent);
}

void CL_AddConfirmations(void) {
    FOR_LOOP(index, MAX_CONFIRMATION_OBJECTS) {
        if (cl.time - cl_confs[index].timespamp > 1000)
            continue;
        CL_AddConfirmationObject(&cl_confs[index]);
    }
}

void CL_ClearTEnts(void) {
    memset(cl_confs, 0, sizeof(cl_confs));
}

void CL_AddTEnts(void) {
    CL_AddConfirmations();
}
