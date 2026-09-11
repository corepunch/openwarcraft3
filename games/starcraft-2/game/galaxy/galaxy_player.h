/* galaxy_player.h — player and playergroup natives */
static DWORD sc2_PlayerGroupAll(LPJASS j)            { return jass_pushnullhandle(j, "playergroup"); }
static DWORD sc2_PlayerGroupActive(LPJASS j)         { return jass_pushnullhandle(j, "playergroup"); }
static DWORD sc2_PlayerGroupAdd(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGroupClear(LPJASS j)          { return jass_pushnullhandle(j, "playergroup"); }
static DWORD sc2_PlayerGroupCopy(LPJASS j)           { return jass_pushnullhandle(j, "playergroup"); }
/* PlayerGroupCount / PlayerGroupPlayer: singleplayer stubs — exactly 1 human player. */
static DWORD sc2_PlayerGroupCount(LPJASS j) {
    (void)jass_checkhandle(j, 1, "playergroup");
    return jass_pushinteger(j, 1);
}
static DWORD sc2_PlayerGroupEmpty(LPJASS j)          { return jass_pushnullhandle(j, "playergroup"); }
static DWORD sc2_PlayerGroupHasPlayer(LPJASS j)      { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerGroupPlayer(LPJASS j) {
    (void)jass_checkhandle(j, 1, "playergroup");
    (void)jass_checkinteger(j, 2);
    return jass_pushinteger(j, 1);
}
static DWORD sc2_PlayerGroupLoopBegin(LPJASS j)      { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGroupLoopDone(LPJASS j)       { return jass_pushboolean(j, true); }
static DWORD sc2_PlayerGroupLoopEnd(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGroupLoopStep(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGroupRemove(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGroupSingle(LPJASS j)         { return jass_pushnullhandle(j, "playergroup"); }
static DWORD sc2_PlayerGroupAlliance(LPJASS j)       { return jass_pushnullhandle(j, "playergroup"); }
static DWORD sc2_PlayerGetState(LPJASS j)            { return jass_pushinteger(j, 0); }
static DWORD sc2_PlayerSetState(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGetAlliance(LPJASS j)         { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerSetAlliance(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerDifficulty(LPJASS j)          { return jass_pushinteger(j, 0); }
static DWORD sc2_PlayerModifyPropertyInt(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerAddChargeRegen(LPJASS j)      { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerAddChargeUsed(LPJASS j)       { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerAddCooldown(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerGetChargeRegen(LPJASS j)      { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_PlayerGetChargeUsed(LPJASS j)       { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_PlayerGetCooldown(LPJASS j)         { return jass_pushnumber(j, 0.0f); }
static DWORD sc2_PlayerScoreValueEnable(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerScoreValueEnableAll(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerScoreValueGetAsFixed(LPJASS j){ return jass_pushnumber(j, 0.0f); }
static DWORD sc2_PlayerScoreValueGetAsInt(LPJASS j)  { return jass_pushinteger(j, 0); }
static DWORD sc2_PlayerScoreValueSetFromFixed(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerScoreValueSetFromInt(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerPauseAllCharges(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerPauseAllCooldowns(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerBeaconAlert(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerBeaconClearTarget(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerBeaconGetTargetPoint(LPJASS j){ return jass_pushinteger(j, 0); }
static DWORD sc2_PlayerBeaconGetTargetUnit(LPJASS j) { return jass_pushinteger(j, 0); }
static DWORD sc2_PlayerBeaconIsAutoCast(LPJASS j)    { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerBeaconIsFromUser(LPJASS j)    { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerBeaconIsSet(LPJASS j)         { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerBeaconSetAutoCast(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerBeaconSetTargetPoint(LPJASS j){ (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerBeaconSetTargetUnit(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerCreateEffectPoint(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerCreateEffectUnit(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_PlayerValidateEffectPoint(LPJASS j) { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerValidateEffectUnit(LPJASS j)  { return jass_pushboolean(j, false); }
static DWORD sc2_PlayerType(LPJASS j)                { (void)j; return jass_pushinteger(j, 0); }
