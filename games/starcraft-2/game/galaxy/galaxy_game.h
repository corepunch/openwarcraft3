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
/* Galaxy color components are fixed-point percentages; integer byte reads aborted cinematic fades. */
static DWORD sc2_color(LPJASS j, DWORD count) {
    DWORD packed = count == 3 ? 0xff000000u : 0;
    FOR_LOOP(i, count) {
        FLOAT value = jass_checknumber(j, i + 1);
        DWORD byte = (DWORD)lroundf(MAX(0.0f, MIN(100.0f, value)) * 255.0f / 100.0f);
        packed |= byte << (i < 3 ? 16 - i * 8 : 24);
    }
    return jass_pushinteger(j, (LONG)packed);
}
static DWORD sc2_Color(LPJASS j) { return sc2_color(j, 3); }
static DWORD sc2_ColorWithAlpha(LPJASS j) { return sc2_color(j, 4); }
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
/* natives.galaxy declares one integer argument; reading a second boolean aborted campaign credit formatting. */
static DWORD sc2_FormatNumber(LPJASS j) {
    LONG v   = jass_checkinteger(j, 1);
    char raw[32], out[48];
    snprintf(raw, sizeof(raw), "%ld", (long)v);
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
/* NativeLib passes maxCount before caseSens; reading slot four as bool aborted animation setup. */
static DWORD sc2_StringReplaceWord(LPJASS j) {
    LPCSTR src = jass_checkstring(j, 1), find = jass_checkstring(j, 2), repl = jass_checkstring(j, 3);
    LONG limit = jass_checkinteger(j, 4);
    BOOL cs = jass_checkboolean(j, 5);
    if (!src || !find || !*find) return jass_pushstring(j, src ? src : "");
    size_t len = strlen(src), flen = strlen(find), rlen = repl ? strlen(repl) : 0, count = len / flen;
    /* NativeLib uses zero for all replacements; natives.galaxy also defines c_stringReplaceAll as -1. */
    if (limit > 0) count = MIN(count, (size_t)limit);
    size_t size = len + 1 + (rlen > flen ? (rlen - flen) * count : 0);
    LPSTR out = jass_alloc((long)size), dst = out;
    if (!out) jass_rterror(j, "StringReplaceWord: allocation failed");
    while (*src) {
        BOOL match = count && !(cs ? strncmp(src, find, flen) : strncasecmp(src, find, flen));
        if (match) {
            if (rlen) memcpy(dst, repl, rlen);
            dst += rlen; src += flen; count--;
        } else
            *dst++ = *src++;
    }
    *dst = '\0';
    DWORD ret = jass_pushstring(j, out);
    jass_free(out);
    return ret;
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
