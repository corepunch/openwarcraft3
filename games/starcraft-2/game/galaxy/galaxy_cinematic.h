/* galaxy_cinematic.h — cinematic natives */
static DWORD sc2_CinematicMode(LPJASS j) {
    BOOL  enable = jass_checkboolean(j, 2);
    FLOAT dur    = jass_checknumber(j, 3);
#ifdef SC2_DEBUG_CUTSCENE
    fprintf(stderr, "CinematicMode: enable=%d dur=%.1f\n", enable, dur);
#endif
    if (sc2_galaxy_on_cinematic) sc2_galaxy_on_cinematic(enable, dur);
    return jass_pushnull(j);
}

static DWORD sc2_CinematicFade(LPJASS j) {
    BOOL  fadein = jass_checkboolean(j, 1);
    FLOAT dur    = jass_checknumber(j, 2);
#ifdef SC2_DEBUG_CUTSCENE
    fprintf(stderr, "CinematicFade: fadein=%d dur=%.1f\n", fadein, dur);
#endif
    if (sc2_galaxy_on_fade) sc2_galaxy_on_fade(fadein ? 0.0f : 1.0f, dur);
    return jass_pushnull(j);
}

static DWORD sc2_CinematicOverlay(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_CinematicDataRun(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_CinematicDataStop(LPJASS j) { (void)j; return jass_pushnull(j); }
