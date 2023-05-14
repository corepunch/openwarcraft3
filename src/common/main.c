#include "../client/client.h"
#include "../server/server.h"


extern struct tTexture *Texture;

void Sys_Quit(void) {
    exit(0);
}

int main( int argc, char * argv[] ) {
    FS_Init();
    SV_Init();
    CL_Init();
    SV_Map("Maps\\Campaign\\Human01.w3m");
    
    while (true) {
        SV_Frame(0);
        CL_Frame(0);
    }

    return 0;
}
