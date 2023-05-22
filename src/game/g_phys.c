#include "g_local.h"

void SV_Physics_Step(LPEDICT edict) {
    M_CheckGround(edict);

}

void G_RunEntity(LPEDICT edict) {
    SAFE_CALL(edict->prethink, edict);
    switch (edict->movetype) {
        case MOVETYPE_STEP:
            SV_Physics_Step(edict);
            break;
        default:
//            gi.error("SV_Physics: bad movetype %d", edict->movetype);
            break;
    }
    SAFE_CALL(edict->think, edict);
}
