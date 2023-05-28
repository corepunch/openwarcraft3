#include "r_local.h"

void R_DrawEntities(void) {
    FOR_LOOP(i, tr.viewDef.num_entities) {
        renderEntity_t const *edict = &tr.viewDef.entities[i];
        if (!(tr.viewDef.rdflags & RDF_NOFRUSTUMCULL) &&
            !R_IsPointVisible(&edict->origin, 1.25f))
            continue;
        RenderModel(edict);
    }
}
