/* galaxy_transmission.h — transmission natives */
static DWORD sc2_TransmissionSource(LPJASS j)        { return jass_pushnullhandle(j, "transmissionsource"); }
static DWORD sc2_TransmissionSourceFromModel(LPJASS j){ return jass_pushnullhandle(j, "transmissionsource"); }
static DWORD sc2_TransmissionSourceFromUnit(LPJASS j) { return jass_pushnullhandle(j, "transmissionsource"); }
/* TransmissionSend: send a transmission to players; sleep if waitUntilDone and duration > 0. */
static DWORD sc2_TransmissionSend(LPJASS j) {
    /* args: playergroup, source, camerainfo, string anim, soundlink, text speaker,
     *       text msg, fixed duration, int durationType, bool waitUntilDone */
    FLOAT sound_dur  = sc2_sound_length(j, 5);
    FLOAT dur        = jass_checknumber(j, 8);
    LONG  dur_type   = jass_checkinteger(j, 9);
    BOOL  wait_done  = jass_checkboolean(j, 10);
    LONG  sound_h    = (LONG)(uintptr_t)jass_checkhandle(j, 5, "soundlink");
    /* Native SC2 derives transmission time from the linked asset before applying the requested modifier. */
    if (dur_type == 0) dur = sound_dur;
    else if (dur_type == 1) dur += sound_dur;
    else if (dur_type == 2) dur = MAX(sound_dur - dur, 0.0f);
    if (sound_h > 0 && sound_h < sc2_gsound_n && sc2_galaxy_on_sound)
        sc2_galaxy_on_sound(sc2_gsounds[sound_h].id, sc2_gsounds[sound_h].asset);
    fprintf(stderr, "TransmissionSend: sound=%s asset=%ld duration=%.2f wait=%d\n",
            sound_h > 0 && sound_h < sc2_gsound_n ? sc2_gsounds[sound_h].id : "(null)",
            sound_h > 0 && sound_h < sc2_gsound_n ? (long)sc2_gsounds[sound_h].asset : -1L, dur, wait_done);
    if (wait_done && dur > 0.0f)
        jass_sleep(j, (DWORD)(dur * 1000.0f));
    return jass_pushnullhandle(j, "sound");
}
static DWORD sc2_TransmissionLastSent(LPJASS j)      { return jass_pushinteger(j, 0); }
static DWORD sc2_TransmissionClear(LPJASS j)         { (void)j; return jass_pushnull(j); }
static DWORD sc2_TransmissionClearAll(LPJASS j)      { (void)j; return jass_pushnull(j); }
static DWORD sc2_TransmissionWait(LPJASS j) {
    FLOAT secs = jass_checknumber(j, 2);
    if (secs > 0.0f) jass_sleep(j, (DWORD)(secs * 1000.0f));
    return jass_pushnull(j);
}
static DWORD sc2_TransmissionSetOption(LPJASS j)     { (void)j; return jass_pushnull(j); }
