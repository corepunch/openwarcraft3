#include <stdlib.h>

#include "client.h"
#include "ui.h"

typedef struct {
    int x, y;
} layoutstate_t;

void layout_xr(parser_t *p, layoutstate_t *state){
    state->x = 800 - atoi(ParserGetToken(p));
}

void layout_yb(parser_t *p, layoutstate_t *state){
    state->y = 600 - atoi(ParserGetToken(p));
}

void layout_pic(parser_t *p, layoutstate_t *state){
    int pic = atoi(ParserGetToken(p));
    re.DrawPic(cl.pics[pic], state->x, state->y);
}

typedef struct {
    LPCSTR cmd;
    void (*func)(parser_t *p, layoutstate_t *state);
} layoutcmd_t;

layoutcmd_t cmds[] = {
    { "xr", layout_xr },
    { "yb", layout_yb },
    { "pic", layout_pic },
    { NULL }
};

void SCR_ExecuteLayoutString(void) {
    LPCSTR tok;
    parser_t parser = { 0 };
    parser.tok = parser.token;
    parser.str = cl.layout;
    parser_t *p = &parser;
    layoutstate_t state = { 0 };
    while ((tok = ParserGetToken(p))) {
        for (layoutcmd_t *layout = cmds; layout->cmd; layout++) {
            if (strcmp(tok, layout->cmd))
                continue;
            layout->func(p, &state);
            break;
        }
    }
}

void SCR_UpdateScreen(void) {
    re.BeginFrame();
    
    V_RenderView();
    
    ui.Draw();

    SCR_ExecuteLayoutString();

    CON_DrawConsole();
    
    re.EndFrame();
}
