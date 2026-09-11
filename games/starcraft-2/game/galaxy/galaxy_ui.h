/* galaxy_ui.h — UI, objective, ping, and help-panel natives */
static DWORD sc2_ObjectiveCreate3(LPJASS j)   { return jass_pushinteger(j, 0); }
static DWORD sc2_ObjectiveLastCreated(LPJASS j){ return jass_pushinteger(j, 0); }
static DWORD sc2_ObjectiveSetName(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_ObjectiveSetState(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_ObjectiveGetState(LPJASS j)  { return jass_pushinteger(j, 0); }
static DWORD sc2_PingLastCreated(LPJASS j)    { return jass_pushinteger(j, 0); }
static DWORD sc2_PingDestroy(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_PingSetScale(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_PingSetTooltip(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_MinimapPing(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_UISetMode(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_UIAlertPoint(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_UIAlertUnit(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_UISetFrameVisible(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_HelpPanelAddTip(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_HelpPanelDisplayPage(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_HelpPanelEnableTechTreeButton(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_DialogControlSetPropertyAsText(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_DialogControlSetVisible(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_HelpPanelAddTutorial(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_HelpPanelShowTechTreeRace(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_ObjectiveGetPrimary(LPJASS j)           { (void)j; return jass_pushnullhandle(j, "objective"); }
static DWORD sc2_PingCreate(LPJASS j)                    { (void)j; return jass_pushnull(j); }
static DWORD sc2_UIClearMessages(LPJASS j)               { (void)j; return jass_pushnull(j); }
static DWORD sc2_UIFlyerHelperClearOverride(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_UIFlyerHelperOverride(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_UIFrameVisible(LPJASS j)                { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_UISetCursorVisible(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_UISetGameMenuItemVisible(LPJASS j)      { (void)j; return jass_pushnull(j); }
static DWORD sc2_UISetRestartLoadingScreen(LPJASS j)     { (void)j; return jass_pushnull(j); }
