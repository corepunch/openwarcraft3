#include <stdlib.h>

#include "g_local.h"

#include "Units/UnitAbilities.h"
#include "Units/AbilityData.h"
//
//static void G_ClientCommandMove(clientMessage_t const *clientMessage) {
//    handle_t heatmap = gi.BuildHeatmap(&clientMessage->location);
//    edict_t *waypoint = Waypoint_add(clientMessage->location);
//    FOR_LOOP(i, clientMessage->num_entities) {
//        handle_t entnum = clientMessage->entities[i];
//        if (entnum >= globals.num_edicts)
//            continue;
//        globals.edicts[entnum].goalentity = waypoint;
//        globals.edicts[entnum].heatmap = heatmap;
//    }
//}
//
//static void G_ClientCommandAttack(clientMessage_t const *clientMessage) {
//    edict_t *enemy = &game_state.edicts[clientMessage->targetentity];
//    handle_t heatmap = gi.BuildHeatmap((LPCVECTOR2)&enemy->s.origin);
//    FOR_LOOP(i, clientMessage->num_entities) {
//        handle_t entnum = clientMessage->entities[i];
//        if (entnum >= globals.num_edicts)
//            continue;
//        if (globals.edicts[entnum].unitinfo.weapon) {
//            globals.edicts[entnum].enemy = enemy;
//            globals.edicts[entnum].goalentity = enemy;
//            globals.edicts[entnum].heatmap = heatmap;
//        }
//    }
//}
//
//void G_ClientCommand2(clientMessage_t const *clientMessage) {
//    switch (clientMessage->cmd) {
//        case CMD_MOVE:
//            G_ClientCommandMove(clientMessage);
//            break;
//        case CMD_ATTACK:
//            G_ClientCommandAttack(clientMessage);
//            break;
//        default:
//            break;
//    }
//}

#define CLIENTCOMMAND(NAME) void CMD_##NAME(edict_t *ent, DWORD argc, LPCSTR argv[])

CLIENTCOMMAND(Move) {
}

CLIENTCOMMAND(Inventory) {
    if (argc < 2)
        return;
    char string[1024] = { 0 };
    DWORD entity_id = atoi(argv[1]);
    if (entity_id > globals.num_edicts)
        return;
    gi.WriteByte(svc_layout);
    edict_t *e = &game_state.edicts[entity_id];
    sprintf(string, "inventory ");
    PARSE_LIST(e->unitinfo.abilities->abilList, abil, gi.ParserGetToken) {
        struct AbilityData const *adata = FindAbilityData(*(DWORD const*)abil);
        if (adata) {
            sprintf(string + strlen(string), "%.4s,", (LPCSTR)&adata->code);
        } else {
            gi.error("Can't find ability %s", abil);
        }
    }
    gi.WriteString(string);
    gi.unicast(ent);
}

typedef struct {
    LPCSTR name;
    void (*func)(edict_t *ent, DWORD argc, LPCSTR argv[]);
} clientCommand_t;

clientCommand_t clientCommands[] = {
    { "inventory", CMD_Inventory },
    { "move", CMD_Move },
    { NULL }
};

void G_ClientCommand(edict_t *ent, DWORD argc, LPCSTR argv[]) {
    for (clientCommand_t const *cmd = clientCommands; cmd->name; cmd++) {
        if (!strcmp(cmd->name, argv[0])) {
            cmd->func(ent, argc, argv);
            break;
        }
    }
}
