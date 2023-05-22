#include "client.h"
#include <stdarg.h>

typedef struct  {
    char msg[MAX_CONSOLE_MESSAGE_LEN];
    DWORD time;
} consoleMessage_t;

static consoleMessage_t messages[MAX_CONSOLE_MESSAGES] = { 0 };
static DWORD current_message = 0;

void CON_printf(char *fmt, ...) {
    consoleMessage_t *msg = &messages[current_message++ % MAX_CONSOLE_MESSAGES];
    va_list argptr;
    va_start(argptr,fmt);
    vsprintf(msg->msg, fmt, argptr);
    va_end(argptr);
    msg->time = cl.time;
}

void CON_DrawConsole(void) {
    DWORD x = 4, y = 0;
    COLOR32 color = {255,255,255,160};
    FOR_LOOP(i, MAX_CONSOLE_MESSAGES) {
        if (!*messages[i].msg || cl.time > messages[i].time + CONSOLE_MESSAGE_TIME)
            continue;
        renderer->PrintText(messages[i].msg, x, y, color);
        y += 16;
    }
}
