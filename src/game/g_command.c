#include "g_local.h"

static void G_ClientCommandMove(clientMessage_t const *clientMessage) {
    handle_t heatmap = gi.BuildHeatmap(&clientMessage->location);
    LPEDICT waypoint = Waypoint_add(clientMessage->location);
    FOR_LOOP(i, clientMessage->num_entities) {
        handle_t entnum = clientMessage->entities[i];
        if (entnum >= globals.num_edicts)
            continue;
        globals.edicts[entnum].goalentity = waypoint;
        globals.edicts[entnum].heatmap = heatmap;
    }
}

static void G_ClientCommandAttack(clientMessage_t const *clientMessage) {
    LPEDICT enemy = &game_state.edicts[clientMessage->targetentity];
    handle_t heatmap = gi.BuildHeatmap((LPCVECTOR2)&enemy->s.origin);
    FOR_LOOP(i, clientMessage->num_entities) {
        handle_t entnum = clientMessage->entities[i];
        if (entnum >= globals.num_edicts)
            continue;
        if (globals.edicts[entnum].unitinfo.weapon) {
            globals.edicts[entnum].enemy = enemy;
            globals.edicts[entnum].goalentity = enemy;
            globals.edicts[entnum].heatmap = heatmap;
        }
    }
}

void G_ClientCommand(clientMessage_t const *clientMessage) {
    switch (clientMessage->cmd) {
        case CMD_MOVE:
            G_ClientCommandMove(clientMessage);
            break;
        case CMD_ATTACK:
            G_ClientCommandAttack(clientMessage);
            break;
        default:
            break;
    }
}
