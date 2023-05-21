#ifndef __AnimSounds_h__
#define __AnimSounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct AnimSounds {
	SHEETSTR SoundName; /* BerserkerRage Bloodlust HeroCinematicStep BigWaterStep WaterStep FiendStep DeepFootstep TestFootstep */
	SHEETSTR FileNames; /* BerserkerCaster.wav BloodlustTarget.wav HeroStep1.wav,HeroStep2.wav,HeroStep3.wav,HeroStep4.wav BigWaterStep1.wav,BigWaterStep2.wav,BigWaterStep3.wav,BigWaterStep4.wav WaterStep1.wav,WaterStep2.wav,WaterStep3.wav,WaterStep4.wav FiendStep1.wav,FiendStep2.wav,FiendStep3.wav,FiendStep4.wav Step1.wav,Step2.wav,Step3.wav,Step4.wav step.wav */
	SHEETSTR DirectoryBase; /* Units\Orc\Grunt\ Abilities\Spells\Orc\Bloodlust\ Sound\Units\Footsteps\ Sound\Units\Footsteps\ Sound\Units\Footsteps\ Sound\Units\Footsteps\ Sound\Units\Footsteps\ Sound\Units\Footsteps\ */
	DWORD Volume; /* 127 127 100 40 40 20 70 30 */
	DWORD Pitch; /* 1 1 1 1 1 1 1 1 */
	float PitchVariance; /* 0 0 0.1 0.1 0.1 0.1 0.1 0.1 */
	DWORD Priority; /* 8 8 3 3 3 3 3 3 */
	DWORD Channel; /* 11 11 11 11 11 11 11 11 */
	SHEETSTR Flags; /* WANT3D,CHANNELFULLPREEMPT WANT3D,CHANNELFULLPREEMPT WANT3D,RANDOMPITCH,CINEMATICSONLY WANT3D,RANDOMPITCH WANT3D,RANDOMPITCH WANT3D,RANDOMPITCH WANT3D,RANDOMPITCH WANT3D,RANDOMPITCH */
	DWORD MinDistance; /* 1000 600 800 400 400 400 800 400 */
	DWORD MaxDistance; /* 10000 10000 10000 10000 10000 10000 10000 10000 */
	DWORD DistanceCutoff; /* 3000 3000 2000 2000 2000 2000 2000 2000 */
	DWORD InsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideVolume; /* 127 127 127 127 127 127 127 127 */
	DWORD OrientationX; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationY; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationZ; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR EAXFlags; /* SpellsEAX SpellsEAX SpellsEAX DefaultEAXON DefaultEAXON DefaultEAXON SpellsEAX DefaultEAXON */
};

struct AnimSounds *FindAnimSounds(LPCSTR SoundName);
void InitAnimSounds(void);
void ShutdownAnimSounds(void);

#endif
