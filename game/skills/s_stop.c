#include "s_skills.h"

ability_t a_stop = {
//    .cmd = build_command,
};

void order_stop(LPEDICT ent) {
    unit_leavecombat(ent);
    ent->stand(ent);
}
