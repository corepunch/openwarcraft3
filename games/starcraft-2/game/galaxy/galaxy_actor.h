/* galaxy_actor.h — actor, bank, conversation, achievement, and timer natives */

#define MAX_GALAXY_ACTORS 1024

typedef struct {
    DWORD id;
    char  model[256];   /* actor type / model link from ActorCreate */
    LONG  unit_id;      /* owning unit handle (1-based), or 0 for world actors */
    FLOAT x, y;
} sc2GActor_t;

static sc2GActor_t sc2_gactors[MAX_GALAXY_ACTORS];
static DWORD sc2_gactor_n;
static LONG  sc2_last_actor_handle;

static LONG sc2_actor_h(LPJASS j, int idx) {
    return (LONG)(uintptr_t)jass_checkhandle(j, idx, "actor");
}

/* ActorCreate: actortype(string), scope(actorscope), hostSite(string), hostable(string) */
static DWORD sc2_ActorCreate(LPJASS j) {
    LPCSTR type   = jass_checkstring(j, 1);
    LONG   scope_h = (LONG)(uintptr_t)jass_checkhandle(j, 2, "actorscope");
    if (sc2_gactor_n >= MAX_GALAXY_ACTORS)
        return jass_pushnullhandle(j, "actor");
    LONG h = (LONG)(++sc2_gactor_n);
    sc2GActor_t *a = &sc2_gactors[h - 1];
    memset(a, 0, sizeof(*a));
    a->id = (DWORD)h;
    if (type) strlcpy(a->model, type, sizeof(a->model));
    /* actorscope encodes the owning unit handle */
    a->unit_id = (scope_h > 0 && scope_h <= (LONG)sc2_gunit_n) ? scope_h : 0;
    sc2_last_actor_handle = h;
    if (sc2_galaxy_on_actor_create)
        sc2_galaxy_on_actor_create((DWORD)h, a->model, (DWORD)a->unit_id, a->x, a->y);
    return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "actor");
}

/* ActorSend: actor, msg — dispatch to renderer; handle "Destroy" locally */
static DWORD sc2_ActorSend(LPJASS j) {
    LONG   h   = sc2_actor_h(j, 1);
    LPCSTR msg = jass_checkstring(j, 2);
    if (h > 0 && h <= (LONG)sc2_gactor_n && msg) {
        if (sc2_galaxy_on_actor_send)
            sc2_galaxy_on_actor_send((DWORD)h, msg);
        if (strcmp(msg, "Destroy") == 0)
            memset(&sc2_gactors[h - 1], 0, sizeof(sc2_gactors[0]));
    }
    return jass_pushnull(j);
}

/* ActorScopeFromUnit: encode the unit handle as an actorscope */
static DWORD sc2_ActorScopeFromUnit(LPJASS j) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, 1, "unit");
    if (h <= 0 || h > (LONG)sc2_gunit_n)
        return jass_pushnullhandle(j, "actorscope");
    return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "actorscope");
}

static DWORD sc2_ActorFrom(LPJASS j)           { (void)j; return jass_pushnullhandle(j, "actor"); }
static DWORD sc2_ActorFromScope(LPJASS j)      { (void)j; return jass_pushnullhandle(j, "actor"); }
static DWORD sc2_ActorRegionCreate(LPJASS j)   { (void)j; return jass_pushnullhandle(j, "actorscope"); }
static DWORD sc2_ActorRegionSend(LPJASS j)     { (void)j; return jass_pushnull(j); }

static DWORD sc2_AchievementAward(LPJASS j)              { (void)j; return jass_pushnull(j); }
static DWORD sc2_AchievementErase(LPJASS j)              { (void)j; return jass_pushnull(j); }
static DWORD sc2_AchievementPanelSetCategory(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_AchievementPanelSetVisible(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_AchievementPercentText(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_AchievementTermQuantitySet(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_AchievementsDisable(LPJASS j)           { (void)j; return jass_pushnull(j); }

static DWORD sc2_BankExists(LPJASS j)                    { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_BankKeyRemove(LPJASS j)                 { (void)j; return jass_pushnull(j); }
static DWORD sc2_BankLastCreated(LPJASS j)               { (void)j; return jass_pushnullhandle(j, "bank"); }
static DWORD sc2_BankLoad(LPJASS j)                      { (void)j; return jass_pushnullhandle(j, "bank"); }
static DWORD sc2_BankSave(LPJASS j)                      { (void)j; return jass_pushnull(j); }
static DWORD sc2_BankValueSetFromFlag(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_BankValueSetFromInt(LPJASS j)           { (void)j; return jass_pushnull(j); }
static DWORD sc2_BankValueSetFromString(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_BankValueSetFromText(LPJASS j)          { (void)j; return jass_pushnull(j); }

static DWORD sc2_ConversationDataResetNodeState(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataResetStateValues(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataSaveNodeState(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataSaveStateValues(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataStateFixedValue(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataStateGetValue(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataStateIndex(LPJASS j)       { (void)j; return jass_pushinteger(j, 0); }
static DWORD sc2_ConversationDataStateIndexCount(LPJASS j)  { (void)j; return jass_pushinteger(j, 0); }
static DWORD sc2_ConversationDataStateName(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataStateSetValue(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataStateText(LPJASS j)        { (void)j; return jass_pushnull(j); }

static DWORD sc2_TimerPause(LPJASS j)                    { (void)j; return jass_pushnull(j); }
