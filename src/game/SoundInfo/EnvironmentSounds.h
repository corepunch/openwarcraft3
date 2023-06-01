#ifndef __EnvironmentSounds_h__
#define __EnvironmentSounds_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct EnvironmentSounds {
	SHEETSTR EnvironmentType; /* psychotic lake forest Default */
	SHEETSTR RoomType; /* PSYCHOTIC PLAIN FOREST MOUNTAINS */
	DWORD EffectVolume; /* 1 1 1 1 */
	float DecayTime; /* 20 0 20 2.5 */
	DWORD Damping; /* 0 1 20 1 */
	DWORD Size; /* 100 1 100 10 */
	DWORD Diffusion; /* 0 1 0 1 */
	DWORD Room; /* 0 0 0 0 */
	DWORD RoomHF; /* 0 0 0 0 */
	DWORD DecayHFRatio; /* 0 20 0 1 */
	DWORD Reflections; /* 0 0 0 -2700 */
	float ReflectionsDelay; /* 0.3 0 0.3 0.15 */
	DWORD Reverb; /* 20 0 0 -3000 */
	float ReverbDelay; /* 0.1 0 0.1 0.5 */
	DWORD RoomRolloff; /* 0 0 0 0 */
	DWORD AirAbsorption; /* 0 0 0 -2 */
};

typedef struct EnvironmentSounds ENVIRONMENTSOUNDS;
typedef struct EnvironmentSounds *LPENVIRONMENTSOUNDS;
typedef struct EnvironmentSounds const *LPCENVIRONMENTSOUNDS;

LPCENVIRONMENTSOUNDS FindEnvironmentSounds(LPCSTR EnvironmentType);
void InitEnvironmentSounds(void);
void ShutdownEnvironmentSounds(void);

#endif
