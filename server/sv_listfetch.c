#include "server.h"

#include <stdlib.h>

#define MAX_SERVER_LIST_FETCHES 16

static serverListFetch_t list_fetches[MAX_SERVER_LIST_FETCHES];

serverListFetch_t *SV_ListFetchStates(DWORD *count) {
    if (count) {
        *count = MAX_SERVER_LIST_FETCHES;
    }
    return list_fetches;
}

static serverListFetchProvider_t const *SV_FindListFetchProvider(LPCSTR command) {
    serverListFetchProvider_t const *providers = SV_ListFetchProviders();

    if (!command || !*command) {
        return NULL;
    }
    for (serverListFetchProvider_t const *provider = providers; provider && provider->name; provider++) {
        if (!strcmp(provider->name, command)) {
            return provider;
        }
    }
    return NULL;
}

static void SV_ListFetchCommand(DWORD argc, LPCSTR *argv, LPSTR out, DWORD out_size) {
    DWORD len = 0;

    if (!out || out_size == 0) {
        return;
    }
    out[0] = '\0';
    for (DWORD i = 2; i < argc; i++) {
        int written = snprintf(out + len,
                               out_size - len,
                               "%s%s",
                               len ? " " : "",
                               argv[i]);
        if (written < 0) {
            out[len] = '\0';
            return;
        }
        if ((DWORD)written >= out_size - len) {
            out[out_size - 1] = '\0';
            return;
        }
        len += (DWORD)written;
    }
}

static serverListFetch_t *SV_FindListFetch(LPCLIENT client, DWORD requestId) {
    serverListFetch_t *free_state = NULL;

    FOR_LOOP(i, MAX_SERVER_LIST_FETCHES) {
        serverListFetch_t *state = &list_fetches[i];
        if (state->inuse && state->client == client && state->requestId == requestId) {
            return state;
        }
        if (!state->inuse && !free_state) {
            free_state = state;
        }
    }

    if (!free_state) {
        free_state = &list_fetches[0];
    }
    memset(free_state, 0, sizeof(*free_state));
    free_state->inuse = true;
    free_state->client = client;
    free_state->requestId = requestId;
    return free_state;
}

static void SV_ListFetchWrite(LPCLIENT client, DWORD requestId, LPCSTR text) {
    if (!client) {
        return;
    }
    MSG_WriteByte(&client->netchan.message, svc_list);
    MSG_WriteLong(&client->netchan.message, (int)requestId);
    MSG_WriteString(&client->netchan.message, text ? text : "");
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

void SV_ListFetchAppendRow(serverListFetch_t *state, LPCSTR text) {
    DWORD len;
    DWORD add;

    if (!state || !text || state->numRows >= MAX_LIST_FETCH_ROWS) {
        return;
    }
    len = (DWORD)strlen(state->text);
    add = (DWORD)strlen(text);
    if (len && len + 1 < sizeof(state->text)) {
        state->text[len++] = '\n';
        state->text[len] = '\0';
    }
    if (len + add >= sizeof(state->text)) {
        add = (DWORD)sizeof(state->text) - len - 1;
    }
    if (add) {
        memcpy(state->text + len, text, add);
    }
    state->text[len + add] = '\0';
    state->numRows++;
}

void SV_ListFetchSend(serverListFetch_t *state) {
    if (!state || !state->client) {
        return;
    }
    SV_ListFetchWrite(state->client, state->requestId, state->text);
    state->loading = false;
    state->inuse = false;
}

void SV_ListFetch_f(LPCLIENT client, DWORD argc, LPCSTR *argv) {
    DWORD requestId;
    serverListFetch_t *state;
    serverListFetchProvider_t const *provider;
    UINAME command;

    if (!client || argc < 3) {
        return;
    }

    SV_ListFetchCommand(argc, argv, command, sizeof(command));
    requestId = (DWORD)strtoul(argv[1], NULL, 10);

    provider = SV_FindListFetchProvider(command);
    if (!provider) {
        ge->ClientCommand(client->edict, argc, argv);
        return;
    }

    state = SV_FindListFetch(client, requestId);
    memset(state, 0, sizeof(*state));
    state->inuse = true;
    state->client = client;
    state->requestId = requestId;
    state->loading = true;
    snprintf(state->command, sizeof(state->command), "%s", command);

    if (provider->start) {
        provider->start(state);
    }
}

void SV_ListFetchFrame(void) {
    FOR_LOOP(i, MAX_SERVER_LIST_FETCHES) {
        serverListFetch_t *state = &list_fetches[i];
        if (!state->inuse || !state->loading) {
            continue;
        }
        serverListFetchProvider_t const *provider = SV_FindListFetchProvider(state->command);
        if (provider && provider->frame) {
            provider->frame(state);
        }
    }
}

void SV_ListFetchInfoResponse(const netadr_t *from, LPCSTR status) {
    serverListFetchProvider_t const *providers = SV_ListFetchProviders();

    for (serverListFetchProvider_t const *provider = providers; provider && provider->name; provider++) {
        if (provider->info) {
            provider->info(from, status);
        }
    }
}
