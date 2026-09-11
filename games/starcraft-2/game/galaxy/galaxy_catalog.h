/* galaxy_catalog.h — ability, order, and catalog natives */
static DWORD sc2_AbilityClass(LPJASS j)       { return jass_pushinteger(j, 0); }
static DWORD sc2_AbilityCommand(LPJASS j) {
    LPCSTR name = jass_checkstring(j, 1);
    LONG   cmd  = jass_checkinteger(j, 2);
    if (sc2_gabilcmd_n < MAX_GALAXY_ABILCMDS) {
        LONG h = sc2_gabilcmd_n++;
        snprintf(sc2_gabilcmds[h].ability, sizeof(sc2_gabilcmds[h].ability),
                 "%s", name ? name : "");
        sc2_gabilcmds[h].cmd_idx = cmd;
        return jass_pushinteger(j, h);
    }
    fprintf(stderr, "AbilityCommand: table full (%d entries) — '%s' lost\n",
            MAX_GALAXY_ABILCMDS, name ? name : "");
    return jass_pushinteger(j, 0);
}
static DWORD sc2_AbilityCommandGetAbility(LPJASS j){ return jass_pushstring(j, ""); }
static DWORD sc2_AbilityCommandGetCommand(LPJASS j){ return jass_pushinteger(j, 0); }
static DWORD sc2_AbilityCommandGetAction(LPJASS j) { return jass_pushinteger(j, 0); }
static DWORD sc2_Order(LPJASS j)              { return jass_pushnullhandle(j, "order"); }
static DWORD sc2_OrderTargetingPoint(LPJASS j) {
    LONG abilcmd_h = jass_checkinteger(j, 1);
    LONG pt_h      = (LONG)(uintptr_t)jass_checkhandle(j, 2, "point");
    if (sc2_gorder_n < MAX_GALAXY_ORDERS) {
        LONG h = sc2_gorder_n++;
        sc2_gorders[h] = (sc2GOrder_t){ abilcmd_h, pt_h };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "order");
    }
    fprintf(stderr, "OrderTargetingPoint: table full (%d entries)\n", MAX_GALAXY_ORDERS);
    return jass_pushnullhandle(j, "order");
}
static DWORD sc2_OrderTargetingUnit(LPJASS j) { return jass_pushnullhandle(j, "order"); }
static DWORD sc2_OrderSetPlayer(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitOrderIsValid(LPJASS j)   { return jass_pushboolean(j, false); }
static DWORD sc2_CatalogEntryClass(LPJASS j)  { return jass_pushinteger(j, 0); }
static DWORD sc2_CatalogEntryCount(LPJASS j)  { return jass_pushinteger(j, 0); }
static DWORD sc2_CatalogEntryGet(LPJASS j)    { return jass_pushstring(j, ""); }
static DWORD sc2_CatalogEntryIsValid(LPJASS j){ return jass_pushboolean(j, false); }
static DWORD sc2_CatalogEntryParent(LPJASS j) { return jass_pushstring(j, ""); }
static DWORD sc2_CatalogEntryScope(LPJASS j)  { return jass_pushstring(j, ""); }
static DWORD sc2_CatalogFieldCount(LPJASS j)  { return jass_pushinteger(j, 0); }
static DWORD sc2_CatalogFieldGet(LPJASS j)    { return jass_pushstring(j, ""); }
static DWORD sc2_CatalogFieldIsArray(LPJASS j){ return jass_pushboolean(j, false); }
static DWORD sc2_CatalogFieldIsScope(LPJASS j){ return jass_pushboolean(j, false); }
static DWORD sc2_CatalogFieldType(LPJASS j)   { return jass_pushstring(j, ""); }
static DWORD sc2_CatalogFieldValueCount(LPJASS j){ return jass_pushinteger(j, 0); }
static DWORD sc2_CatalogFieldValueGet(LPJASS j){ return jass_pushstring(j, ""); }
static DWORD sc2_CatalogFieldValueSet(LPJASS j){ return jass_pushboolean(j, false); }
