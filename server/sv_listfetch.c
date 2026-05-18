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

static void SV_ListFetchWrite(serverListFetch_t *state, listFetchOp_t op, LPCSTR text) {
    if (!state || !state->client) {
        return;
    }
    MSG_WriteByte(&state->client->netchan.message, svc_listfetch);
    MSG_WriteLong(&state->client->netchan.message, (int)state->requestId);
    MSG_WriteByte(&state->client->netchan.message, op);
    if (op == listfetch_add) {
        MSG_WriteString(&state->client->netchan.message, text ? text : "");
    }
    Netchan_Transmit(NS_SERVER, &state->client->netchan);
}

void SV_ListFetchClear(serverListFetch_t *state) {
    if (state) {
        state->numRows = 0;
    }
    SV_ListFetchWrite(state, listfetch_clear, NULL);
}

void SV_ListFetchAdd(serverListFetch_t *state, LPCSTR text) {
    if (state) {
        state->numRows++;
    }
    SV_ListFetchWrite(state, listfetch_add, text);
}

void SV_ListFetchDone(serverListFetch_t *state) {
    if (state) {
        state->loading = false;
    }
    SV_ListFetchWrite(state, listfetch_done, NULL);
}

void SV_ListFetch_f(LPCLIENT client, DWORD argc, LPCSTR *argv) {
    DWORD requestId;
    serverListFetch_t *state;
    serverListFetchProvider_t const *provider;

    if (!client || argc < 3) {
        return;
    }

    requestId = (DWORD)strtoul(argv[1], NULL, 10);
    provider = SV_FindListFetchProvider(argv[2]);
    if (!provider) {
        state = SV_FindListFetch(client, requestId);
        SV_ListFetchDone(state);
        return;
    }

    state = SV_FindListFetch(client, requestId);
    memset(state, 0, sizeof(*state));
    state->inuse = true;
    state->client = client;
    state->requestId = requestId;
    state->loading = true;
    snprintf(state->command, sizeof(state->command), "%s", argv[2]);

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
