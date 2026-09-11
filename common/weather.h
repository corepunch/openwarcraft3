#ifndef common_weather_h
#define common_weather_h

#include "common/shared.h"

#define MAX_WEATHER_EFFECTS 256 // effects; bounds the per-client weather snapshot and stable server registry
#define BZ_GAME_DATAGRAM_ENTITY_TINTS 0x8000u /* first weather-count bit: full per-entity RGBA list follows weather */
_Static_assert(MAX_WEATHER_EFFECTS < BZ_GAME_DATAGRAM_ENTITY_TINTS, "weather count must leave the game-datagram extension bit free");

typedef struct {
    DWORD handle;
    DWORD effect_id;
    BOX2 bounds;
    DWORD enabled;
} wc3WeatherEffect_t;

#endif
