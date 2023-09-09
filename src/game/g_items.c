#include "g_local.h"

void SP_SpawnItem(LPEDICT self) {
    PATHSTR model_filename;
    strcpy(model_filename, ITEM_FILE(self->class_id));
    if (!strstr(model_filename, ".mdx")) {
        LPSTR mdl = strstr(model_filename, ".mdl");
        mdl = mdl ? mdl : (model_filename + strlen(model_filename));
        strcpy(mdl, ".mdx");
    }
    self->s.model = gi.ModelIndex(model_filename);
}
