#include "g_local.h"

#define CLIENTCOMMAND(NAME) void CMD_##NAME(edict_t *clent, DWORD argc, LPCSTR argv[])

edict_t *G_GetMainSelectedUnit(gclient_t *client) {
    FOR_SELECTED_UNITS(client, ent) {
        return ent;
    }
    return NULL;
}

void G_SelectEntity(gclient_t *client, edict_t *ent) {
    ent->selected |= 1 << client->ps.number;
}

void G_DeselectEntity(gclient_t *client, edict_t *ent) {
    ent->selected &= ~(1 << client->ps.number);
}

bool G_IsEntitySelected(gclient_t *client, edict_t *ent) {
    return ent->selected & (1 << client->ps.number);
}

void CMD_CancelCommand(edict_t *ent) {
    Get_Commands_f(ent);
}

CLIENTCOMMAND(Select) {
    gclient_t *client = clent->client;
    if (client->menu.on_entity_selected) {
        DWORD number = atoi(argv[1]);
        if (number >= globals.num_edicts)
            return;
        if (client->menu.on_entity_selected(clent, &globals.edicts[number])) {
            Get_Commands_f(clent);
        }
    } else {
        bool selected = false;
        for (DWORD i = 1; i < argc; i++) {
            DWORD number = atoi(argv[i]);
            if (number >= globals.num_edicts)
                continue;
            edict_t *e = &globals.edicts[number];
            if (e->s.player == client->ps.number) {
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
    gclient_t *client = clent->client;
    if (client->menu.on_location_selected) {
        VECTOR2 loc = { atoi(argv[1]), atoi(argv[2]) };
        if (client->menu.on_location_selected(clent, &loc)) {
            Get_Commands_f(clent);
        }
    }
}

CLIENTCOMMAND(Button) {
    DWORD code = atoi(argv[1]);
    gclient_t *client = clent->client;
    ability_t *ability = GetAbilityByIndex(code);
    if (ability && ability->cmd) {
        ability->cmd(clent);
    } else if (client->menu.cmdbutton) {
        client->menu.cmdbutton(clent, code);
    } else {
        edict_t *ent = G_GetMainSelectedUnit(client);
        LPCSTR builds = UNIT_TRAINS(ent->class_id);
        if (!builds)
            return;
        PARSE_LIST(builds, build, getNextSegment) {
            if (*((DWORD *)build) == code) {
                SP_TrainUnit(&client->ps, ent, code);
                break;
            }
        }
    }
}

typedef struct {
    LPCSTR name;
    void (*func)(edict_t *ent, DWORD argc, LPCSTR argv[]);
} clientCommand_t;

clientCommand_t clientCommands[] = {
    { "button", CMD_Button },
    { "select", CMD_Select },
    { "point", CMD_Point },
    { NULL }
};

void G_ClientCommand(edict_t *ent, DWORD argc, LPCSTR argv[]) {
    for (clientCommand_t const *cmd = clientCommands; cmd->name; cmd++) {
        if (!strcmp(cmd->name, argv[0])) {
            cmd->func(ent, argc, argv);
            return;
        }
    }
}
