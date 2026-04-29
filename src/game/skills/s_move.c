/*
 * s_move.c — Move ability: ground movement orders for units.
 *
 * When a player right-clicks on empty ground, move_selectlocation() is called
 * on the server.  It creates a waypoint entity at the target position and
 * calls order_move() for each selected unit.
 *
 * order_move() sets the unit's goalentity to the waypoint and switches to the
 * walk state.  Each game frame, ai_walk() checks the remaining distance: if
 * the unit has arrived it switches to the stand (idle) state; otherwise it
 * rotates toward the goal and advances by one frame's worth of movement.
 *
 * Collision separation between walking units is handled in g_phys.c
 * (G_SolveCollisions) after all units have moved.
 */
#include "s_skills.h"

void move_walk(LPEDICT ent);

static void ai_move_walk(LPEDICT ent) {
    if (M_DistanceToGoal(ent) <= unit_movedistance(ent)) {
        ent->stand(ent);
    } else {
        unit_changeangle(ent);
        unit_moveindirection(ent);
    }
}

static umove_t move_move_walk = { "walk", ai_move_walk, NULL, &a_move };

/* Set the unit's move target and begin walking.
 * goalentity must be a waypoint or any entity whose origin is the destination. */
void order_move(LPEDICT self, LPEDICT target) {
    self->goalentity = target;
    unit_setmove(self, &move_move_walk);
}

/* Handle a right-click move command from the client.
 * Creates a shared waypoint at the clicked map position, issues move orders
 * to all currently selected units, and sends a move-confirmation effect back
 * to the commanding client (svc_temp_entity / TE_MOVE_CONFIRMATION). */
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
