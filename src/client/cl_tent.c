#include "client.h"

#define MAX_MISSILES 64

typedef struct  {
    VECTOR3 origin;
    DWORD timespamp;
} moveConfirmation_t;

typedef enum {
    MISSILE_FREE,
    MISSILE_NORMAL,
} mistype_t;

typedef struct {
    mistype_t type;
    VECTOR3 origin;
    float angle;
    float speed;
    DWORD model;
    DWORD starttime;
    DWORD killtime;
} missile_t;

struct {
    missile_t missiles[MAX_MISSILES];
} tents = { 0 };

moveConfirmation_t cl_confs[MAX_CONFIRMATION_OBJECTS] = { 0 };
DWORD cl_confcounter = 0;

missile_t *CL_AllocMissile(void) {
    FOR_LOOP(i, MAX_MISSILES) {
        if (tents.missiles[i].type == MISSILE_FREE) {
            return &tents.missiles[i];
        }
    }
    return tents.missiles;
}

void CL_AllocateConfirmationObject(LPCVECTOR3 origin) {
    DWORD i = cl_confcounter++;
    cl_confs[i % MAX_CONFIRMATION_OBJECTS].origin = *origin;
    cl_confs[i % MAX_CONFIRMATION_OBJECTS].timespamp = cl.time;
}

void CL_ParseTEnt(LPSIZEBUF msg) {
    VECTOR3 pos;//, pos2, dir;
    tempEvent_t evt = MSG_ReadByte(msg);
    missile_t *missile;
    switch (evt) {
        case TE_MOVE_CONFIRMATION:
            MSG_ReadPos(msg, &pos);
            CL_AllocateConfirmationObject(&pos);
            break;
        case TE_MISSILE:
            missile = CL_AllocMissile();
            MSG_ReadPos(msg, &missile->origin);
            missile->model = MSG_ReadShort(msg);
            missile->speed = MSG_ReadShort(msg);
            missile->killtime = MSG_ReadShort(msg) + cl.time;
            missile->angle = MSG_ReadAngle(msg);
            missile->starttime = cl.time;
            missile->type = MISSILE_NORMAL;
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
    ent.flags |= RF_NO_FOGOFWAR;
    V_AddEntity(&ent);
}

void CL_AddMissile(missile_t const *missile) {
    VECTOR3 dir = { cos(missile->angle), sin(missile->angle), 0 };
    float distance = (cl.time - missile->starttime) * missile->speed / 1000;
    renderEntity_t ent;
    memset(&ent, 0, sizeof(ent));
    float k = (float)(cl.time -  missile->starttime) / (float)(missile->killtime - missile->starttime);
    ent.origin = Vector3_mad(&missile->origin, distance, &dir);
    ent.origin.z += sqrt(1.0 - fabs(k - 0.5) * 2.0) * 200;
    ent.scale = 1;
    ent.frame = 0;//cl.time % 1000;
    ent.oldframe = 0;//cl.time % 1000;
    ent.angle = missile->angle;
    ent.model = cl.models[missile->model];
    V_AddEntity(&ent);
}

void CL_AddConfirmations(void) {
    FOR_LOOP(i, MAX_CONFIRMATION_OBJECTS) {
        if (cl.time - cl_confs[i].timespamp > 1000)
            continue;
        CL_AddConfirmationObject(cl_confs+i);
    }
}

void CL_AddMissiles(void) {
    FOR_LOOP(i, MAX_MISSILES) {
        missile_t *missile = tents.missiles+i;
        if (missile->type == MISSILE_FREE)
            continue;
        if (missile->killtime < cl.time) {
            missile->type = MISSILE_FREE;
            continue;;
        }
        CL_AddMissile(tents.missiles+i);
    }
}

void CL_ClearTEnts(void) {
    memset(&tents, 0, sizeof(tents));
    memset(cl_confs, 0, sizeof(cl_confs));
}

void CL_AddTEnts(void) {
    CL_AddConfirmations();
    CL_AddMissiles();
}
