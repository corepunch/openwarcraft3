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
static DWORD sc2_Color(LPJASS j) {
    LONG r = jass_checkinteger(j,1), g = jass_checkinteger(j,2), b = jass_checkinteger(j,3);
    return jass_pushinteger(j, (0xFF << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF));
}
static DWORD sc2_ColorWithAlpha(LPJASS j) {
    LONG r = jass_checkinteger(j,1), g = jass_checkinteger(j,2), b = jass_checkinteger(j,3), a = jass_checkinteger(j,4);
    return jass_pushinteger(j, ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF));
}
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
static DWORD sc2_FixedToInt(LPJASS j)  { return jass_pushinteger(j, (LONG)jass_checknumber(j, 1)); }
static DWORD sc2_FixedToString(LPJASS j) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%.4g", (double)jass_checknumber(j, 1));
    return jass_pushstring(j, buf);
}
static DWORD sc2_FormatNumber(LPJASS j) {
    LONG v   = jass_checkinteger(j, 1);
    BOOL sep = jass_checkboolean(j, 2);
    char raw[32], out[48];
    snprintf(raw, sizeof(raw), "%ld", (long)v);
    if (!sep) return jass_pushstring(j, raw);
    /* Insert thousands separators. */
    LONG len = (LONG)strlen(raw), start = (v < 0) ? 1 : 0;
    LONG digits = len - start, wr = 0;
    if (start) out[wr++] = '-';
    for (LONG i = 0; i < digits; i++) {
        if (i > 0 && (digits - i) % 3 == 0) out[wr++] = ',';
        out[wr++] = raw[start + i];
    }
    out[wr] = '\0';
    return jass_pushstring(j, out);
}
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
static DWORD sc2_IntToFixed(LPJASS j) { return jass_pushnumber(j, (FLOAT)jass_checkinteger(j, 1)); }
static DWORD sc2_IntToString(LPJASS j) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long)jass_checkinteger(j, 1));
    return jass_pushstring(j, buf);
}
/* Galaxy uses 1-based, inclusive string indices. */
static DWORD sc2_StringSub(LPJASS j) {
    LPCSTR src = jass_checkstring(j, 1);
    LONG start = jass_checkinteger(j, 2) - 1;
    LONG end   = jass_checkinteger(j, 3);
    if (!src) return jass_pushstring(j, "");
    LONG len = (LONG)strlen(src);
    if (start < 0) start = 0;
    if (end > len) end = len;
    if (start >= end) return jass_pushstring(j, "");
    return jass_pushstringlen(j, src + start, (DWORD)(end - start));
}
static DWORD sc2_StringReplaceWord(LPJASS j) {
    LPCSTR src = jass_checkstring(j, 1);
    LPCSTR find = jass_checkstring(j, 2);
    LPCSTR repl = jass_checkstring(j, 3);
    BOOL   cs   = jass_checkboolean(j, 4);
    if (!src || !find || !*find) return jass_pushstring(j, src ? src : "");
    /* Build result in a local buffer (up to 1KB to keep it simple). */
    char out[1024]; LONG wr = 0;
    LONG flen = (LONG)strlen(find), rlen = (LONG)strlen(repl);
    while (*src && wr < (LONG)sizeof(out) - 1) {
        int match = cs ? (strncmp(src, find, (size_t)flen) == 0)
                       : (strncasecmp(src, find, (size_t)flen) == 0);
        if (match) {
            for (LONG i = 0; i < rlen && wr < (LONG)sizeof(out) - 1; i++)
                out[wr++] = repl[i];
            src += flen;
        } else {
            out[wr++] = *src++;
        }
    }
    out[wr] = '\0';
    return jass_pushstring(j, out);
}
/* text and string are the same underlying type in Galaxy; just forward the string. */
static DWORD sc2_StringToText(LPJASS j) {
    LPCSTR s = jass_checkstring(j, 1);
    return jass_pushstring(j, s ? s : "");
}
static DWORD sc2_TextCase(LPJASS j) {
    LPCSTR src  = jass_checkstring(j, 1);
    BOOL upper  = jass_checkboolean(j, 2);
    if (!src) return jass_pushstring(j, "");
    char buf[512]; LONG i = 0;
    for (; src[i] && i < (LONG)sizeof(buf) - 1; i++)
        buf[i] = (char)(upper ? toupper((unsigned char)src[i]) : tolower((unsigned char)src[i]));
    buf[i] = '\0';
    return jass_pushstring(j, buf);
}
