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

void G_SetPlayerAlliance(LPCPLAYER p1, LPCPLAYER p2, PLAYERALLIANCE type, BOOL value) {
    DWORD flag = 1 << type;
    if (value) {
        level.alliances[p1->number][p2->number] |= flag;
        level.alliances[p2->number][p1->number] |= flag;
    } else {
        level.alliances[p1->number][p2->number] &= ~flag;
        level.alliances[p2->number][p1->number] &= ~flag;
    }
}

BOOL G_GetPlayerAlliance(LPCPLAYER p1, LPCPLAYER p2, PLAYERALLIANCE type) {
    return level.alliances[p1->number][p2->number] &= (1 << type);
}
