DWORD CreateItem(LPJASS j) {
    LONG itemid = jass_checkinteger(j, 1);
    FLOAT x = jass_checknumber(j, 2);
    FLOAT y = jass_checknumber(j, 3);
    LPEDICT item = SP_SpawnAtLocation(itemid, 0, &MAKE(VECTOR2, x, y));
    return jass_pushlighthandle(j, item, "item");
}
DWORD RemoveItem(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    return 0;
}
DWORD GetItemPlayer(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    return jass_pushnullhandle(j, "player");
}
DWORD GetItemTypeId(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    return jass_pushinteger(j, 0);
}
DWORD GetItemX(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    return jass_pushnumber(j, 0);
}
DWORD GetItemY(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    return jass_pushnumber(j, 0);
}
DWORD SetItemPosition(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD SetItemDropOnDeath(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetItemDroppable(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetItemPlayer(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL changeColor = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetItemInvulnerable(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsItemInvulnerable(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    return jass_pushboolean(j, 0);
}
DWORD GetManipulatedItem(LPJASS j) {
    return jass_pushnullhandle(j, "item");
}
DWORD GetOrderTargetItem(LPJASS j) {
    return jass_pushnullhandle(j, "item");
}
