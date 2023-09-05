#include "g_local.h"

BOOL jass_calltrigger(LPJASS j, LPTRIGGER trigger);

static void G_ExecuteEvent(GAMEEVENT *evt) {
    HANDLE subject = evt->edict;
    if (evt->type >= EVENT_PLAYER_STATE_LIMIT &&
        evt->type <= EVENT_PLAYER_END_CINEMATIC)
    {
        subject = (LPMAPPLAYER)level.mapinfo->players+evt->edict->client->ps.number;
    }
    FOR_EACH_LIST(EVENT, e, level.events.handlers) {
        switch (e->type) {
            case EVENT_GAME_VICTORY:
                break;
            case EVENT_GAME_END_LEVEL:
                break;
            case EVENT_GAME_VARIABLE_LIMIT:
                break;
            case EVENT_GAME_STATE_LIMIT:
                break;
            case EVENT_GAME_TIMER_EXPIRED:
                break;
            case EVENT_GAME_ENTER_REGION:
                if (e->type == evt->type && evt->region == &e->region) {
                    extern LPEDICT enteringunit;
                    enteringunit = subject;
                    printf("%.4s entered region\n", (LPCSTR)&enteringunit->class_id);
                    jass_calltrigger(level.vm, e->trigger);
                    enteringunit = NULL;
                }
                break;
            case EVENT_GAME_LEAVE_REGION:
                if (e->type == evt->type && evt->region == &e->region) {
                    extern LPEDICT leavingunit;
                    leavingunit = subject;
                    jass_calltrigger(level.vm, e->trigger);
                    leavingunit = NULL;
                }
                break;
            case EVENT_GAME_TRACKABLE_HIT:
                break;
            case EVENT_GAME_TRACKABLE_TRACK:
                break;
            case EVENT_GAME_SHOW_SKILL:
                break;
            case EVENT_GAME_BUILD_SUBMENU:
                break;
            default:
                if (e->subject == subject && e->type == evt->type) {
                    jass_calltrigger(level.vm, e->trigger);
                }
                break;
        }
    }
}

static void G_TriggerRegionEvents(LPEDICT ent) {
    FOR_EACH_LIST(EVENT, evt, level.events.handlers) {
        if (evt->type == EVENT_GAME_ENTER_REGION) {
            if (G_RegionContains(&evt->region, &ent->s.origin2) &&
                !G_RegionContains(&evt->region, &ent->old_origin))
            {
                G_PublishEvent(ent, evt->type)->region = &evt->region;
            }
        } else if (evt->type == EVENT_GAME_LEAVE_REGION) {
            if (!G_RegionContains(&evt->region, &ent->s.origin2) &&
                G_RegionContains(&evt->region, &ent->old_origin))
            {
                G_PublishEvent(ent, evt->type)->region = &evt->region;
            }
        }
    }
}

void G_RunEntities(void) {
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = globals.edicts+i;
        ent->old_origin = ent->s.origin2;
    }
    FOR_LOOP(i, globals.num_edicts) {
        G_RunEntity(globals.edicts+i);
    }
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = globals.edicts+i;
        if (memcmp(&ent->old_origin, &ent->s.origin2, sizeof(VECTOR2)) != 0) {
            G_TriggerRegionEvents(ent);
            ent->old_origin = ent->s.origin2;
        }
    }
}

void G_RunEvents(void) {
    for (LEVELEVENTS *e = &level.events; e->read < e->write; e->read++) {
        GAMEEVENT *evt = &e->queue[e->read % MAX_EVENT_QUEUE];
        G_ExecuteEvent(evt);
    }
}
