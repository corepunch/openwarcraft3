#include "s_skills.h"

static LPCSTR TARGET_ART;

void holylight_done(LPEDICT self);

static umove_t effect_move_birth = { "birth", NULL, G_FreeEdict };
static umove_t move_heal = { "stand channel", ai_idle, holylight_done, &a_holylight };

void holylight_done(LPEDICT self) {
    self->stand(self);
}

void holylight_command(LPEDICT clent) {
    LPEDICT unit = G_GetMainSelectedUnit(clent->client);
    
    LPEDICT effect = G_Spawn();
    effect->s.origin = unit->s.origin;
    effect->s.angle = unit->s.angle;
    effect->s.model = gi.ModelIndex(TARGET_ART);
    effect->goalentity = unit;
    effect->movetype = MOVETYPE_LINK;
    effect->think = M_MoveFrame;
    
    M_SetMove(effect, &effect_move_birth);

    M_SetMove(unit, &move_heal);
    Get_Commands_f(clent);
}

void SP_ability_holylight(LPCSTR classname, ability_t *self) {
    TARGET_ART = FindConfigValue(classname, STR_TARGET_ART);
}

ability_t a_holylight = {
    .cmd = holylight_command,
    .init = SP_ability_holylight,
};
