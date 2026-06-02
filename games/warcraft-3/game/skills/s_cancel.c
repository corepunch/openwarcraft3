#include "s_skills.h"

void cancel_command(LPEDICT ent) {
    Get_Commands_f(ent);
}

ability_t a_cancel = {
    .cmd = cancel_command,
};
