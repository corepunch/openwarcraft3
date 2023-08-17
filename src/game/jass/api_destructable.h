DWORD CreateDestructable(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT face = jass_checknumber(j, 4);
    //FLOAT scale = jass_checknumber(j, 5);
    //LONG variation = jass_checkinteger(j, 6);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD CreateDestructableZ(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    //FLOAT scale = jass_checknumber(j, 6);
    //LONG variation = jass_checkinteger(j, 7);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD CreateDeadDestructable(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT face = jass_checknumber(j, 4);
    //FLOAT scale = jass_checknumber(j, 5);
    //LONG variation = jass_checkinteger(j, 6);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD CreateDeadDestructableZ(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    //FLOAT scale = jass_checknumber(j, 6);
    //LONG variation = jass_checkinteger(j, 7);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD RemoveDestructable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return 0;
}
DWORD KillDestructable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return 0;
}
DWORD SetDestructableInvulnerable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsDestructableInvulnerable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushboolean(j, 0);
}
DWORD EnumDestructablesInRect(LPJASS j) {
    //HANDLE r = jass_checkhandle(j, 1, "rect");
    //HANDLE filter = jass_checkhandle(j, 2, "boolexpr");
    //LPCSTR actionFunc = jass_checkcode(j, 3);
    return 0;
}
DWORD GetDestructableTypeId(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushinteger(j, 0);
}
DWORD GetDestructableX(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD GetDestructableY(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD SetDestructableLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //FLOAT life = jass_checknumber(j, 2);
    return 0;
}
DWORD GetDestructableLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD SetDestructableMaxLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //FLOAT max = jass_checknumber(j, 2);
    return 0;
}
DWORD GetDestructableMaxLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD DestructableRestoreLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //FLOAT life = jass_checknumber(j, 2);
    //BOOL birth = jass_checkboolean(j, 3);
    return 0;
}
DWORD QueueDestructableAnimation(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
DWORD SetDestructableAnimation(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
