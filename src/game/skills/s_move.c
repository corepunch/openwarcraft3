#include "s_skills.h"

void move_walk(LPEDICT ent);

static void ai_walk(LPEDICT ent) {
    if (M_DistanceToGoal(ent) <= M_MoveDistance(ent)) {
        ent->stand(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static umove_t move_move_walk = { "walk", ai_walk, NULL, &a_move };

void order_move(LPEDICT self, LPEDICT target) {
    self->goalentity = target;
    M_SetMove(self, &move_move_walk);
}

BOOL move_selectlocation(LPEDICT clent, LPCVECTOR2 location) {
    LPEDICT waypoint = Waypoint_add(location);
    FOR_SELECTED_UNITS(clent->client, ent) {
        order_move(ent, waypoint);
    }
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_MOVE_CONFIRMATION);
    gi.WritePosition(&waypoint->s.origin);
    gi.unicast(clent);
    return true;
}

void move_command(LPEDICT ent) {
    UI_AddCancelButton(ent);
    ent->client->menu.on_location_selected = move_selectlocation;
}

ability_t a_move = {
    .cmd = move_command,
};
