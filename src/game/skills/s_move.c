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

void move_start(edict_t *self, edict_t *target) {
    self->goalentity = target;
 
    M_SetMove(self, &move_move_walk);
}

void move_selectlocation(edict_t *ent, LPCVECTOR2 mouse) {
    gclient_t *client = ent->client;
    VECTOR2 location = client_getpointlocation(client, mouse);
    handle_t heatmap = gi.BuildHeatmap(&location);
    edict_t *waypoint = Waypoint_add(&location);
    ability_t const *ability = GetAbilityByIndex(client->lastcode);
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *ent = &globals.edicts[i];
        if (!client_isentityselected(client, ent))
            continue;
        ability->use(ent, waypoint);
        ent->heatmap = heatmap;
    }
}

void move_command(edict_t *edict) {
    uiFrameDef_t *layer = UI_Clear();
    UI_AddAbilityButton(layer, STR_CmdCancel);
    UI_WriteLayout(edict, layer, UI_COMMANDBAR);
    edict->client->menu.mouseup = move_selectlocation;
}

void SP_ability_move(ability_t *self) {
    self->use = move_start;
    self->cmd = move_command;
}
