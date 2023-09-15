#include "../g_local.h"

#define QUEST_ICON_SIZE 0.026
#define QUEST_LIST_SPACING 0.0020
#define QUEST_ITEM_LIST_SPACING 0.0030

static LPCQUEST currentquest;
static LPCQUEST selectedquest;

void UI_QuestListItemIconContainer(LPEDICT ent, LPCFRAMEDEF parent) {
    FRAMEDEF icon;
    LPSTR ext = strstr(currentquest->iconPath, ".tga");
    UI_InitFrame(&icon, FT_TEXTURE);
    UI_SetSize(&icon, QUEST_ICON_SIZE, QUEST_ICON_SIZE);
    UI_SetPoint(&icon, FRAMEPOINT_CENTER, NULL, FRAMEPOINT_CENTER, 0, 0);
    if (ext) {
        memcpy(ext, ".blp", 4);
    }
    if (currentquest->discovered) {
        UI_SetTexture(&icon, currentquest->iconPath, false);
    } else {
        UI_SetTexture(&icon, "UndiscoveredQuestIcon", true);
    }
    UI_WriteFrameWithChildren(&icon, parent);
}

uiTrigger_t questlistitemicon[] = {
    { "QuestListItemIconContainer", UI_QuestListItemIconContainer },
    { NULL },
};

static DWORD QuestIndex(LPCQUEST quest) {
    DWORD index = 0;
    FOR_EACH_LIST(QUEST, q, level.quests) {
        if (q == quest)
            return index;
        index++;
    }
    return index;
}

static void UI_QuestListItem(LPEDICT ent, LPCQUEST quest, LPCFRAMEDEF parent, DWORD index) {
    UI_FRAME(QuestListItem);
    UI_FRAME(QuestListItemComplete);
    UI_FRAME(QuestListItemTitle);
    UI_FRAME(QuestListItemFailedHighlight);
    UI_FRAME(QuestListItemCompletedHighlight);
    UI_FRAME(QuestListItemSelectedHighlight);
    FLOAT y = -index * (QuestListItem->Height + QUEST_LIST_SPACING);
    UI_SetPoint(QuestListItem, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, y);
    UI_SetPoint(QuestListItem, FRAMEPOINT_TOPRIGHT, parent, FRAMEPOINT_TOPRIGHT, 0, y);
    if (quest->discovered) {
        UI_SetTextPointer(QuestListItemTitle, G_LevelString(quest->title));
    } else {
        UI_SetTextPointer(QuestListItemTitle, UI_GetString("UNDISCOVERED_QUEST"));
    }
    UI_SetOnClick(QuestListItem, "quest %d", QuestIndex(quest));
    QuestListItemFailedHighlight->hidden = true;
    QuestListItemCompletedHighlight->hidden = true;
    QuestListItemSelectedHighlight->hidden = quest != selectedquest;
    QuestListItemComplete->hidden = true;
//    UI_Set
    UI_WriteFrameWithChildrenWithTriggers(ent, QuestListItem, parent, questlistitemicon);
}

static void UI_QuestItemListItem(LPEDICT ent, LPCQUESTITEM item, LPCFRAMEDEF parent, DWORD index) {
    UI_FRAME(QuestItemListItem);
    UI_FRAME(QuestItemListItemTitle);
    PATHSTR description;
    FLOAT y = -index * (QuestItemListItem->Height + QUEST_ITEM_LIST_SPACING);
    sprintf(description, " - %s", G_LevelString(item->description));
    UI_SetPoint(QuestItemListItem, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, y);
    UI_SetPoint(QuestItemListItem, FRAMEPOINT_TOPRIGHT, parent, FRAMEPOINT_TOPRIGHT, 0, y);
    UI_SetTextPointer(QuestItemListItemTitle, description);
    UI_WriteFrameWithChildren(QuestItemListItem, parent);
}

void UI_QuestMainContainer(LPEDICT ent, LPCFRAMEDEF parent) {
    DWORD index = 0;
    FOR_EACH_LIST(QUEST, quest, level.quests) {
        if (!quest->required)
            continue;
        currentquest = quest;
        UI_QuestListItem(ent, quest, parent, index++);
    }
}

void UI_QuestOptionalContainer(LPEDICT ent, LPCFRAMEDEF parent) {
    DWORD index = 0;
    FOR_EACH_LIST(QUEST, quest, level.quests) {
        if (quest->required)
            continue;
        currentquest = quest;
        UI_QuestListItem(ent, quest, parent, index++);
    }
}

void UI_QuestItemListContainer(LPEDICT ent, LPCFRAMEDEF parent) {
    DWORD index = 0;
    FOR_EACH_LIST(QUESTITEM, item, selectedquest->items) {
        UI_QuestItemListItem(ent, item, parent, index++);
    }
}

uiTrigger_t questdialog[] = {
    { "QuestMainContainer", UI_QuestMainContainer },
    { "QuestOptionalContainer", UI_QuestOptionalContainer },
    { "QuestItemListContainer", UI_QuestItemListContainer },
    { NULL },
};

void UI_ShowQuest(LPEDICT ent, LPCQUEST quest) {
    if (!quest->discovered)
        return;
    
    if (ent->client->ps.uiflags & (1 << LAYER_QUESTDIALOG))
        return;
    
    UI_FRAME(QuestDialog);
    UI_FRAME(QuestDisplay);
    UI_FRAME(QuestDetailsTitle);
    UI_FRAME(QuestTitleValue);
    UI_FRAME(QuestSubtitleValue);
    UI_FRAME(QuestAcceptButton);
    
    selectedquest = quest;

    if (*level.mapinfo->loadingScreenTitle) {
        UI_SetTextPointer(QuestTitleValue, G_LevelString(level.mapinfo->loadingScreenTitle));
    } else {
        UI_SetTextPointer(QuestTitleValue, " ");
    }

    if (*level.mapinfo->loadingScreenSubtitle) {
        UI_SetTextPointer(QuestSubtitleValue, G_LevelString(level.mapinfo->loadingScreenSubtitle));
    } else {
        UI_SetTextPointer(QuestSubtitleValue, "Quests");
    }

    UI_SetOnClick(QuestAcceptButton, "hidequests");
    UI_SetTextPointer(QuestDetailsTitle, G_LevelString(selectedquest->title));
    UI_SetTextPointer(QuestDisplay, G_LevelString(selectedquest->description));
    UI_WriteWithTriggers(ent, QuestDialog, LAYER_QUESTDIALOG, questdialog);
}

void UI_ShowQuests(LPEDICT ent) {
    FOR_EACH_LIST(QUEST, q, level.quests) {
        if (q->required) {
            UI_ShowQuest(ent, q);
            break;
        }
    }
}

void UI_HideQuests(LPEDICT ent) {
    UI_WriteStart(LAYER_QUESTDIALOG);
    gi.WriteLong(0);
    gi.unicast(ent);
}
