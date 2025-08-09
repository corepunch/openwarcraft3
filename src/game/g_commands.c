#include "g_local.h"

#define CLIENTCOMMAND(NAME) void CMD_##NAME(LPEDICT clent, DWORD argc, LPCSTR argv[])

LPEDICT G_GetMainSelectedUnit(LPGAMECLIENT client) {
    FOR_SELECTED_UNITS(client, ent) {
        return ent;
    }
    return NULL;
}

void G_SelectEntity(LPGAMECLIENT client, LPEDICT ent) {
    ent->selected |= 1 << client->ps.number;
}

void G_DeselectEntity(LPGAMECLIENT client, LPEDICT ent) {
    ent->selected &= ~(1 << client->ps.number);
}

BOOL G_IsEntitySelected(LPGAMECLIENT client, LPEDICT ent) {
    return ent->selected & (1 << client->ps.number);
}

void CMD_CancelCommand(LPEDICT ent) {
    Get_Commands_f(ent);
}

CLIENTCOMMAND(Select) {
    LPGAMECLIENT client = clent->client;
    if (client->menu.on_entity_selected) {
        DWORD number = atoi(argv[1]);
        if (number >= globals.num_edicts)
            return;
        if (client->menu.on_entity_selected(clent, &globals.edicts[number])) {
            Get_Commands_f(clent);
        }
    } else {
        BOOL selected = false;
        BOOL hasunits = false;
        for (DWORD i = 1; i < argc; i++) {
            DWORD number = atoi(argv[i]);
            if (number >= globals.num_edicts)
                continue;
            LPEDICT e = &globals.edicts[number];
            if (!UNIT_IS_BUILDING(e->class_id)) {
                hasunits = true;
            }
        }
        for (DWORD i = 1; i < argc; i++) {
            DWORD number = atoi(argv[i]);
            if (number >= globals.num_edicts)
                continue;
            LPEDICT e = &globals.edicts[number];
            if (e->s.player == client->ps.number) {
                if (hasunits && UNIT_IS_BUILDING(e->class_id))
                    continue;
                if (!selected) {
                    FOR_SELECTED_UNITS(client, ent) G_DeselectEntity(client, ent);
                    selected = true;
                }
                G_SelectEntity(client, e);
            }
        }
        if (selected) {
            Get_Portrait_f(clent);
            Get_Commands_f(clent);
        }
    }
}

CLIENTCOMMAND(Point) {
    LPGAMECLIENT client = clent->client;
    if (client->menu.on_location_selected) {
        VECTOR2 loc = { atoi(argv[1]), atoi(argv[2]) };
        if (client->menu.on_location_selected(clent, &loc)) {
            Get_Commands_f(clent);
        }
    }
}

CLIENTCOMMAND(Button) {
    LPCSTR classname = argv[1];
    LPGAMECLIENT client = clent->client;
    ability_t const *ability = FindAbilityByClassname(classname);
    if (ability && ability->cmd) {
        ability->cmd(clent);
    } else if (client->menu.cmdbutton) {
        client->menu.cmdbutton(clent, *((DWORD *)classname));
    } else {
        LPEDICT ent = G_GetMainSelectedUnit(client);
        LPCSTR builds = UNIT_TRAINS(ent->class_id);
        if (!builds)
            return;
        PARSE_LIST(builds, build, parse_segment) {
            if (!strcmp(build, classname)) {
                SP_TrainUnit(ent, *((DWORD *)classname));
                break;
            }
        }
    }
}

CLIENTCOMMAND(Research) {
    LPCSTR classname = argv[1];
    LPGAMECLIENT client = clent->client;
//    ability_t const *ability = FindAbilityByClassname(classname);
//    if (!ability) {
//        gi.error("No such ability %s", classname);
//        return;
//    }
    LPEDICT ent = G_GetMainSelectedUnit(client);
    DWORD abilcode = *(DWORD const *)classname;
    unit_learnability(ent, abilcode);
    Get_Commands_f(clent);
}

CLIENTCOMMAND(Cancel) {
    G_PublishEvent(clent, EVENT_PLAYER_END_CINEMATIC);
}

void UI_ShowQuests(LPEDICT ent);
void UI_ShowQuest(LPEDICT ent, LPCQUEST quest);
void UI_HideQuests(LPEDICT ent);

CLIENTCOMMAND(Quests) {
    UI_ShowQuests(clent);
}

CLIENTCOMMAND(HideQuests) {
    UI_HideQuests(clent);
}

CLIENTCOMMAND(Quest) {
    DWORD index = atoi(argv[1]);
    FOR_EACH_LIST(QUEST, q, level.quests) {
        if (index == 0) {
            UI_ShowQuest(clent, q);
            break;
        } else {
            index--;
        }
    }
}

typedef struct {
    LPCSTR name;
    void (*func)(LPEDICT ent, DWORD argc, LPCSTR argv[]);
} clientCommand_t;

clientCommand_t clientCommands[] = {
    { "button", CMD_Button },
    { "research", CMD_Research },
    { "select", CMD_Select },
    { "point", CMD_Point },
    { "cancel", CMD_Cancel },
    { "quests", CMD_Quests },
    { "hidequests", CMD_HideQuests },
    { "quest", CMD_Quest },
    { NULL }
};

void G_ClientCommand(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    for (clientCommand_t const *cmd = clientCommands; cmd->name; cmd++) {
        if (!strcmp(cmd->name, argv[0])) {
            cmd->func(ent, argc, argv);
            return;
        }
    }
}

void G_ClientPanCamera(LPEDICT ent, LPVECTOR2 offset) {
    if (ent->client->no_control)
        return;
    ent->client->camera.state.position.x += offset->x;
    ent->client->camera.state.position.y += offset->y;
}
