#include "client.h"

#define MAX_FETCH_LISTBOXES 16

static listFetchState_t fetch_listboxes[MAX_FETCH_LISTBOXES];
static DWORD next_listfetch_request_id = 0;

static listFetchState_t *CL_FindFetchListBox(HANDLE layout, DWORD frameNumber, LPCSTR command) {
    listFetchState_t *free_state = NULL;

    FOR_LOOP(i, MAX_FETCH_LISTBOXES) {
        listFetchState_t *state = &fetch_listboxes[i];
        if (state->inuse &&
            state->layout == layout &&
            state->frameNumber == frameNumber &&
            !strcmp(state->command, command)) {
            return state;
        }
        if (!state->inuse && !free_state) {
            free_state = state;
        }
    }

    if (!free_state) {
        free_state = &fetch_listboxes[0];
    }
    memset(free_state, 0, sizeof(*free_state));
    free_state->inuse = true;
    free_state->layout = layout;
    free_state->frameNumber = frameNumber;
    snprintf(free_state->command, sizeof(free_state->command), "%s", command);
    free_state->selectedIndex = -1;
    return free_state;
}

static listFetchState_t *CL_FindFetchRequest(DWORD requestId) {
    FOR_LOOP(i, MAX_FETCH_LISTBOXES) {
        listFetchState_t *state = &fetch_listboxes[i];
        if (state->inuse && state->requestId == requestId) {
            return state;
        }
    }
    return NULL;
}

static void CL_ListFetchClear(listFetchState_t *state) {
    if (!state) {
        return;
    }
    state->text[0] = '\0';
    state->selectedIndex = -1;
}

static void CL_ListFetchAdd(listFetchState_t *state, LPCSTR text) {
    DWORD len;
    DWORD add;

    if (!state) {
        return;
    }
    len = (DWORD)strlen(state->text);
    add = text ? (DWORD)strlen(text) : 0;
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
}

static void CL_ListFetchStart(listFetchState_t *state, uiListBox_t const *listbox) {
    char command[CMDARG_LEN * 2];

    state->started = true;
    state->loading = true;
    state->requestId = ++next_listfetch_request_id;
    CL_ListFetchClear(state);

    snprintf(command,
             sizeof(command),
             "listfetch %u %s",
             (unsigned)state->requestId,
             listbox->fetchCommand);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, command);
}

static void CL_ReadListFetchString(LPSIZEBUF msg, LPSTR out, DWORD out_size) {
    DWORD write = 0;

    if (!msg) {
        return;
    }

    for (;;) {
        int c = MSG_ReadByte(msg);
        if (c == 0) {
            break;
        }
        if (out && out_size > 0 && write + 1 < out_size) {
            out[write++] = (char)c;
        }
    }

    if (out && out_size > 0) {
        out[write] = '\0';
    }
}

void CL_ParseListFetch(LPSIZEBUF msg) {
    DWORD requestId = (DWORD)MSG_ReadLong(msg);
    listFetchOp_t op = (listFetchOp_t)MSG_ReadByte(msg);
    listFetchState_t *state = CL_FindFetchRequest(requestId);
    char text[MAX_LIST_FETCH_TEXT];

    if (!state) {
        if (op == listfetch_add) {
            CL_ReadListFetchString(msg, text, sizeof(text));
        }
        return;
    }

    switch (op) {
        case listfetch_clear:
            state->numRows = 0;
            CL_ListFetchClear(state);
            break;
        case listfetch_add:
            CL_ReadListFetchString(msg, text, sizeof(text));
            CL_ListFetchAdd(state, text);
            state->numRows++;
            break;
        case listfetch_done:
            state->loading = false;
            break;
        default:
            break;
    }
}

void CL_ListBoxApplyFetch(HANDLE layout,
                          LPCUIFRAME frame,
                          uiListBox_t const *listbox,
                          LPCSTR *text,
                          BOOL *loading,
                          SHORT *selectedIndex)
{
    listFetchState_t *state;

    if (!layout || !frame || !listbox || !listbox->fetchCommand[0]) {
        return;
    }

    state = CL_FindFetchListBox(layout, frame->number, listbox->fetchCommand);
    if (!state->started) {
        CL_ListFetchStart(state, listbox);
    }

    *text = state->text;
    *loading = state->loading;
    *selectedIndex = state->selectedIndex;
}

void CL_ListFetchResetLayout(HANDLE layout) {
    if (!layout) {
        return;
    }

    FOR_LOOP(i, MAX_FETCH_LISTBOXES) {
        listFetchState_t *state = &fetch_listboxes[i];
        if (state->inuse && state->layout == layout) {
            memset(state, 0, sizeof(*state));
        }
    }
}
