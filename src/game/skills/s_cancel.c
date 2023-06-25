#include "g_local.h"

void cancel_command(edict_t *edict) {
    Get_Commands_f(edict);
}

void SP_ability_cancel(ability_t *self) {
    self->cmd = cancel_command;
}
