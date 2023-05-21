#ifndef __AbilitySounds_h__
#define __AbilitySounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct AbilitySounds {
	SHEETSTR SoundName; /* NightElfFireSmall HumanFireLarge HumanFireMedium HumanFireSmall RainOfFireLoop RainOfFireWave BlizzardLoop BlizzardWave */
	SHEETSTR FileNames; /* NightElvesSmallFireLoop1.wav OrcHumanLargeBuildingFire1.wav OrcHumanMediumBuildingFire1.wav OrcHumanSmallBuildingFire1.wav RainOfFireLoop1.wav RainOfFireTarget1.wav,RainOfFireTarget2.wav,RainOfFireTarget3.wav BlizzardLoop1.wav BlizzardTarget1.wav,BlizzardTarget2.wav,BlizzardTarget3.wav */
	SHEETSTR DirectoryBase; /* Sound\Buildings\Fire Sound\Buildings\Fire Sound\Buildings\Fire Sound\Buildings\Fire Abilities\Spells\Demon\RainOfFire Abilities\Spells\Demon\RainOfFire Abilities\Spells\Human\Blizzard Abilities\Spells\Human\Blizzard */
	DWORD Volume; /* 127 127 127 110 127 127 127 127 */
	DWORD Pitch; /* 1 1 1 1 1 1 1 1 */
	DWORD PitchVariance; /* 1 1 1 1 1 1 1 1 */
	DWORD Priority; /* 101 103 102 101 10 10 10 10 */
	DWORD Channel; /* 14 14 14 14 13 13 13 13 */
	SHEETSTR Flags; /* WANT3D,CHANNELFULLPREEMPT WANT3D,CHANNELFULLPREEMPT WANT3D,CHANNELFULLPREEMPT WANT3D,CHANNELFULLPREEMPT WANT3D,IGNOREUSERNAME WANT3D,IGNOREUSERNAME WANT3D,IGNOREUSERNAME WANT3D,IGNOREUSERNAME */
	DWORD MinDistance; /* 700 700 700 700 750 750 750 750 */
	DWORD MaxDistance; /* 100000 100000 100000 100000 100000 100000 100000 100000 */
	DWORD DistanceCutoff; /* 2000 2000 2000 2000 2000 2000 2000 2000 */
	DWORD InsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideVolume; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationX; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationY; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationZ; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR EAXFlags; /* SpellsEAX SpellsEAX SpellsEAX SpellsEAX SpellsEAX SpellsEAX SpellsEAX SpellsEAX */
};

typedef struct AbilitySounds ABILITYSOUNDS;
typedef struct AbilitySounds *LPABILITYSOUNDS;
typedef struct AbilitySounds const *LPCABILITYSOUNDS;

LPCABILITYSOUNDS FindAbilitySounds(LPCSTR SoundName);
void InitAbilitySounds(void);
void ShutdownAbilitySounds(void);

#endif
