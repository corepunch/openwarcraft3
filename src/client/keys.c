#include "client.h"

static uiName_t keybindings[MAX_KEYS];

typedef struct {
    LPCSTR name;
    DWORD keynum;
} keyname_t;

keyname_t keynames[] = {
    { "MOUSE1", K_MOUSE1 },
};

void Key_SetBinding(keyCode_t key, LPCSTR binding) {
    if (binding) {
        strcpy(keybindings[key], binding);
    } else {
        memset(keybindings[key], 0, sizeof(*keybindings));
    }
}

void Key_Event(keyCode_t key, bool down, DWORD time) {
    LPCSTR kb = keybindings[key];
    char cmd[1024];
    
    if (!down) {
        if (*kb == '+') {
            snprintf(cmd, sizeof(cmd), "-%s %i %i\n", kb+1, key, time);
            Cbuf_AddText(cmd);
        }
        return;
    }

    if (kb) {
        if (kb[0] == '+') {    // button commands add keynum and time as a parm
            snprintf(cmd, sizeof(cmd), "%s %i %i\n", kb, key, time);
            Cbuf_AddText(cmd);
        } else {
            Cbuf_AddText(kb);
            Cbuf_AddText("\n");
        }
    }
    return;
}
