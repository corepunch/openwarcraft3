/* galaxy_game.h — string, text, color, game, AI, and data-table natives */
static DWORD sc2_StringExternal(LPJASS j)     { LPCSTR s = jass_checkstring(j,1); return jass_pushstring(j, s ? s : ""); }
/* Galaxy uses a null result to terminate whitespace-delimited word iteration. */
static DWORD sc2_StringWord(LPJASS j) {
    LPCSTR str = jass_checkstring(j, 1), word;
    LONG index = jass_checkinteger(j, 2);
    if (!str || index < 1) return jass_pushnull(j);
    while (*str) {
        while (*str && isspace((unsigned char)*str)) str++;
        if (!*str) break;
        word = str;
        while (*str && !isspace((unsigned char)*str)) str++;
        if (!--index) return jass_pushstringlen(j, word, (DWORD)(str - word));
    }
    return jass_pushnull(j);
}
/* Galaxy text is represented as a VM string; an integer placeholder corrupted objective-name concatenation. */
static DWORD sc2_IntToText(LPJASS j) {
    char text[32];
    snprintf(text, sizeof(text), "%ld", (long)jass_checkinteger(j, 1));
    return jass_pushstring(j, text);
}
static DWORD sc2_Color(LPJASS j)              { (void)j; return jass_pushinteger(j, 0); }
static DWORD sc2_ColorWithAlpha(LPJASS j)     { (void)j; return jass_pushinteger(j, 0); }
static DWORD sc2_GameTimeOfDayPause(LPJASS j) { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameTimeOfDaySet(LPJASS j)   { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameSetBackground(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameSetLighting(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_DifficultyEnabled(LPJASS j)  { return jass_pushboolean(j, false); }
static DWORD sc2_DifficultyName(LPJASS j)     { return jass_pushstring(j, ""); }
static DWORD sc2_DifficultyNameCampaign(LPJASS j) { return jass_pushstring(j, ""); }
static DWORD sc2_AITimePause(LPJASS j)        { (void)j; return jass_pushnull(j); }
static DWORD sc2_AIDisableAllScouting(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_CampaignMode(LPJASS j)                  { (void)j; return jass_pushnull(j); }
static DWORD sc2_DataTableSetString(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_DataTableValueExists(LPJASS j)          { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_FixedToInt(LPJASS j)                    { (void)j; return jass_pushinteger(j, 0); }
static DWORD sc2_FixedToString(LPJASS j)                 { (void)j; return jass_pushnull(j); }
static DWORD sc2_FormatNumber(LPJASS j)                  { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameCheatAllow(LPJASS j)                { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameGetSpeedValue(LPJASS j)             { (void)j; return jass_pushnumber(j, 1.0f); }
static DWORD sc2_GameIsDebugOptionSet(LPJASS j)          { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_GameIsTestMap(LPJASS j)                 { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_GameIsTransitionMap(LPJASS j)           { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_GameMapIsBlizzard(LPJASS j)             { (void)j; return jass_pushboolean(j, false); }
static DWORD sc2_GamePauseAllCharges(LPJASS j)           { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameSetSeedLocked(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameSetSpeedLocked(LPJASS j)            { (void)j; return jass_pushnull(j); }
static DWORD sc2_GameSetSpeedValue(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_IntToFixed(LPJASS j)                    { (void)j; return jass_pushnumber(j, 0.0f); }
static DWORD sc2_IntToString(LPJASS j)                   { (void)j; return jass_pushnull(j); }
static DWORD sc2_StringReplaceWord(LPJASS j)             { (void)j; return jass_pushnull(j); }
static DWORD sc2_StringSub(LPJASS j)                     { (void)j; return jass_pushnull(j); }
static DWORD sc2_StringToText(LPJASS j)                  { (void)j; return jass_pushnull(j); }
static DWORD sc2_TextCase(LPJASS j)                      { (void)j; return jass_pushnull(j); }
