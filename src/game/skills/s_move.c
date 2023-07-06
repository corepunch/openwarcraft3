#include "g_local.h"

EDICT_FUNC(move_walk);

static EDICT_FUNC(ai_walk) {
    if (M_DistanceToGoal(ent) <= M_MoveDistance(ent)) {
        ent->stand(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static umove_t move_move_walk = { "walk", ai_walk };

void move_move(edict_t *self, edict_t *target) {
    self->goalentity = target;
    M_SetMove(self, &move_move_walk);
}

bool move_selectlocation(edict_t *clent, LPCVECTOR2 location) {
    edict_t *waypoint = Waypoint_add(location);
    FOR_SELECTED_UNITS(clent->client, ent) {
        move_move(ent, waypoint);
    }
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_MOVE_CONFIRMATION);
    gi.WritePosition(&waypoint->s.origin);
    gi.unicast(clent);
    return true;
}

void move_command(edict_t *edict) {
    uiFrameDef_t *layer = UI_EmptyScreen();
    UI_AddAbilityButton(layer, STR_CmdCancel);
    UI_WriteLayout(edict, layer, LAYER_COMMANDBAR);
    edict->client->menu.on_location_selected = move_selectlocation;
}

void SP_ability_move(ability_t *self) {
    self->cmd = move_command;
}
