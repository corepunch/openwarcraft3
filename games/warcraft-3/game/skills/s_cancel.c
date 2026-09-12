#include "s_skills.h"

void cancel_command(LPEDICT ent) {
    CMD_CancelCommand(ent);
}

BZ_COMMAND_PROC(AbilityCancel, cancel_command)
