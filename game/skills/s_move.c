/*
 * s_move.c — Move ability: ground movement orders for units.
 *
 * When a player right-clicks on empty ground, move_selectlocation() is called
 * on the server.  It creates a waypoint entity at the target position and
 * calls order_move() for each selected unit.
 *
 * order_move() sets the unit's goalentity to the waypoint and switches to the
 * movement state.  Each game frame, ai_move_walk() checks the remaining distance: if
 * the unit has arrived it switches to the stand (idle) state; otherwise it
 * rotates toward the goal and advances by one frame's worth of movement.
 *
 * Collision separation between walking units is handled in g_phys.c
 * (G_SolveCollisions) after all units have moved.
 */
#include "s_skills.h"

void move_walk(LPEDICT ent);

#define MOVE_BLOCKED_FRAMES 8
#define MOVE_SETTLE_FRAMES 4
#define MOVE_SLOT_MARGIN 8.0f
#define MOVE_MIN_SLOT_SPACING 16.0f
#define MOVE_ARRIVE_TOLERANCE 4.0f

typedef struct {
    VECTOR2 point;
    FLOAT radius;
} moveSlot_t;

static FLOAT move_slot_spacing(LPEDICT const *units, DWORD count) {
    FLOAT max_radius = 0;
    FOR_LOOP(i, count) {
        max_radius = MAX(max_radius, units[i]->collision);
    }
    return MAX(MOVE_MIN_SLOT_SPACING, max_radius * 2 + MOVE_SLOT_MARGIN);
}

static BOOL move_slot_overlaps(LPCVECTOR2 point,
                               FLOAT radius,
                               moveSlot_t const *reserved,
                               DWORD num_reserved) {
    FOR_LOOP(i, num_reserved) {
        FLOAT min_distance = radius + reserved[i].radius + MOVE_SLOT_MARGIN;
        if (Vector2_distance(point, &reserved[i].point) < min_distance) {
            return true;
        }
    }
    return false;
}

static BOOL move_try_slot(LPCVECTOR2 point,
                          FLOAT radius,
                          moveSlot_t const *reserved,
                          DWORD num_reserved,
                          LPVECTOR2 out) {
    VECTOR2 pathable = *point;
    if (!gi.ClosestPathablePointForRadius(point, radius, &pathable)) {
        return false;
    }
    if (move_slot_overlaps(&pathable, radius, reserved, num_reserved)) {
        return false;
    }
    *out = pathable;
    return true;
}

static BOOL move_find_reserved_slot(LPCVECTOR2 location,
                                    LPCVECTOR2 preferred,
                                    FLOAT radius,
                                    FLOAT spacing,
                                    DWORD unit_count,
                                    moveSlot_t const *reserved,
                                    DWORD num_reserved,
                                    LPVECTOR2 out) {
    FLOAT best_distance = 0;
    BOOL found = false;
    VECTOR2 best = *location;
    int max_ring = (int)ceilf(sqrtf(MAX(1, unit_count))) + 8;

    if (move_try_slot(preferred, radius, reserved, num_reserved, out)) {
        return true;
    }

    for (int ring = 0; ring <= max_ring; ring++) {
        int min = -ring;
        int max = ring;
        for (int y = min; y <= max; y++) {
            for (int x = min; x <= max; x++) {
                if (ring > 0 && x != min && x != max && y != min && y != max) {
                    continue;
                }
                VECTOR2 candidate = {
                    location->x + x * spacing,
                    location->y + y * spacing,
                };
                VECTOR2 pathable;
                FLOAT distance;

                if (!move_try_slot(&candidate, radius, reserved, num_reserved, &pathable)) {
                    continue;
                }

                distance = Vector2_distance(&pathable, preferred);
                if (!found || distance < best_distance) {
                    best_distance = distance;
                    best = pathable;
                    found = true;
                }
            }
        }
        if (found) {
            *out = best;
            return true;
        }
    }
    return false;
}

static VECTOR2 move_preferred_slot(LPEDICT ent,
                                   LPCVECTOR2 group_center,
                                   LPCVECTOR2 location,
                                   FLOAT spacing,
                                   DWORD unit_count) {
    VECTOR2 offset = Vector2_sub(&ent->s.origin2, group_center);
    FLOAT max_offset = spacing * (sqrtf(MAX(1, unit_count)) + 1);
    FLOAT len = Vector2_len(&offset);
    if (len > max_offset && len > 0.001f) {
        offset = Vector2_scale(&offset, max_offset / len);
    }
    return Vector2_add(location, &offset);
}

static DWORD move_collect_selected(LPGAMECLIENT client,
                                   LPEDICT *units,
                                   DWORD max_units,
                                   LPVECTOR2 center) {
    DWORD count = 0;
    *center = MAKE(VECTOR2, 0, 0);

    FOR_SELECTED_UNITS(client, ent) {
        if (count >= max_units) {
            break;
        }
        units[count++] = ent;
        center->x += ent->s.origin2.x;
        center->y += ent->s.origin2.y;
    }

    if (count > 0) {
        center->x /= count;
        center->y /= count;
    }
    return count;
}

