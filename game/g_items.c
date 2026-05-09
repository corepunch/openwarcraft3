#include "g_local.h"

void SP_SpawnItem(LPEDICT self) {
    PATHSTR model_filename;
    strcpy(model_filename, ITEM_FILE(self->class_id));
    fprintf(stderr, "SP_SpawnItem: class=%.4s model=%s\n", (const char *)&self->class_id, model_filename);
    self->s.model = gi.ModelIndex(model_filename);
    self->movetype = MOVETYPE_NONE;
}
