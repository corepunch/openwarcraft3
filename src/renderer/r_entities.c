#include "r_local.h"

void R_DrawEntities(void) {
    FOR_LOOP(i, tr.viewDef.num_entities) {
        LPCRENDERENTITY lpEdict = &tr.viewDef.entities[i];
        if (!R_IsPointVisible(&lpEdict->origin, 1.25f))
            continue;
        RenderModel(lpEdict);
    }
}