static void move_reset_progress(LPEDICT self) {
    self->move_last_origin = self->s.origin2;
    self->move_last_distance = -1;
    self->move_blocked_frames = 0;
}

static BOOL move_should_arrive(LPEDICT ent, FLOAT move_distance) {
    VECTOR2 to_goal = Vector2_sub(&ent->goalentity->s.origin2, &ent->s.origin2);
    FLOAT distance = Vector2_len(&to_goal);

    if (distance <= move_distance) {
        return true;
    }

    /*
     * If the goal lies within this frame's movement corridor, snap to it
     * rather than letting the unit wobble around the destination.  This keeps
     * short path segments and near-goal collision nudges from producing a
     * visible back-and-forth at the endpoint.
     */
    VECTOR2 direction = { cosf(ent->s.angle), sinf(ent->s.angle) };
    FLOAT projected = Vector2_dot(&to_goal, &direction);
    if (projected < 0 || projected > move_distance + MOVE_ARRIVE_TOLERANCE) {
        return false;
    }

    FLOAT lateral = fabsf(to_goal.x * direction.y - to_goal.y * direction.x);
    return lateral <= MAX(MOVE_ARRIVE_TOLERANCE, ent->collision + MOVE_SLOT_MARGIN);
}

static BOOL move_is_blocked(LPEDICT ent, FLOAT distance, FLOAT move_distance) {
    if (ent->move_last_distance >= 0) {
        FLOAT progress = ent->move_last_distance - distance;
        FLOAT moved = Vector2_distance(&ent->s.origin2, &ent->move_last_origin);
        FLOAT min_progress = MAX(1.0f, move_distance * 0.05f);
        FLOAT min_moved = MAX(1.0f, move_distance * 0.25f);
        FLOAT settle_distance = move_distance + ent->collision + MOVE_SLOT_MARGIN;

        if (distance <= settle_distance && progress < min_progress) {
            ent->move_blocked_frames++;
        } else if (progress < min_progress && moved < min_moved) {
            ent->move_blocked_frames++;
        } else {
            ent->move_blocked_frames = 0;
        }
    }

    ent->move_last_distance = distance;
    ent->move_last_origin = ent->s.origin2;
    return distance <= move_distance + ent->collision + MOVE_SLOT_MARGIN
        ? ent->move_blocked_frames >= MOVE_SETTLE_FRAMES
        : ent->move_blocked_frames >= MOVE_BLOCKED_FRAMES;
}

static void ai_move_walk(LPEDICT ent) {
    FLOAT distance = M_DistanceToGoal(ent);
    FLOAT move_distance = unit_movedistance(ent);

    if (move_should_arrive(ent, move_distance)) {
        ent->s.origin2 = ent->goalentity->s.origin2;
        gi.LinkEntity(ent);
        ent->stand(ent);
    } else if (move_is_blocked(ent, distance, move_distance)) {
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
    move_reset_progress(self);
    unit_setmove(self, &move_move_walk);
}

/* Handle a right-click move command from the client.
 * Creates a shared waypoint at the clicked map position, issues move orders
 * to all currently selected units, and sends a move-confirmation effect back
 * to the commanding client (svc_temp_entity / TE_MOVE_CONFIRMATION). */
BOOL move_selectlocation(LPEDICT clent, LPCVECTOR2 location) {
    LPEDICT units[MAX_SELECTED_ENTITIES];
    moveSlot_t reserved[MAX_SELECTED_ENTITIES];
    VECTOR2 center;
    VECTOR2 confirmation = *location;
    BOOL have_confirmation = false;
    DWORD num_units = move_collect_selected(clent->client, units, MAX_SELECTED_ENTITIES, &center);
    FLOAT spacing = move_slot_spacing(units, num_units);

    FOR_LOOP(i, num_units) {
        LPEDICT ent = units[i];
        VECTOR2 preferred = move_preferred_slot(ent, &center, location, spacing, num_units);
        VECTOR2 target;

        if (!move_find_reserved_slot(location,
                                     &preferred,
                                     ent->collision,
                                     spacing,
                                     num_units,
                                     reserved,
                                     i,
                                     &target)) {
            target = *location;
            gi.ClosestPathablePointForRadius(location, ent->collision, &target);
        }
        reserved[i] = (moveSlot_t){ target, ent->collision };
        LPEDICT waypoint = Waypoint_add(&target);
        if (!have_confirmation) {
            confirmation = target;
            have_confirmation = true;
        }
        order_move(ent, waypoint);
    }
    gi.Write(PF_BYTE, &(LONG){svc_temp_entity});
    gi.Write(PF_BYTE, &(LONG){TE_MOVE_CONFIRMATION});
    gi.Write(PF_POSITION, &(VECTOR3){confirmation.x, confirmation.y, 0});
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
