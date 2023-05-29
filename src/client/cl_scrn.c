#include <stdlib.h>

#include "client.h"
#include "../ui/ui.h"

void SCR_ExecuteLayoutString(void) {
    int x = 0, y = 0;
    parser_t parser = { 0 };
    parser.tok = parser.token;
    parser.str = cl.layout;
    parser_t *p = &parser;
    for (LPCSTR tok = ParserGetToken(p);
         !ParserDone(p);
         tok = ParserGetToken(p))
    {
        if (!strcmp(tok, "xr")) {
            x = 800 - atoi(ParserGetToken(p));
            continue;
        }
        if (!strcmp(tok, "yb")) {
            y = 600 - atoi(ParserGetToken(p));
            continue;
        }
        if (!strcmp(tok, "pic")) {
            int pic = atoi(ParserGetToken(p));
            re.DrawPic(cl.pics[pic], x, y);
        }
    }
}

void SCR_UpdateScreen(void) {
    re.BeginFrame();
    
    V_RenderView();
    
    SCR_ExecuteLayoutString();

    UI_Draw();

    CON_DrawConsole();
    
    re.EndFrame();
}
