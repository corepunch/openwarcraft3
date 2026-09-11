/* galaxy_trigger.h — trigger natives */
/* Triggers are opaque handles; extract the integer ID via pointer cast. */
static LONG sc2_trigger_id(LPJASS j, int index) {
    HANDLE h = jass_checkhandle(j, index, "trigger");
    return h ? (LONG)(uintptr_t)h : 0;
}

static DWORD sc2_TriggerCreate(LPJASS j) {
    LPCSTR name = jass_checkstring(j, 1);
    LONG id = 0;
    if (name && sc2_trig_n < MAX_SC2_TRIGGERS) {
        id = sc2_trig_next_id++;
        /* strdup: the JASS VM may GC the string after this call returns. */
        sc2_trigs[sc2_trig_n++] = (sc2trig_t){ id, strdup(name), false };
    }
    return id ? jass_pushlighthandle(j, (HANDLE)(uintptr_t)id, "trigger")
              : jass_pushnullhandle(j, "trigger");
}

static DWORD sc2_TriggerEnable(LPJASS j)      { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerStop(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerGetCurrent(LPJASS j)  { return jass_pushnullhandle(j, "trigger"); }
static DWORD sc2_TriggerGetExecCount(LPJASS j){ return jass_pushinteger(j, 0); }
static DWORD sc2_TriggerIsEnabled(LPJASS j)   { return jass_pushboolean(j, true); }

static DWORD sc2_TriggerExecute(LPJASS j) {
    LONG   id        = sc2_trigger_id(j, 1);
    BOOL   testConds = jass_checkboolean(j, 2);
    BOOL   waitDone  = jass_checkboolean(j, 3);
    for (DWORD i = 0; i < sc2_trig_n; i++) {
        if (sc2_trigs[i].id == id && sc2_trigs[i].func) {
#ifdef SC2_DEBUG_CUTSCENE
            fprintf(stderr, "TriggerExecute: %s testConds=%d waitDone=%d\n",
                    sc2_trigs[i].func, testConds, waitDone);
#endif
            sc2_fire_trigger_func(j, sc2_trigs[i].func, testConds, !waitDone);
            break;
        }
    }
    return jass_pushnull(j);
}

static DWORD sc2_TriggerAddEventMapInit(LPJASS j) {
    LONG id = sc2_trigger_id(j, 1);
    for (DWORD i = 0; i < sc2_trig_n; i++) {
        if (sc2_trigs[i].id == id) { sc2_trigs[i].mapinit = true; break; }
    }
    return jass_pushnull(j);
}

static DWORD sc2_TriggerSkippableBegin(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerSkippableEnd(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerQueueEnter(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerQueueExit(LPJASS j)      { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerQueueIsEmpty(LPJASS j)   { return jass_pushboolean(j, true); }
static DWORD sc2_TriggerQueuePause(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerQueueClear(LPJASS j)     { (void)j; return jass_pushnull(j); }

static DWORD sc2_Wait(LPJASS j) {
    FLOAT secs = jass_checknumber(j, 1);
    jass_sleep(j, (DWORD)(secs * 1000.0f));
    return 0;
}

/* Event registration stubs */
static DWORD sc2_TriggerAddEventPlayerAIWave(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventPlayerAllianceChange(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventPlayerLeft(LPJASS j)              { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventPlayerPropChange(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventTimeElapsed(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventTimePeriodic(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventTimer(LPJASS j)                   { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitAttacked(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitCargo(LPJASS j)               { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitDamaged(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitDied(LPJASS j)                { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitOrder(LPJASS j)               { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitRange(LPJASS j)               { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitRangePoint(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerAddEventUnitRegion(LPJASS j)              { (void)j; return jass_pushnull(j); }
static DWORD sc2_TriggerDebugOutput(LPJASS j)                     { (void)j; return jass_pushnull(j); }
