#include "g_local.h"

void SV_Physics_Step(LPEDICT lpEdict) {
    M_CheckGround(lpEdict);
    
}

void G_RunEntity(LPEDICT lpEdict) {
    SAFE_CALL(lpEdict->prethink, lpEdict);
    switch (lpEdict->movetype) {
        case MOVETYPE_STEP:
            SV_Physics_Step(lpEdict);
            break;
        default:
//            gi.error("SV_Physics: bad movetype %d", lpEdict->movetype);
            break;
    }
    SAFE_CALL(lpEdict->think, lpEdict);
}

