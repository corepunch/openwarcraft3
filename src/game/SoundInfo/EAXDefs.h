#ifndef __EAXDefs_h__
#define __EAXDefs_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct EAXDefs {
	SHEETSTR EAXLabel; /* DoodadsEAX HeroAcksEAX MissilesEAX SpellsEAX KotoDrumsEAX CombatSoundsEAX DefaultEAXON */
	SHEETSTR DisplayText; /* WESTRING_EAX_DOODADS WESTRING_EAX_HERO WESTRING_EAX_MISSILES WESTRING_EAX_SPELLS WESTRING_EAX_KOTODRUMS WESTRING_EAX_COMBAT WESTRING_EAX_DEFAULT */
	DWORD EffectsLevel; /* 0 0 0 0 0 0 0 */
	DWORD EAX2Direct; /* 0 0 0 0 0 0 0 */
	DWORD EAX2DirectHF; /* 0 0 0 0 0 0 0 */
	DWORD EAX2Room; /* -100 -300 -500 -100 -600 -1200 -1400 */
	DWORD EAX2RoomHF; /* 0 0 0 0 0 0 0 */
	DWORD EAX2Obstruction; /* 0 0 0 0 0 0 0 */
	DWORD EAX2ObstructionLFRatio; /* 0 0 0 0 0 0 0 */
	DWORD EAX2Occlusion; /* 0 0 0 0 0 0 0 */
	DWORD EAX2OcclusionLFRatio; /* 0 0 0 0 0 0 0 */
	DWORD EAX2OcclusionRoomRatio; /* 0 0 0 0 0 0 0 */
	DWORD EAX2RoomRolloff; /* 0 0 0 0 0 0 0 */
	DWORD EAX2AirAbsorption; /* 0 0 0 0 0 0 0 */
	DWORD EAX2OutsideVolumeHF; /* 0 0 0 0 0 0 0 */
	DWORD EAX2AutoRoomLevels; /* 0 0 0 0 0 0 0 */
	DWORD Fast2DPredelayTime; /* 0 0 0 0 0 0 0 */
	DWORD Fast2DDamping; /* 20000 20000 20000 20000 20000 20000 20000 */
	DWORD Fast2DReverbTime; /* 2000 2000 2000 2000 2000 2000 2000 */
	DWORD Fast2DReverbQuality; /* 0 0 0 0 0 0 0 */
	float Fast2DOcclusionScale; /* 0.05 0 0.1 0.05 0.2 0.1 0 */
};

typedef struct EAXDefs EAXDEFS;
typedef struct EAXDefs *LPEAXDEFS;
typedef struct EAXDefs const *LPCEAXDEFS;

LPCEAXDEFS FindEAXDefs(LPCSTR EAXLabel);
void InitEAXDefs(void);
void ShutdownEAXDefs(void);

#endif
