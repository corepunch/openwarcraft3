#include "g_local.h"

/* Serialize one indicator to a target client's temporary-entity stream. */
static void G_SendWidgetIndicatorClient(LPGAMECLIENT client, LPCEDICT widget, COLOR32 color) {
    LPEDICT clent;
    DWORD packed;

    if (!client || !widget || !color.a || !client->connected || !gi.Write || !gi.unicast) return;
    clent = G_GetPlayerEntityByNumber(client->ps.number);
    if (!clent || !clent->client) return;

    packed = (DWORD)color.r | ((DWORD)color.g << 8) |
             ((DWORD)color.b << 16) | ((DWORD)color.a << 24);
    gi.Write(PF_BYTE, &(LONG){ svc_temp_entity });
    gi.Write(PF_BYTE, &(LONG){ TE_ENTITY_INDICATOR });
    gi.Write(PF_LONG, &(LONG){ (LONG)widget->s.number });
    gi.Write(PF_LONG, &(LONG){ (LONG)packed });
    gi.unicast(clent);
}

/* AddIndicator is local presentation, not simulation state. Keep its lifetime
 * on the client and send only the source widget/color for each recipient. */
void G_SendWidgetIndicator(LPEDICT widget, COLOR32 color, LPPLAYER local_player) {
    if (!widget || !color.a) return;
    if (local_player) {
        G_SendWidgetIndicatorClient(PLAYER_CLIENT(local_player), widget, color);
        return;
    }
    FOR_LOOP(i, game.max_clients)
        G_SendWidgetIndicatorClient(&game.clients[i], widget, color);
}
