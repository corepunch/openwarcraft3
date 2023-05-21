#ifndef __UnitAckSounds_h__
#define __UnitAckSounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitAckSounds {
	SHEETSTR SoundName; /* CaptainYesAttack CaptainPissed CaptainWhat ArthasWarcry ArthasYes ArthasYesAttack ArthasPissed ArthasWhat */
	SHEETSTR FileNames; /* CaptainYesAttack1.wav CaptainPissed1.wav,CaptainPissed2.wav,CaptainPissed3.wav CaptainWhat1.wav,CaptainWhat2.wav,CaptainWhat3.wav ArthasWarcry1.wav ArthasYes1.wav,ArthasYes2.wav,ArthasYes3.wav,ArthasYes4.wav ArthasYesAttack1.wav,ArthasYesAttack2.wav,ArthasYesAttack3.wav,ArthasYesAttack4.wav,ArthasYesAttack5.wav,ArthasYesAttack6.wav ArthasPissed1.wav,ArthasPissed2.wav,ArthasPissed3.wav,ArthasPissed4.wav,ArthasPissed5.wav,ArthasPissed6.wav,ArthasPissed7.wav ArthasWhat1.wav,ArthasWhat2.wav,ArthasWhat3.wav,ArthasWhat4.wav,ArthasWhat5.wav */
	SHEETSTR DirectoryBase; /* Units\Human\TheCaptain\ Units\Human\TheCaptain\ Units\Human\TheCaptain\ Units\Human\Arthas\ Units\Human\Arthas\ Units\Human\Arthas\ Units\Human\Arthas\ Units\Human\Arthas\ */
	DWORD Volume; /* 127 127 127 127 127 127 127 127 */
	float Pitch; /* 1 1 1 1 1 1 1 1 */
	float PitchVariance; /* 0.01 0.01 0.01 0.01 0.01 0.01 0.01 0.01 */
	DWORD Priority; /* 1000 1000 1000 1000 1000 1000 1000 1000 */
	DWORD Channel; /* 2 1 1 1 3 2 1 1 */
	SHEETSTR Flags; /* WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH WANT3D,NODUPEUSERNAMES,CHANNELFULLPREEMPT,RANDOMPITCH */
	DWORD MinDistance; /* 3000 3000 3000 3000 3000 3000 3000 3000 */
	DWORD MaxDistance; /* 10000 10000 10000 10000 10000 10000 10000 10000 */
	DWORD DistanceCutoff; /* 100000 100000 100000 100000 100000 100000 100000 100000 */
	DWORD InsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideVolume; /* 127 127 127 127 127 127 127 127 */
	DWORD OrientationX; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationY; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationZ; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR EAXFlags; /* HeroAcksEAX HeroAcksEAX HeroAcksEAX HeroAcksEAX HeroAcksEAX HeroAcksEAX HeroAcksEAX HeroAcksEAX */
};

typedef struct UnitAckSounds UNITACKSOUNDS;
typedef struct UnitAckSounds *LPUNITACKSOUNDS;
typedef struct UnitAckSounds const *LPCUNITACKSOUNDS;

LPCUNITACKSOUNDS FindUnitAckSounds(LPCSTR SoundName);
void InitUnitAckSounds(void);
void ShutdownUnitAckSounds(void);

#endif
