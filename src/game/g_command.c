#include "g_local.h"

static void G_ClientCommandMove(LPCCLIENTMESSAGE clientMessage) {
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

static void G_ClientCommandAttack(LPCCLIENTMESSAGE clientMessage) {
    
}

void G_ClientCommand(LPCCLIENTMESSAGE clientMessage) {
    switch (clientMessage->cmd) {
        case CMD_MOVE:
            G_ClientCommandMove(clientMessage);
            break;
        case CMD_ATTACK:
//            G_ClientCommandAttack(clientMessage);
//            edict->goalentity = &game_state.edicts[clientMessage->targetentity];
//            edict->enemy = &game_state.edicts[clientMessage->targetentity];
            break;
        default:
            break;
    }
}
