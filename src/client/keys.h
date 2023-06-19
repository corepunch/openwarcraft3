#ifndef cl_key_h
#define cl_key_h

#define MAX_KEYS 256

typedef unsigned char keyCode_t;

enum {
    K_MOUSE1 = 200,
    K_MOUSE2 = 201,
    K_MOUSE3 = 202,
};

void Key_SetBinding(keyCode_t key, LPCSTR binding);
void Key_Event(keyCode_t key, bool down, DWORD time);

#endif
