/* galaxy_techtree.h — tech tree, victory panel, loop, and preload natives */
static DWORD sc2_TechTreeUpgradeAddLevel(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_VictoryPanelAddAchievement(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_VictoryPanelAddCustomStatisticLine(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_VictoryPanelAddTrackedStatistic(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_IntLoopBegin(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_IntLoopDone(LPJASS j)   { return jass_pushboolean(j, true); }
static DWORD sc2_IntLoopEnd(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_IntLoopStep(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadAsset(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadImage(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadModel(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadMovie(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadObject(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadScene(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadScript(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_PreloadSound(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_TechTreeAbilityAllow(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_TechTreeAbilityIsAllowed(LPJASS j)      { (void)j; return jass_pushboolean(j, true); }
static DWORD sc2_TechTreeRestrictionsEnable(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_TechTreeUnitHelp(LPJASS j)              { (void)j; return jass_pushnull(j); }
static DWORD sc2_TechTreeUnitHelpDefault(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_TechTreeUpgradeCount(LPJASS j)          { (void)j; return jass_pushinteger(j, 0); }
