#include "s_skills.h"

#define ID_DEVOTION_AURA "AHad"

static LPCSTR TARGET_ART;

static umove_t aura_move_stand = { "stand", ai_idle, NULL };
static umove_t aura_move_death = { "death", NULL, G_FreeEdict };

void devotionaura_command(LPEDICT clent) {
    LPEDICT unit = G_GetMainSelectedUnit(clent->client);
    unit_addstatus(unit, ID_DEVOTION_AURA, 1);
    
    LPEDICT effect = G_Spawn();
    effect->s.origin = unit->s.origin;
    effect->s.angle = unit->s.angle;
    effect->s.model = gi.ModelIndex(TARGET_ART);
    effect->goalentity = unit;
    effect->movetype = MOVETYPE_LINK;
    effect->think = M_MoveFrame;
    
    M_SetMove(effect, &aura_move_stand);
    
    Get_Commands_f(clent);
}

void SP_ability_devotionaura(LPCSTR classname, ability_t *self) {
    TARGET_ART = FindConfigValue(classname, STR_TARGET_ART);
}

ability_t a_devotionaura = {
    .cmd = devotionaura_command,
    .init = SP_ability_devotionaura,
};
