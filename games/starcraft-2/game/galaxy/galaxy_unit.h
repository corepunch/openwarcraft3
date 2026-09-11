/* galaxy_unit.h — unit, unitgroup, and unittype natives */
/* UnitCreate: resolve unit model from catalog, spawn at point position. */
static DWORD sc2_UnitCreate(LPJASS j) {
    LONG   count  = jass_checkinteger(j, 1);
    LPCSTR type   = jass_checkstring(j, 2);
    LONG   player = jass_checkinteger(j, 4);
    LONG   pt_h   = (LONG)(uintptr_t)jass_checkhandle(j, 5, "point");
    FLOAT  angle  = jass_checknumber(j, 6);
    FLOAT  x = 0.0f, y = 0.0f;
    if (pt_h > 0 && pt_h < sc2_gpoint_n) { x = sc2_gpoints[pt_h].x; y = sc2_gpoints[pt_h].y; }
    LONG handle = 0;
    for (LONG i = 0; i < count && sc2_gunit_n < MAX_GALAXY_UNITS; i++) {
        void *ent = sc2_galaxy_on_unit_create ?
            sc2_galaxy_on_unit_create(type ? type : "", (int)player, x, y, angle) : NULL;
        if (!ent)
            fprintf(stderr, "sc2_UnitCreate: on_unit_create returned NULL for type '%s' (%ld/%ld) — unit will be invisible\n",
                    type ? type : "(null)", (long)(i + 1), (long)count);
        handle = (LONG)(++sc2_gunit_n);
        sc2_gunits[handle - 1] = ent;
        sc2_last_unit_handle = handle;
    }
    return handle ? jass_pushlighthandle(j, (HANDLE)(uintptr_t)handle, "unit")
                  : jass_pushnullhandle(j, "unit");
}

static DWORD sc2_UnitLastCreated(LPJASS j) {
    return sc2_last_unit_handle ?
        jass_pushlighthandle(j, (HANDLE)(uintptr_t)sc2_last_unit_handle, "unit") :
        jass_pushnullhandle(j, "unit");
}
static DWORD sc2_UnitLastCreatedGroup(LPJASS j)     { return jass_pushnullhandle(j, "unitgroup"); }
static void *sc2_ent_from_handle(LPJASS j, int idx) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, idx, "unit");
    return (h > 0 && h <= (LONG)sc2_gunit_n) ? sc2_gunits[h - 1] : NULL;
}

static DWORD sc2_UnitSetPosition(LPJASS j) {
    void *ent = sc2_ent_from_handle(j, 1);
    LONG pt_h = (LONG)(uintptr_t)jass_checkhandle(j, 2, "point");
    if (ent && sc2_galaxy_unit_set_position && pt_h > 0 && pt_h < sc2_gpoint_n)
        sc2_galaxy_unit_set_position(ent, sc2_gpoints[pt_h].x, sc2_gpoints[pt_h].y, 0.0f);
    return jass_pushnull(j);
}

static DWORD sc2_UnitSetFacing(LPJASS j) {
    void *ent = sc2_ent_from_handle(j, 1);
    FLOAT ang = jass_checknumber(j, 2);
    if (ent && sc2_galaxy_unit_set_position) {
        /* Re-use set_position with NaN for x/y to indicate facing-only update.
         * g_sc2.c checks for this sentinel and only updates the angle. */
        sc2_galaxy_unit_set_position(ent, 0.0f/0.0f, 0.0f/0.0f, ang * 3.14159265f / 180.0f);
    }
    return jass_pushnull(j);
}

