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
    state->scrollOffset = 0;
}

static DWORD CL_ListFetchCountRows(LPCSTR text) {
    DWORD rows = 0;

    if (!text || !*text) {
        return 0;
    }
    rows = 1;
    for (LPCSTR p = text; *p; p++) {
        if (*p == '\n') {
            rows++;
        }
    }
    return rows;
}

static void CL_ListFetchStart(listFetchState_t *state, uiListBox_t const *listbox) {
    char command[CMDARG_LEN * 2];

    state->started = true;
    state->loading = true;
    state->requestId = ++next_listfetch_request_id;
    CL_ListFetchClear(state);

    snprintf(command,
             sizeof(command),
             "list %u %s",
             (unsigned)state->requestId,
             listbox->fetchCommand);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, command);
}

void CL_ParseListFetch(LPSIZEBUF msg) {
    DWORD requestId = (DWORD)MSG_ReadLong(msg);
    listFetchState_t *state = CL_FindFetchRequest(requestId);
    char text[MAX_LIST_FETCH_TEXT];

    MSG_ReadString(msg, text);
    if (!state) {
        return;
    }

    snprintf(state->text, sizeof(state->text), "%s", text);
    state->numRows = CL_ListFetchCountRows(state->text);
    state->selectedIndex = -1;
    state->scrollOffset = 0;
    state->loading = false;
}

static DWORD CL_ListBoxMaxScroll(listFetchState_t const *state, DWORD visibleRows) {
    if (!state || state->numRows <= visibleRows) {
        return 0;
    }
    return state->numRows - visibleRows;
}

void CL_ListBoxApplyFetch(HANDLE layout,
                          LPCUIFRAME frame,
                          uiListBox_t const *listbox,
                          LPCSTR *text,
                          BOOL *loading,
                          SHORT *selectedIndex,
                          DWORD *scrollOffset,
                          DWORD *numRows)
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
    if (scrollOffset) {
        *scrollOffset = state->scrollOffset;
    }
    if (numRows) {
        *numRows = state->numRows;
    }
}

void CL_ListBoxScroll(HANDLE layout,
                      LPCUIFRAME frame,
                      uiListBox_t const *listbox,
                      int delta,
                      DWORD visibleRows)
{
    listFetchState_t *state;
    DWORD maxScroll;

    if (!layout || !frame || !listbox || !listbox->fetchCommand[0] || delta == 0) {
        return;
    }

    state = CL_FindFetchListBox(layout, frame->number, listbox->fetchCommand);
    maxScroll = CL_ListBoxMaxScroll(state, visibleRows);
    if (delta < 0) {
        DWORD amount = (DWORD)-delta;

        state->scrollOffset = amount > state->scrollOffset ? 0 : state->scrollOffset - amount;
    } else {
        state->scrollOffset = MIN(state->scrollOffset + (DWORD)delta, maxScroll);
    }
}

void CL_ListBoxSelect(HANDLE layout,
                      LPCUIFRAME frame,
                      uiListBox_t const *listbox,
                      DWORD rowIndex)
{
    listFetchState_t *state;
    char command[CMDARG_LEN * 2];

    if (!layout || !frame || !listbox || !listbox->fetchCommand[0]) {
        return;
    }

    state = CL_FindFetchListBox(layout, frame->number, listbox->fetchCommand);
    state->selectedIndex = (SHORT)rowIndex;
    snprintf(command,
             sizeof(command),
             "listselect %s %u",
             listbox->fetchCommand,
             (unsigned)rowIndex);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, command);
}
