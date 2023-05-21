#ifndef __UISounds_h__
#define __UISounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UISounds {
	SHEETSTR SoundName; /* CreepAggro ChatroomTimerTick RallyPointPlace ErrorMessage ItemGet ItemDrop InterfaceError InterfaceClick */
	SHEETSTR FileNames; /* CreepAggroWhat1.wav BattleNetTick.wav RallyPointPlace1.wav Error.wav PickUpItem.wav HeroDropItem1.wav Error.wav MouseClick1.wav */
	SHEETSTR DirectoryBase; /* Sound\Interface\ Sound\Interface\ Sound\Interface\ Sound\Interface\ Sound\Interface\ Sound\Interface\ Sound\Interface\ Sound\Interface\ */
	DWORD Volume; /* 127 80 90 127 60 60 127 127 */
	DWORD Pitch; /* 1 1 1 1 1 1 1 1 */
	DWORD PitchVariance; /* 1 1 1 1 1 1 1 1 */
	DWORD Priority; /* 1000 1000 1000 1000 1000 1000 1000 1000 */
	DWORD Channel; /* 8 8 8 8 8 8 8 8 */
	SHEETSTR Flags; /* 0 0 0 0 0 WANT3D 0 */
	DWORD MinDistance; /* 0 0 0 0 0 600 0 0 */
	DWORD MaxDistance; /* 0 0 0 0 0 10000 0 0 */
	DWORD DistanceCutoff; /* 10000 10000 10000 10000 10000 2000 10000 10000 */
	DWORD InsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideAngle; /* 0 0 0 0 0 0 0 0 */
	DWORD OutsideVolume; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationX; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationY; /* 0 0 0 0 0 0 0 0 */
	DWORD OrientationZ; /* 0 0 0 0 0 0 0 0 */
	DWORD EAXFlags; /* 0 0 0 0 0 0 0 0 */
};

typedef struct UISounds UISOUNDS;
typedef struct UISounds *LPUISOUNDS;
typedef struct UISounds const *LPCUISOUNDS;

LPCUISOUNDS FindUISounds(LPCSTR SoundName);
void InitUISounds(void);
void ShutdownUISounds(void);

#endif
