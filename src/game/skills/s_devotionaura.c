#include "s_skills.h"

static LPCSTR TARGET_ART;

#define ID_DEVOTION_AURA "AHad"

void devotionaura_command(LPEDICT clent) {
    LPEDICT unit = G_GetMainSelectedUnit(clent->client);
    unit_addstatus(unit, ID_DEVOTION_AURA, 1);
    unit->s.model2 = gi.ModelIndex(TARGET_ART);
    Get_Commands_f(clent);
}

void SP_ability_devotionaura(LPCSTR classname, ability_t *self) {
    TARGET_ART = FindConfigValue(classname, STR_TARGET_ART);
}

ability_t a_devotionaura = {
    .cmd = devotionaura_command,
    .init = SP_ability_devotionaura,
};
