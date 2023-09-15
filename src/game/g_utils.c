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

LPQUEST G_MakeQuest(void) {
    LPQUEST quest = gi.MemAlloc(sizeof(QUEST));
    PUSH_BACK(QUEST, quest, level.quests);
    return quest;
}

static void DeleteQuestItem(LPQUESTITEM questitem) {
    free(questitem->description);
}

static void DeleteQuest(LPQUEST quest) {
    DELETE_LIST(QUESTITEM, quest->items, DeleteQuestItem);
    free(quest->description);
    free(quest->title);
    gi.MemFree(quest);
}

void G_RemoveQuest(LPQUEST quest) {
    REMOVE_FROM_LIST(QUEST, quest, level.quests, DeleteQuest);
}
