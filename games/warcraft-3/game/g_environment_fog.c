#include "g_local.h"
#include "common/stb_slk.h"

/* Warsmash reads DefaultZFog from the merged MiscData table at the active
 * RoC/TFT version index.  The runtime scene starts unfogged; this row is the
 * value restored by ResetTerrainFog rather than an implicit map-load enable. */
static BOOL G_EnvironmentFogListValue(LPCSTR value, DWORD index, LPFLOAT out) {
    char *stop = NULL;
    FLOAT parsed = 0.0f;

    if (!value || !out) return false;
    FOR_LOOP(i, index + 1) {
        while (*value == ' ' || *value == '\t') value++;
        if (!*value) return false;
        parsed = strtof(value, &stop);
        if (!stop || stop == value) return false;
        value = stop;
        while (*value == ' ' || *value == '\t') value++;
        if (i == index) {
            *out = parsed;
            return true;
        }
        if (*value != ',') return false;
        value++;
    }
    return false;
}

static BOOL G_EnvironmentFogDefault(wc3EnvironmentFogState_t *fog) {
    if (!fog) return false;
    *fog = (wc3EnvironmentFogState_t){ .style = WC3_ENV_FOG_NONE };
    DWORD const version = gi.CvarString && atoi(gi.CvarString("fs_expansion", "0")) != 0 ? 1u : 0u;
    LPCSTR style_value = Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Style");
    FLOAT style = -1.0f;
    FLOAT alpha = 255.0f, red = 0.0f, green = 0.0f, blue = 0.0f;

    if (!G_EnvironmentFogListValue(style_value, version, &style)) return false;

    /* FogSettings.parse() maps the authored -1/0/1/2 style through +1 to
     * NONE/LINEAR/EXP/EXP2.  Invalid values are disabled. */
    if (style >= -1.0f && style <= 2.0f) fog->style = (LONG)style + 1;
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Start"), version, &fog->start);
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "End"), version, &fog->end);
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Density"), version, &fog->density);
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Color"), version * 4u + 0u, &alpha);
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Color"), version * 4u + 1u, &red);
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Color"), version * 4u + 2u, &green);
    G_EnvironmentFogListValue(Stb_IniCacheFind(&game.config.misc, "DefaultZFog", "Color"), version * 4u + 3u, &blue);
    (void)alpha; /* viewDef currently carries RGB only. */
    fog->color = (VECTOR3){ red / 255.0f, green / 255.0f, blue / 255.0f };
    return true;
}

void G_EnvironmentFogPublish(void) {
    wc3EnvironmentFogState_t const *fog = &level.environment_fog.active;
    char value[MAX_PATHLEN];

    if (!gi.configstring) return;
    /* Keep style/density on the wire even though the generic renderer currently
     * renders all enabled styles with its Warsmash-compatible linear path. */
    snprintf(value, sizeof(value), "%d %.9g %.9g %.9g %.9g %.9g %.9g",
             fog->style,
             (double)fog->start, (double)fog->end, (double)fog->density,
             (double)fog->color.x, (double)fog->color.y, (double)fog->color.z);
    gi.configstring(CS_SCENE_FOG, value);
}

void G_EnvironmentFogInitMap(void) {
    level.environment_fog.defaults_valid = G_EnvironmentFogDefault(&level.environment_fog.defaults);
    level.environment_fog.active = (wc3EnvironmentFogState_t){ .style = WC3_ENV_FOG_NONE };
    G_EnvironmentFogPublish();
}

void G_EnvironmentFogSet(LONG style, FLOAT start, FLOAT end, FLOAT density,
                         FLOAT red, FLOAT green, FLOAT blue) {
    wc3EnvironmentFogState_t *fog = &level.environment_fog.active;

    fog->start = start;
    fog->end = end;
    fog->density = density;
    fog->color = (VECTOR3){ red, green, blue };
    fog->style = style >= -1 && style <= 2 ? style + 1 : WC3_ENV_FOG_NONE;
    G_EnvironmentFogPublish();
}

void G_EnvironmentFogReset(void) {
    if (!level.environment_fog.defaults_valid) return;
    level.environment_fog.active = level.environment_fog.defaults;
    G_EnvironmentFogPublish();
}
