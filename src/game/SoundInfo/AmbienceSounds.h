#ifndef __AmbienceSounds_h__
#define __AmbienceSounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct AmbienceSounds {
	SHEETSTR SoundName; /* NightElfGrowingLoop OrcBuildingConstructionLoop BuildingConstructionLoop HumanHeroArchMageMovement OrcCatapultMovement GoblinZeppelinMovement HumanGyrocopterMovement HumanKnightMovement */
	SHEETSTR FileNames; /* NightElfBuildingLoop.wav OrcBuildingLoop.wav ConstructionLoop1.wav,ConstructionLoop2.wav HorseLoop1.wav,HorseLoop2.wav,HorseLoop3.wav CatapultLoop.wav ZeppelinLoop.wav GyrocopterLoop1.wav,GyrocopterLoop2.wav HorseLoop1.wav,HorseLoop2.wav,HorseLoop3.wav */
	SHEETSTR DirectoryBase; /* Sound\Buildings\NightElf\ Sound\Buildings\Orc\ Sound\Buildings\Shared\ Units\Human\Knight\ Units\Orc\Catapult\ Units\Creeps\GoblinZeppelin\ Units\Human\Gyrocopter\ Units\Human\Knight\ */
	DWORD Volume; /* 90 90 60 55 80 50 50 55 */
	DWORD Pitch; /* 1 1 1 1 1 1 1 1 */
	DWORD PitchVariance; /* 1 1 1 1 1 1 1 1 */
	DWORD Priority; /* 1000 1000 1000 1000 1000 1000 1000 1000 */
	DWORD Channel; /* 9 9 9 9 9 9 9 9 */
	SHEETSTR Flags; /* WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY WANT3D,CHANNELFULLPREEMPT,NODUPLICATES,SCALEPRIORITY */
	DWORD MinDistance; /* 400 400 400 300 300 300 300 300 */
	DWORD MaxDistance; /* 5000 5000 5000 5000 5000 5000 5000 5000 */
	DWORD DistanceCutoff; /* 2500 2500 2500 2500 2500 2500 2500 2500 */
	DWORD insideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD outsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD outsideConeVolume; /* 127 127 127 127 127 127 127 127 */
	DWORD coneOrientationX; /* 0 0 0 0 0 0 0 0 */
	DWORD coneOrientationY; /* 0 0 0 0 0 0 0 0 */
	DWORD ConeOrientationZ; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR EAXFlags; /* DefaultEAXON DefaultEAXON DefaultEAXON DefaultEAXON DefaultEAXON DefaultEAXON DefaultEAXON DefaultEAXON */
};

struct AmbienceSounds *FindAmbienceSounds(LPCSTR SoundName);
void InitAmbienceSounds(void);
void ShutdownAmbienceSounds(void);

#endif