static DWORD sc2_UnitSetOwner(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGetOwner(LPJASS j) {
    (void)jass_checkhandle(j, 1, "unit");  /* consume arg */
    return jass_pushinteger(j, 0);
}
static DWORD sc2_UnitSetHeight(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitSetScale(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitSetState(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitSetCursor(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitTestState(LPJASS j)            { return jass_pushboolean(j, false); }
static DWORD sc2_UnitIsAlive(LPJASS j) {
    void *ent = sc2_ent_from_handle(j, 1);
    return jass_pushboolean(j, ent && (!sc2_galaxy_unit_is_alive || sc2_galaxy_unit_is_alive(ent)));
}
static DWORD sc2_UnitIsValid(LPJASS j) {
    void *ent = sc2_ent_from_handle(j, 1);
    return jass_pushboolean(j, ent != NULL);
}
static DWORD sc2_UnitKill(LPJASS j)                 { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitRemove(LPJASS j)               { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitRevive(LPJASS j)               { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitWaitUntilIdle(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitIssueOrder(LPJASS j) {
    LONG unit_h  = (LONG)(uintptr_t)jass_checkhandle(j, 1, "unit");
    LONG order_h = (LONG)(uintptr_t)jass_checkhandle(j, 2, "order");
    LONG queue   = jass_checkinteger(j, 3);
    if (order_h <= 0 || order_h >= sc2_gorder_n)
        return jass_pushboolean(j, false);
    sc2GOrder_t *ord = &sc2_gorders[order_h];
    LONG  ac_h = ord->abilcmd_h;
    LONG  pt_h = ord->pt_h;
    FLOAT tx   = (pt_h > 0 && pt_h < sc2_gpoint_n) ? sc2_gpoints[pt_h].x : 0.0f;
    FLOAT ty   = (pt_h > 0 && pt_h < sc2_gpoint_n) ? sc2_gpoints[pt_h].y : 0.0f;
    const char *ability = (ac_h > 0 && ac_h < sc2_gabilcmd_n)
                          ? sc2_gabilcmds[ac_h].ability : "move";
    if (unit_h <= 0 || unit_h > (LONG)sc2_gunit_n || (queue != 0 && queue != 1))
        return jass_pushboolean(j, false);
    if (queue == 0) sc2_uorder_n[unit_h - 1] = 0;
    LONG *count = &sc2_uorder_n[unit_h - 1];
    if (*count >= MAX_UNIT_ORDERS) {
        fprintf(stderr, "UnitIssueOrder: queue full for unit %ld\n", (long)unit_h);
        return jass_pushboolean(j, false);
    }
    sc2GUnitOrder_t *queued = &sc2_uorders[unit_h - 1][(*count)++];
    snprintf(queued->ability, sizeof(queued->ability), "%s", ability);
    queued->x = tx;
    queued->y = ty;
    queued->started = false;
#ifdef SC2_DEBUG_CUTSCENE
    fprintf(stderr, "UnitIssueOrder: unit=%ld ability=%s target=(%.1f,%.1f) queue=%ld depth=%ld\n",
            (long)unit_h, ability, tx, ty, (long)queue, (long)*count);
#endif
    return jass_pushboolean(j, true);
}
static DWORD sc2_UnitPauseAll(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGetFacing(LPJASS j)            { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_UnitGetHeight(LPJASS j)            { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_UnitGetPosition(LPJASS j) {
    return jass_pushnullhandle(j, "point");  /* TODO: extract ent position */
}
static DWORD sc2_UnitGetType(LPJASS j)   { (void)jass_checkhandle(j, 1, "unit"); return jass_pushstring(j, ""); }
static DWORD sc2_UnitFromId(LPJASS j)    { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_UnitBehaviorAdd(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitBehaviorRemove(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitCargoCreate(LPJASS j) {
    LONG   t_h  = (LONG)(uintptr_t)jass_checkhandle(j, 1, "unit");
    LPCSTR type = jass_checkstring(j, 2);
    LONG   cnt  = jass_checkinteger(j, 3);
    if (cnt < 1) cnt = 1;
    LONG handle  = 0;
    for (LONG i = 0; i < cnt && sc2_gunit_n < MAX_GALAXY_UNITS; i++) {
        void *ent = sc2_galaxy_on_unit_create ?
            sc2_galaxy_on_unit_create(type ? type : "", 0, 0.0f, 0.0f, 0.0f) : NULL;
        if (!ent)
            fprintf(stderr, "sc2_UnitCargoCreate: on_unit_create returned NULL for type '%s' (%ld/%ld) — cargo unit will be invisible\n",
                    type ? type : "(null)", (long)(i + 1), (long)cnt);
        handle = (LONG)(++sc2_gunit_n);
        sc2_gunits[handle - 1] = ent;
        sc2_last_cargo_handle  = handle;
        sc2_last_unit_handle   = handle;
        if (t_h > 0 && t_h <= MAX_GALAXY_UNITS) {
            LONG ci = sc2_gcargo_n[t_h - 1];
            if (ci < MAX_CARGO_PER_UNIT)
                sc2_gcargo[t_h - 1][sc2_gcargo_n[t_h - 1]++] = handle;
        }
    }
    fprintf(stderr, "UnitCargoCreate: transport=%ld type=%s count=%ld\n",
            (long)t_h, type ? type : "(null)", (long)cnt);
    return handle ? jass_pushlighthandle(j, (HANDLE)(uintptr_t)handle, "unit")
                  : jass_pushnullhandle(j, "unit");
}
static DWORD sc2_UnitCargoLastCreated(LPJASS j) {
    return sc2_last_cargo_handle ?
        jass_pushlighthandle(j, (HANDLE)(uintptr_t)sc2_last_cargo_handle, "unit") :
        jass_pushnullhandle(j, "unit");
}
static DWORD sc2_UnitCargoGroup(LPJASS j) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, 1, "unit");
    if (h > 0 && h <= (LONG)sc2_gunit_n)
        return jass_pushlighthandle(j,
            (HANDLE)(uintptr_t)(CARGO_GROUP_FLAG | h), "unitgroup");
    return jass_pushnullhandle(j, "unitgroup");
}
static DWORD sc2_UnitCargoLastCreatedGroup(LPJASS j){ return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitClearSelection(LPJASS j)       { (void)j; return jass_pushnull(j); }
/* UnitRef wraps a unit handle into a unitref (same pointer, different type name). */
static DWORD sc2_UnitRefFromUnit(LPJASS j) {
    HANDLE h = jass_checkhandle(j, 1, "unit");
    return h ? jass_pushlighthandle(j, h, "unitref") : jass_pushnullhandle(j, "unitref");
}
static DWORD sc2_UnitRefFromVariable(LPJASS j)  { return jass_pushnullhandle(j, "unitref"); }
static DWORD sc2_UnitRefToUnit(LPJASS j) {
    HANDLE h = jass_checkhandle(j, 1, "unitref");
    return h ? jass_pushlighthandle(j, h, "unit") : jass_pushnullhandle(j, "unit");
}
static DWORD sc2_UnitSetInfoText(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitClearInfoText(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitForceStatusBar(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGetAttachmentPoint(LPJASS j)   { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitSetTeamColorIndex(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitLoadModel(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitUnloadModel(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitSetPropertyFixed(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGetPropertyFixed(LPJASS j)     { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_EventUnit(LPJASS j)       { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_EventUnitCargo(LPJASS j)  { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_EventUnitTarget(LPJASS j) { return jass_pushnullhandle(j, "unit"); }

/* UnitGroup */
static DWORD sc2_UnitGroupEmpty(LPJASS j)            { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupAdd(LPJASS j)              { return jass_checkhandle(j, 1, "unitgroup") ? jass_pushlighthandle(j, jass_checkhandle(j, 1, "unitgroup"), "unitgroup") : jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupCount(LPJASS j) {
    LONG h    = (LONG)(uintptr_t)jass_checkhandle(j, 1, "unitgroup");
    LONG mode = jass_checkinteger(j, 2); (void)mode;
    if (h & CARGO_GROUP_FLAG) {
        LONG t = h & ~CARGO_GROUP_FLAG;
        if (t > 0 && t <= (LONG)sc2_gunit_n)
            return jass_pushinteger(j, sc2_gcargo_n[t - 1]);
    }
    return jass_pushinteger(j, 0);
}
static DWORD sc2_UnitGroupHasUnit(LPJASS j)          { return jass_pushboolean(j, false); }
static DWORD sc2_UnitGroupWaitUntilIdle(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitInventoryGroup(LPJASS j)        { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitTechTreeBehaviorCount(LPJASS j) { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitTechTreeUnitCount(LPJASS j)     { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitTechTreeUpgradeCount(LPJASS j)  { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitGroupUnit(LPJASS j)             { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitGroupIssueOrder(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGroupIdle(LPJASS j)             { return jass_pushboolean(j, false); }
static DWORD sc2_UnitGroupFilter(LPJASS j)           { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitGroupClear(LPJASS j)            { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupCopy(LPJASS j)             { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupRemove(LPJASS j)           { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGroupAlliance(LPJASS j)         { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupFilterAlliance(LPJASS j)   { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupFilterPlayer(LPJASS j)     { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupFilterPlane(LPJASS j)      { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupFilterRegion(LPJASS j)     { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupFilterThreat(LPJASS j)     { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupFromId(LPJASS j)           { return jass_pushnullhandle(j, "unitgroup"); }
static DWORD sc2_UnitGroupLoopBegin(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGroupLoopCurrent(LPJASS j)      { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_UnitGroupLoopDone(LPJASS j)         { return jass_pushboolean(j, true); }
static DWORD sc2_UnitGroupLoopEnd(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGroupLoopStep(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitGroupNearestUnit(LPJASS j)      { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_UnitGroupRandomUnit(LPJASS j)       { return jass_pushnullhandle(j, "unit"); }
static DWORD sc2_UnitGroupTestPlane(LPJASS j)        { return jass_pushboolean(j, false); }
static DWORD sc2_UnitFilter(LPJASS j)                { return jass_pushnullhandle(j, "unitfilter"); }
static DWORD sc2_UnitFilterMatch(LPJASS j)           { return jass_pushboolean(j, false); }
static DWORD sc2_UnitFilterSetState(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitFilterStr(LPJASS j)             { return jass_pushnullhandle(j, "unitfilter"); }
static DWORD sc2_UnitGroup(LPJASS j)                 { return jass_pushnullhandle(j, "unitgroup"); }

/* UnitType */
static DWORD sc2_UnitTypeFromString(LPJASS j)          { return jass_pushstring(j, ""); }
static DWORD sc2_UnitTypeGetCost(LPJASS j)             { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitTypeGetName(LPJASS j)             { return jass_pushstring(j, ""); }
static DWORD sc2_UnitTypeGetProperty(LPJASS j)         { return jass_pushinteger(j, 0); }
static DWORD sc2_UnitTypeIsAffectedByUpgrade(LPJASS j) { return jass_pushboolean(j, false); }
static DWORD sc2_UnitTypeTestAttribute(LPJASS j)       { return jass_pushboolean(j, false); }
static DWORD sc2_UnitTypeTestFlag(LPJASS j)            { return jass_pushboolean(j, false); }
static DWORD sc2_UnitTypeAnimationLoad(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_UnitTypeAnimationUnload(LPJASS j)     { (void)j; return jass_pushnull(j); }
