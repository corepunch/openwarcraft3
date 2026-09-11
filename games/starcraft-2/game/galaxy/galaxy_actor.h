/* galaxy_actor.h — actor, bank, conversation, achievement, and timer natives */

#define MAX_GALAXY_ACTORS 1024 // actors/map; bounds script actor records; used for actor identity
#define BZ_SC2_SCOPES (MAX_GALAXY_UNITS + MAX_GALAXY_ACTORS) // scopes/map; one per unit and created actor; used for scope ownership

typedef struct {
    DWORD id;
    char  model[256];   /* actor type / model link from ActorCreate */
    LONG scope;
    LONG  unit_id;      /* owning unit handle (1-based), or 0 for world actors */
    FLOAT x, y;
} sc2GActor_t;

static sc2GActor_t sc2_gactors[MAX_GALAXY_ACTORS];
static DWORD sc2_gactor_n;
static LONG  sc2_last_actor_handle;

typedef struct { LONG parent, unit; BOOL live; } SC2SCOPE;
typedef SC2SCOPE *LPSC2SCOPE;
typedef const SC2SCOPE *LPCSC2SCOPE;
static SC2SCOPE sc2_scopes[BZ_SC2_SCOPES];
static LONG sc2_scope_n, sc2_scope_last;
static LONG sc2_unit_scope[MAX_GALAXY_UNITS];

/* Scope ownership is independent of actor identity: killing an attachment must not kill the unit scope. */
static LONG sc2_scope_create(LPJASS j, LONG parent, LONG unit) {
    if (sc2_scope_n == BZ_SC2_SCOPES) jass_rterror(j, "ActorCreate: scope capacity exhausted");
    sc2_scopes[sc2_scope_n++] = (SC2SCOPE){ .parent = parent, .unit = unit, .live = true };
    sc2_scope_last = sc2_scope_n;
    return sc2_scope_last;
}

static LONG sc2_actor_h(LPJASS j, int idx) {
    return (LONG)(uintptr_t)jass_checkhandle(j, idx, "actor");
}

/* NativeLib passes scope first, then actor and three content links; the old reversed ABI asserted on a live scope. */
static DWORD sc2_ActorCreate(LPJASS j) {
    LPCSTR type   = jass_checkstring(j, 2);
    LONG   scope_h = (LONG)(uintptr_t)jass_checkhandle(j, 1, "actorscope");
    if (sc2_gactor_n >= MAX_GALAXY_ACTORS)
        jass_rterror(j, "ActorCreate: actor capacity exhausted");
    if (scope_h < 0 || scope_h > sc2_scope_n || (scope_h && !sc2_scopes[scope_h - 1].live))
        jass_rterror(j, "ActorCreate: invalid parent scope");
    LONG h = (LONG)(++sc2_gactor_n);
    sc2GActor_t *a = &sc2_gactors[h - 1];
    memset(a, 0, sizeof(*a));
    a->id = (DWORD)h;
    if (type) strlcpy(a->model, type, sizeof(a->model));
    a->unit_id = scope_h ? sc2_scopes[scope_h - 1].unit : 0;
    a->scope = sc2_scope_create(j, scope_h, a->unit_id);
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

/* A unit owns one stable root scope, shared by its script-created attachments. */
static DWORD sc2_ActorScopeFromUnit(LPJASS j) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, 1, "unit");
    if (h <= 0 || h > (LONG)sc2_gunit_n)
        return jass_pushnullhandle(j, "actorscope");
    if (!sc2_unit_scope[h - 1]) sc2_unit_scope[h - 1] = sc2_scope_create(j, 0, h);
    return jass_pushlighthandle(j, (HANDLE)(uintptr_t)sc2_unit_scope[h - 1], "actorscope");
}

/* NativeLib retrieves both hosted actors through the engine's last-created reference. */
static DWORD sc2_ActorFrom(LPJASS j) {
    LPCSTR ref = jass_checkstring(j, 1);
    if (ref && !strcmp(ref, "::LastCreated"))
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)(sc2_last_actor_handle ? sc2_gactors[sc2_last_actor_handle - 1].id : 0), "actor");
    jass_rterror(j, "ActorFrom: unsupported actor reference");
    return 0;
}

/* NativeLib uses the last-created scope to remove a transmission icon after its wait completes. */
static DWORD sc2_ActorScopeFrom(LPJASS j) {
    LPCSTR ref = jass_checkstring(j, 1);
    if (ref && !strcmp(ref, "::LastCreated"))
        return jass_pushlighthandle(j, (HANDLE)(uintptr_t)sc2_scope_last, "actorscope");
    jass_rterror(j, "ActorScopeFrom: unsupported scope reference");
    return 0;
}

static DWORD sc2_ActorScopeFromActor(LPJASS j) {
    LONG h = sc2_actor_h(j, 1);
    LONG scope = h > 0 && h <= (LONG)sc2_gactor_n ? sc2_gactors[h - 1].scope : 0;
    return jass_pushlighthandle(j, (HANDLE)(uintptr_t)scope, "actorscope");
}

/* Child scopes are allocated after parents, allowing one forward pass to destroy the subtree. */
static DWORD sc2_ActorScopeKill(LPJASS j) {
    LONG scope = (LONG)(uintptr_t)jass_checkhandle(j, 1, "actorscope");
    if (!scope) return 0;
    if (scope < 1 || scope > sc2_scope_n) jass_rterror(j, "ActorScopeKill: invalid scope");
    sc2_scopes[scope - 1].live = false;
    for (LONG i = scope; i < sc2_scope_n; i++) {
        LONG parent = sc2_scopes[i].parent;
        if (parent && !sc2_scopes[parent - 1].live) sc2_scopes[i].live = false;
    }
    for (DWORD i = 0; i < sc2_gactor_n; i++) {
        sc2GActor_t *actor = &sc2_gactors[i];
        if (actor->id && !sc2_scopes[actor->scope - 1].live) {
            if (sc2_galaxy_on_actor_destroy) sc2_galaxy_on_actor_destroy(actor->id);
            memset(actor, 0, sizeof(*actor));
        }
    }
    return 0;
}

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
/* Presentation fields come from the layered ConversationState catalog, retained by the game module. */
static DWORD sc2_conversation_field(LPJASS j, LPCSTR field) {
    LPCSTR key = jass_checkstring(j, 1);
    LPCSTR value = sc2_galaxy_conversation_field(key, field);
    if (!value) jass_rterror(j, "ConversationDataState: unresolved catalog field");
    return jass_pushstring(j, value);
}
static DWORD sc2_ConversationDataStateName(LPJASS j) { return sc2_conversation_field(j, "Name"); }
static DWORD sc2_ConversationDataStateImagePath(LPJASS j) { return sc2_conversation_field(j, "ImagePath"); }
static DWORD sc2_ConversationDataStateSetValue(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_ConversationDataStateText(LPJASS j) {
    char field[128];
    snprintf(field, sizeof(field), "Text:%s", jass_checkstring(j, 2));
    return sc2_conversation_field(j, field);
}

static DWORD sc2_TimerPause(LPJASS j)                    { (void)j; return jass_pushnull(j); }
