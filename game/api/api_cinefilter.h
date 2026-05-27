DWORD SetCineFilterTexture(LPJASS j) {
    LPCSTR filename = jass_checkstring(j, 1);
    level.cinefilter.texture = UI_LoadTexture(filename, true);
    return 0;
}
DWORD SetCineFilterBlendMode(LPJASS j) {
    BLEND_MODE *whichMode = jass_checkhandle(j, 1, "blendmode");
    level.cinefilter.blendmode = *whichMode;
    return 0;
}
DWORD SetCineFilterTexMapFlags(LPJASS j) {
    TEXMAP_FLAGS *whichFlags = jass_checkhandle(j, 1, "texmapflags");
    level.cinefilter.texmapflags = *whichFlags;
    return 0;
}
DWORD SetCineFilterStartUV(LPJASS j) {
    level.cinefilter.start.uv =
    MAKE(BOX2,
         .min = {
             jass_checknumber(j, 1),
             jass_checknumber(j, 2)
         },
         .max = {
             jass_checknumber(j, 3),
             jass_checknumber(j, 4)
         });
    return 0;
}
DWORD SetCineFilterEndUV(LPJASS j) {
    level.cinefilter.end.uv =
    MAKE(BOX2,
         .min = {
             jass_checknumber(j, 1),
             jass_checknumber(j, 2)
         },
         .max = {
             jass_checknumber(j, 3),
             jass_checknumber(j, 4)
         });
    return 0;
}
DWORD SetCineFilterStartColor(LPJASS j) {
    level.cinefilter.start.color = 
    MAKE(COLOR32,
         .r = jass_checkinteger(j, 1),
         .g = jass_checkinteger(j, 2),
         .b = jass_checkinteger(j, 3),
         .a = jass_checkinteger(j, 4));
    fprintf(stderr,
            "CineFilter start color: rgba=(%u,%u,%u,%u) time=%u\n",
            (unsigned)level.cinefilter.start.color.r,
            (unsigned)level.cinefilter.start.color.g,
            (unsigned)level.cinefilter.start.color.b,
            (unsigned)level.cinefilter.start.color.a,
            (unsigned)gi.GetTime());
    return 0;
}
DWORD SetCineFilterEndColor(LPJASS j) {
    level.cinefilter.end.color =
    MAKE(COLOR32,
         .r = jass_checkinteger(j, 1),
         .g = jass_checkinteger(j, 2),
         .b = jass_checkinteger(j, 3),
         .a = jass_checkinteger(j, 4));
    fprintf(stderr,
            "CineFilter end color: rgba=(%u,%u,%u,%u) time=%u\n",
            (unsigned)level.cinefilter.end.color.r,
            (unsigned)level.cinefilter.end.color.g,
            (unsigned)level.cinefilter.end.color.b,
            (unsigned)level.cinefilter.end.color.a,
            (unsigned)gi.GetTime());
    return 0;
}
DWORD SetCineFilterDuration(LPJASS j) {
    FLOAT duration = jass_checknumber(j, 1);
    level.cinefilter.start.time = gi.GetTime();
    level.cinefilter.end.time = gi.GetTime() + duration * 1000;
    fprintf(stderr,
            "CineFilter duration: %.3f start=%u end=%u\n",
            duration,
            (unsigned)level.cinefilter.start.time,
            (unsigned)level.cinefilter.end.time);
    return 0;
}
DWORD DisplayCineFilter(LPJASS j) {
    level.cinefilter.displayed = jass_checkboolean(j, 1);
    fprintf(stderr,
            "CineFilter display: %d time=%u fade=%.3f\n",
            level.cinefilter.displayed,
            (unsigned)gi.GetTime(),
            G_Cinefade());
    return 0;
}
DWORD IsCineFilterDisplayed(LPJASS j) {
    return jass_pushboolean(j, level.cinefilter.displayed);
}
