#include "g_local.h"

void G_FreeEdict(LPEDICT ent) {
    memset(ent, 0, sizeof(*ent));
    ent->freetime = level.time;
}

LPEVENT G_MakeEvent(EVENTTYPE type) {
    LPEVENT evt = gi.MemAlloc(sizeof(EVENT));
    evt->type = type;
    ADD_TO_LIST(evt, level.events.handlers);
    return evt;
}

BOOL G_RegionContains(LPCREGION region, LPCVECTOR2 point) {
    FOR_LOOP(i, region->num_rects) {
        if (Box2_containsPoint(region->rects+i, point)) {
            return true;
        }
    }
    return false;
}
