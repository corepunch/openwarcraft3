#include "client.h"

void CL_SelectEntity(DWORD number) {
    FOR_LOOP(entindex, cl.num_entities) {
        clientEntity_t *e = &cl.ents[entindex];
        e->selected = e->current.number == number;
    }
    
    memset(cls.netchan.message.data, 0, cls.netchan.message.maxsize);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    SZ_Printf(&cls.netchan.message, "inventory %i", number);
    
    if (number < cl.num_entities) {
        ui.HandleEvent(&cl.ents[number].current, UI_REFRESH_CONSOLE);
    } else {
        ui.HandleEvent(NULL, UI_REFRESH_CONSOLE);
    }
}

int CL_ParseEntityBits(LPSIZEBUF buf, DWORD *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadShort(buf);
}

LINE3 CL_GetMouseLine(DWORD pixelX, DWORD pixelY) {
    LINE3 line;
    MATRIX4 cameramat;
    MATRIX4 invcammat;
    SIZE2 const windowSize = re.GetWindowSize();
    float const x = ((float)pixelX / (float)windowSize.width - 0.5) * 2;
    float const y = (0.5 - (float)pixelY / (float)windowSize.height) * 2;
    Matrix4_getCameraMatrix(&cl.viewDef.camera, &cameramat);
    Matrix4_inverse(&cameramat, &invcammat);
    Matrix4_multiply_vector3(&invcammat, &(VECTOR3) { x, y, 0 }, &line.a);
    Matrix4_multiply_vector3(&invcammat, &(VECTOR3) { x, y, 1 }, &line.b);
    return line;
}

short GetHeightMapValue(int x, int y);

//void CL_AddSelectedEntitiesToMessage(clientMessage_t *msg) {
//    msg->num_entities = 0;
//    FOR_LOOP(entindex, cl.num_entities) {
//        clientEntity_t *e = &cl.ents[entindex];
//        if (e->selected) {
//            msg->entities[msg->num_entities++] = e->current.number;
//        }
//    }
//}

bool CL_ProcessEntitySelection(clientEntity_t *entity) {
    if (entity->current.player == cl.playerNumber) {
        CL_SelectEntity(entity->current.number);
        return true;
    }
    if (entity->current.player > 0) {
//        msg.targetentity = entity->current.number;
//        msg.cmd = CMD_ATTACK;
//        CL_AddSelectedEntitiesToMessage(&msg);
        return true;
    }
    return false;
}

void CL_SelectEntityAtScreenPoint(DWORD pixelX, DWORD pixelY) {
    VECTOR3 targetorg;
    LINE3 const line = CL_GetMouseLine(pixelX, pixelY);
    FOR_LOOP(entindex, cl.num_entities) {
        clientEntity_t *e = &cl.ents[entindex];
        if (e->current.player == 0)
            continue;
        if (Line3_intersect_sphere3(&line, &(const SPHERE3) {
            .center = e->current.origin,
            .radius = 50
        }, NULL) && CL_ProcessEntitySelection(e)) {
            return;
        }
    }
    if (CM_IntersectLineWithHeightmap(&line, &targetorg)) {
//        msg.location.x = targetorg.x;
//        msg.location.y = targetorg.y;
//        moveConfirmation_t *mc = &cl.confs[cl.confirmationCounter++ & (MAX_CONFIRMATION_OBJECTS - 1)];
//        mc->origin = targetorg;
//        mc->timespamp = cl.time;
//        msg.cmd = CMD_MOVE;
//        CL_AddSelectedEntitiesToMessage(&msg);
    }
}

bool rect_contains(LPCRECT rect, LPCVECTOR2 vec) {
    BOX3 box = {
        { MIN(rect->x, rect->x + rect->width), MIN(rect->y, rect->y + rect->height)  },
        { MAX(rect->x, rect->x + rect->width), MAX(rect->y, rect->y + rect->height) },
    };
    if (box.min.x > vec->x) return false;
    if (box.min.y > vec->y) return false;
    if (box.max.x <= vec->x) return false;
    if (box.max.y <= vec->y) return false;
    return true;
}

void CL_SelectEntitiesAtScreenRect(LPCRECT rect) {
    MATRIX4 m;
    SIZE2 const windowSize = re.GetWindowSize();
    Matrix4_getCameraMatrix(&cl.viewDef.camera, &m);
    CL_SelectEntity(-1);
    FOR_LOOP(entindex, cl.num_entities) {
        clientEntity_t *e = &cl.ents[entindex];
        if (e->current.player != cl.playerNumber)
            continue;
        VECTOR3 pos;
        Matrix4_multiply_vector3(&m, &e->current.origin, &pos);
        pos.x = windowSize.width * (pos.x + 1.0f) / 2.f;
        pos.y = windowSize.height * (1.0f - pos.y) / 2.f;
        if (rect_contains(rect, (LPCVECTOR2)&pos)) {
            e->selected = true;
        }
    }
}
