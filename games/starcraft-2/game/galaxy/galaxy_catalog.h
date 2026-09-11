/* galaxy_catalog.h — ability, order, and catalog natives */

#define MAX_GALAXY_ABILCMDS 1024
typedef struct { char ability[64]; LONG cmd_idx; } sc2GAbilCmd_t;
static sc2GAbilCmd_t sc2_gabilcmds[MAX_GALAXY_ABILCMDS];
static LONG sc2_gabilcmd_n = 1; /* 1-based; 0 = null */

#define MAX_GALAXY_ORDERS 1024
typedef struct { LONG abilcmd_h; LONG pt_h; LONG unit_h; } sc2GOrder_t;
static sc2GOrder_t sc2_gorders[MAX_GALAXY_ORDERS];
static LONG sc2_gorder_n = 1;  /* 1-based; 0 = null */

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
static DWORD sc2_AbilityCommandGetAbility(LPJASS j) {
    LONG h = jass_checkinteger(j, 1);
    return jass_pushstring(j, (h > 0 && h < sc2_gabilcmd_n) ? sc2_gabilcmds[h].ability : "");
}
static DWORD sc2_AbilityCommandGetCommand(LPJASS j) {
    LONG h = jass_checkinteger(j, 1);
    return jass_pushinteger(j, (h > 0 && h < sc2_gabilcmd_n) ? sc2_gabilcmds[h].cmd_idx : 0);
}
/* Action index is a higher-level concept (e.g. cast vs auto-cast); stub as 0 for now. */
static DWORD sc2_AbilityCommandGetAction(LPJASS j) { (void)j; return jass_pushinteger(j, 0); }
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
static DWORD sc2_OrderTargetingUnit(LPJASS j) {
    LONG abilcmd_h = jass_checkinteger(j, 1);
    LONG unit_h    = (LONG)(uintptr_t)jass_checkhandle(j, 2, "unit");
    if (sc2_gorder_n < MAX_GALAXY_ORDERS) {
        LONG h = sc2_gorder_n++;
        sc2_gorders[h] = (sc2GOrder_t){ abilcmd_h, 0, unit_h };
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "order");
    }
    return jass_pushnullhandle(j, "order");
}
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
