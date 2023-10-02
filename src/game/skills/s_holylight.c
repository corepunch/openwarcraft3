#include "s_skills.h"

static LPCSTR TARGET_ART;

void holylight_command(LPEDICT clent) {
    LPEDICT unit = G_GetMainSelectedUnit(clent->client);
    unit->s.model2 = gi.ModelIndex(TARGET_ART);
    Get_Commands_f(clent);
}

void SP_ability_holylight(LPCSTR classname, ability_t *self) {
    TARGET_ART = FindConfigValue(classname, STR_TARGET_ART);
}

ability_t a_holylight = {
    .cmd = holylight_command,
    .init = SP_ability_holylight,
};
