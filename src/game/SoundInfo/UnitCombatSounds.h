#ifndef __UnitCombatSounds_h__
#define __UnitCombatSounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitCombatSounds {
	SHEETSTR SoundName; /* MetalHeavyChopFlesh MetalHeavyChopEthereal MetalHeavyBashWood MetalHeavyBashStone MetalHeavyBashMetal MetalHeavyBashFlesh MetalHeavyBashEthereal AxeMediumChopWood */
	SHEETSTR FileNames; /* MetalHeavyChopFlesh1.wav,MetalHeavyChopFlesh2.wav,MetalHeavyChopFlesh3.wav, EtherealHeavyHit1.wav,EtherealHeavyHit2.wav,EtherealHeavyHit3.wav MetalHeavyBashWood1.wav,MetalHeavyBashWood2.wav,MetalHeavyBashWood3.wav, MetalHeavyBashStone1.wav,MetalHeavyBashStone2.wav,MetalHeavyBashStone3.wav, MetalHeavyBashMetal1.wav,MetalHeavyBashMetal2.wav,MetalHeavyBashMetal3.wav, MetalHeavyBashFlesh1.wav,MetalHeavyBashFlesh2.wav,MetalHeavyBashFlesh3.wav, EtherealHeavyHit1.wav,EtherealHeavyHit2.wav,EtherealHeavyHit3.wav AxeMediumChopWood1.wav,AxeMediumChopWood2.wav,AxeMediumChopWood3.wav,AxeMediumChopWood4.wav */
	SHEETSTR DirectoryBase; /* Sound\Units\Combat\ Sound\Units\Combat\ Sound\Units\Combat\ Sound\Units\Combat\ Sound\Units\Combat\ Sound\Units\Combat\ Sound\Units\Combat\ Sound\Units\Combat\ */
	DWORD Volume; /* 127 127 127 127 127 127 127 127 */
	DWORD Pitch; /* 1 1 1 1 1 1 1 1 */
	float PitchVariance; /* 0.05 0.05 0.05 0.05 0.05 0.05 0.05 0.1 */
	DWORD Priority; /* 0 0 0 0 0 0 0 0 */
	DWORD Channel; /* 5 5 5 5 5 5 5 5 */
	SHEETSTR Flags; /* WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION WANT3D,RANDOMPITCH, DYNAMICOCCLUSION */
	DWORD MinDistance; /* 600 600 600 600 600 600 600 600 */
	DWORD MaxDistance; /* 10000 10000 10000 10000 10000 10000 10000 10000 */
	DWORD DistanceCutoff; /* 2100 2100 2100 2100 2100 2100 2100 2100 */
	DWORD InsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideVolume; /* 127 127 127 127 127 127 127 127 */
	DWORD OrientationX; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationY; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationZ; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR EAXFlags; /* CombatSoundsEAX CombatSoundsEAX CombatSoundsEAX CombatSoundsEAX CombatSoundsEAX CombatSoundsEAX CombatSoundsEAX DefaultEAXON */
};

struct UnitCombatSounds *FindUnitCombatSounds(LPCSTR SoundName);
void InitUnitCombatSounds(void);
void ShutdownUnitCombatSounds(void);

#endif
