/* galaxy_sound.h — sound and soundtrack natives */

#define MAX_GALAXY_SOUNDS 256  /* links; bounds SoundLink handles retained by live Galaxy scripts */
typedef struct { char id[96]; LONG asset; } sc2GSound_t;
static sc2GSound_t sc2_gsounds[MAX_GALAXY_SOUNDS];
static LONG sc2_gsound_n = 1;

static DWORD sc2_SoundLink(LPJASS j) {
    LPCSTR id = jass_checkstring(j, 1);
    LONG asset = jass_checkinteger(j, 2), h;
    if (!id || !*id || sc2_gsound_n >= MAX_GALAXY_SOUNDS)
        return jass_pushnullhandle(j, "soundlink");
    h = sc2_gsound_n++;
    snprintf(sc2_gsounds[h].id, sizeof(sc2_gsounds[h].id), "%s", id);
    sc2_gsounds[h].asset = asset;
    return jass_pushlighthandle(j, (HANDLE)(uintptr_t)h, "soundlink");
}
static DWORD sc2_SoundLinkAsset(LPJASS j)     { return jass_pushnullhandle(j, "soundlink"); }
static DWORD sc2_SoundLinkId(LPJASS j)        { return jass_pushnullhandle(j, "soundlink"); }
static DWORD sc2_SoundPlay(LPJASS j)          { return jass_pushnullhandle(j, "sound"); }
static DWORD sc2_SoundPlayAtPoint(LPJASS j)   { return jass_pushnullhandle(j, "sound"); }
static DWORD sc2_SoundPlayOnUnit(LPJASS j)    { return jass_pushnullhandle(j, "sound"); }
static DWORD sc2_SoundPlayScene(LPJASS j)     { return jass_pushnullhandle(j, "sound"); }
static DWORD sc2_SoundPlaySceneFile(LPJASS j) { return jass_pushnullhandle(j, "sound"); }
static DWORD sc2_SoundStop(LPJASS j)          { (void)j; return jass_pushnull(j); }
static DWORD sc2_SoundWait(LPJASS j) {
    FLOAT secs = jass_checknumber(j, 2);
    if (secs > 0.0f) jass_sleep(j, (DWORD)(secs * 1000.0f));
    return jass_pushnull(j);
}
static FLOAT sc2_sound_length(LPJASS j, int arg) {
    LONG h = (LONG)(uintptr_t)jass_checkhandle(j, arg, "soundlink");
    return h > 0 && h < sc2_gsound_n && sc2_galaxy_sound_length
        ? sc2_galaxy_sound_length(sc2_gsounds[h].id, sc2_gsounds[h].asset) : 0.0f;
}
static DWORD sc2_SoundLengthSync(LPJASS j)    { return jass_pushnumber(j, sc2_sound_length(j, 1)); }
static DWORD sc2_SoundtrackPlay(LPJASS j)     { (void)j; return jass_pushnull(j); }
static DWORD sc2_SoundtrackPause(LPJASS j)    { (void)j; return jass_pushnull(j); }
static DWORD sc2_SoundtrackDefault(LPJASS j)  { (void)j; return jass_pushnull(j); }
static DWORD sc2_SoundChannelSetVolume(LPJASS j) { (void)j; return jass_pushnull(j); }
