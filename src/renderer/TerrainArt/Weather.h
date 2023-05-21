#ifndef __Weather_h__
#define __Weather_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct Weather {
	DWORD effectID; /* FDrh FDgl FDgh FDbl FDbh MEds RAlr RAhr */
	SHEETSTR name; /* WESTRING_WEATHER_DUNGEONHEAVYREDFOG WESTRING_WEATHER_DUNGEONGREENFOG WESTRING_WEATHER_DUNGEONHEAVYGREENFOG WESTRING_WEATHER_DUNGEONBLUEFOG WESTRING_WEATHER_DUNGEONHEAVYBLUEFOG WESTRING_WEATHER_DALARANSHIELD WESTRING_WEATHER_ASHENVALELIGHTRAIN WESTRING_WEATHER_ASHENVALEHEAVYRAIN */
	SHEETSTR texDir; /* ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather */
	SHEETSTR texFile; /* CloudSingleFlat CloudSingleFlat CloudSingleFlat CloudSingleFlat CloudSingleFlat PurpleEnergy rainTail rainTail */
	DWORD alphaMode; /* 1 1 1 1 1 1 1 1 */
	DWORD useFog; /* 1 1 1 1 1 0 1 1 */
	DWORD height; /* 10 10 10 10 10 10 768 768 */
	DWORD angx; /* 0 0 0 0 0 0 5 15 */
	DWORD angy; /* 0 0 0 0 0 0 5 15 */
	float emrate; /* 40 20 40 20 40 40 40 100 */
	float lifespan; /* 4 4 4 4 4 1 1.1 0.9 */
	DWORD particles; /* 3200 1600 3200 1600 3200 800 880 1800 */
	DWORD veloc; /* 20 20 20 20 20 150 -900 -1200 */
	DWORD accel; /* 0 0 0 0 0 0 0 0 */
	float var; /* 0 0 0 0 0 0 0 0 */
	DWORD texr; /* 1 1 1 1 1 4 1 1 */
	DWORD texc; /* 1 1 1 1 1 1 1 1 */
	DWORD head; /* 1 1 1 1 1 0 0 0 */
	DWORD tail; /* 0 0 0 0 0 1 1 1 */
	float taillen; /* 0.4 0.4 0.4 0.4 0.4 0.4 0.1 0.14 */
	float lati; /* 3 2 3 2 3 0 4.5 4.5 */
	DWORD long_; /* 180 180 180 180 180 180 180 180 */
	float midTime; /* 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 */
	DWORD redStart; /* 160 20 20 20 20 255 100 100 */
	DWORD greenStart; /* 20 160 160 40 40 255 150 150 */
	DWORD blueStart; /* 30 20 20 160 160 255 150 140 */
	DWORD redMid; /* 128 10 10 10 10 255 100 100 */
	DWORD greenMid; /* 20 128 128 30 30 255 150 150 */
	DWORD blueMid; /* 30 10 10 128 128 255 150 140 */
	DWORD redEnd; /* 64 5 5 5 5 255 100 100 */
	DWORD greenEnd; /* 20 64 64 20 20 255 150 150 */
	DWORD blueEnd; /* 30 5 5 64 64 255 150 140 */
	DWORD alphaStart; /* 0 0 0 0 0 128 255 255 */
	DWORD alphaMid; /* 64 16 32 16 32 255 255 255 */
	DWORD alphaEnd; /* 0 0 0 0 0 0 255 255 */
	float scaleStart; /* 20 20 20 20 20 10 1 1.5 */
	float scaleMid; /* 70 70 70 70 70 15 0.75 1 */
	float scaleEnd; /* 100 100 100 100 100 20 0.5 0.75 */
	DWORD hUVStart; /* 0 0 0 0 0 0 0 0 */
	DWORD hUVMid; /* 0 0 0 0 0 0 0 0 */
	DWORD hUVEnd; /* 0 0 0 0 0 0 0 0 */
	DWORD tUVStart; /* 0 0 0 0 0 0 0 0 */
	DWORD tUVMid; /* 0 0 0 0 0 2 0 0 */
	DWORD tUVEnd; /* 0 0 0 0 0 3 0 0 */
	SHEETSTR AmbientSound; /* - - - - - - AmbientSoundRain AmbientSoundRain */
};

struct Weather *FindWeather(DWORD effectID);
void InitWeather(void);
void ShutdownWeather(void);

#endif
