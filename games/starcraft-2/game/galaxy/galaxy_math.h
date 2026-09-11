/* galaxy_math.h — math natives */
static DWORD sc2_AbsF(LPJASS j)  { FLOAT x = jass_checknumber(j, 1); return jass_pushnumber(j, x < 0 ? -x : x); }
static DWORD sc2_MaxF(LPJASS j)  { FLOAT a = jass_checknumber(j,1), b = jass_checknumber(j,2); return jass_pushnumber(j, a > b ? a : b); }
static DWORD sc2_MinF(LPJASS j)  { FLOAT a = jass_checknumber(j,1), b = jass_checknumber(j,2); return jass_pushnumber(j, a < b ? a : b); }
static DWORD sc2_ModF(LPJASS j)  { FLOAT a = jass_checknumber(j,1), b = jass_checknumber(j,2); return jass_pushnumber(j, b ? fmodf(a, b) : 0.0f); }
static DWORD sc2_Pow(LPJASS j)   { return jass_pushnumber(j, powf(jass_checknumber(j,1), jass_checknumber(j,2))); }
static DWORD sc2_SquareRoot(LPJASS j) { FLOAT x = jass_checknumber(j,1); return jass_pushnumber(j, x > 0.0f ? sqrtf(x) : 0.0f); }
static DWORD sc2_Sin(LPJASS j)   { return jass_pushnumber(j, sinf(jass_checknumber(j,1))); }
static DWORD sc2_Cos(LPJASS j)   { return jass_pushnumber(j, cosf(jass_checknumber(j,1))); }
static DWORD sc2_Tan(LPJASS j)   { return jass_pushnumber(j, tanf(jass_checknumber(j,1))); }
static DWORD sc2_ASin(LPJASS j)  { return jass_pushnumber(j, asinf(jass_checknumber(j,1))); }
static DWORD sc2_ACos(LPJASS j)  { return jass_pushnumber(j, acosf(jass_checknumber(j,1))); }
static DWORD sc2_ATan(LPJASS j)  { return jass_pushnumber(j, atanf(jass_checknumber(j,1))); }
static DWORD sc2_ATan2(LPJASS j) { return jass_pushnumber(j, atan2f(jass_checknumber(j,1), jass_checknumber(j,2))); }
static DWORD sc2_RandomFixed(LPJASS j) {
    FLOAT lo = jass_checknumber(j, 1), hi = jass_checknumber(j, 2);
    FLOAT r = lo + (hi - lo) * ((FLOAT)rand() / (FLOAT)RAND_MAX);
    return jass_pushnumber(j, r);
}
static DWORD sc2_RandomInt(LPJASS j) {
    LONG lo = jass_checkinteger(j, 1), hi = jass_checkinteger(j, 2);
    LONG r = hi > lo ? lo + rand() % (hi - lo + 1) : lo;
    return jass_pushinteger(j, r);
}
