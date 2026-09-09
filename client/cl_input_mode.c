#include "cl_input_local.h"

typedef enum { BZ_INPUT_RTS, BZ_INPUT_ORBIT } INPUTMODE;
static INPUTMODE input_mode;
static struct { LPCSTR name; INPUTMODE mode; } const input_modes[] = {
    { "rts", BZ_INPUT_RTS },
    { "orbit", BZ_INPUT_ORBIT },
};

BOOL CL_InputOrbit(void) { return input_mode == BZ_INPUT_ORBIT; }

/* Select a complete interaction contract once, after game defaults and user overrides have loaded. */
void CL_InputModeInit(void) {
    LPCSTR name = Cvar_Get("cl_input_mode", "rts", 0)->string;
    FOR_LOOP(i, sizeof(input_modes) / sizeof(*input_modes)) {
        if (strcmp(name, input_modes[i].name)) continue;
        input_mode = input_modes[i].mode;
        if (CL_InputOrbit()) CL_OrbitInit();
        else CL_RtsInit();
        return;
    }
    Com_Error(ERR_FATAL, "cl_input_mode: unknown mode '%s' (expected rts or orbit)", name);
}

void CL_InputModeResetMap(void) {
    CL_RtsResetMap();
    CL_OrbitResetMap();
}

void CL_InputModeFrame(void) {
    if (CL_InputOrbit()) CL_OrbitFrame();
    else CL_RtsFrame();
}

void CL_InputModeMouseMotion(SDL_MouseMotionEvent const *motion) {
    if (CL_InputOrbit()) CL_OrbitMouseMotion(motion);
    else CL_RtsMouseMotion(motion);
}

BOOL CL_InputModeSelectDown(void) { return CL_InputOrbit() && CL_OrbitSelectDown(); }
BOOL CL_InputModeSelectUp(void) { return CL_InputOrbit() && CL_OrbitSelectUp(); }

#ifdef BZ_TESTS
#include "shared/test.h"
TEST(client_input, modes_are_available_in_one_build) {
    INPUTMODE saved = input_mode;
    char name[32];
    snprintf(name, sizeof(name), "%s", Cvar_String("cl_input_mode", "rts"));
    Cvar_Set("cl_input_mode", "orbit");
    CL_InputModeInit();
    T_ASSERT(CL_InputOrbit()); T_ASSERT(Cmd_Exists("+look")); T_ASSERT(Cmd_Exists("+forward"));
    Cvar_Set("cl_input_mode", "rts");
    CL_InputModeInit();
    T_ASSERT(!CL_InputOrbit()); T_ASSERT(Cmd_Exists("+pan")); T_ASSERT(Cmd_Exists("+smart"));
    Cvar_Set("cl_input_mode", name);
    input_mode = saved;
}
#endif
