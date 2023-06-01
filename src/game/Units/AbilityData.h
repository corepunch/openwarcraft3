#ifndef __AbilityData_h__
#define __AbilityData_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct AbilityData {
	DWORD alias; /* AOwk AOmi AOcr AOww AHwe AHmt AHab AHbz */
	DWORD code; /* AOwk AOmi AOcr AOww AHwe AHmt AHab AHbz */
	DWORD uberAlias; /* AOwk AOmi AOcr AOww AHwe AHmt AHab AHbz */
	SHEETSTR comments; /* Blade Master - Wind Walk Blade Master - Mirror Image Blade Master - Critical Strike Blade Master - Bladestorm Arch Mage - Summon Water Elemental Arch Mage - Mass Teleport Arch Mage - Brilliance Aura Arch Mage - Blizzard */
	DWORD useInEditor; /* 1 1 1 1 1 1 1 1 */
	DWORD hero; /* 1 1 1 1 1 1 1 1 */
	DWORD item; /* 0 0 0 0 0 0 0 0 */
	DWORD sort; /* hero hero hero hero hero hero hero hero */
	DWORD checkDep; /* 1 1 1 1 1 1 1 1 */
	DWORD levels; /* 3 3 3 1 3 1 3 3 */
	DWORD reqLevel; /* 1 1 1 6 1 6 1 1 */
	SHEETSTR targs; /* _ air,ground ground ground,structure,debris,enemy,neutral _ ground,structure,debris,vuln,invu,notself air,ground,friend,self,vuln,invu _ */
	float Cast1; /* 0 0 0 0 0 0 0 0 */
	float Dur1; /* 20 60 0 7 75 0 0 1 */
	float HeroDur1; /* 20 60 0 5 75 0 0 1 */
	float Cool1; /* 0 3 0 240 20 15 0 6 */
	DWORD Cost1; /* 100 150 - 250 140 100 - 75 */
	DWORD Area1; /* - 1000 - - - 700 900 200 */
	DWORD Rng1; /* - 128 - - - 99999 - 800 */
	SHEETSTR Data11; /* 1 1 15 110 hwat 24 1 6 */
	DWORD Data12; /* 0.1 0 2 - - 3 - 30 */
	float Data13; /* - 2 0 - - - - 6 */
	DWORD Data14; /* - 0.5 - - - - - 0.5 */
	DWORD UnitID1; /* */
	DWORD Cast2; /* - - - - - - - - */
	float Dur2; /* 40 60 - 5 75 - - 1 */
	float HeroDur2; /* 40 60 - 5 75 - - 1 */
	DWORD Cool2; /* - 3 - 240 20 30 - 6 */
	DWORD Cost2; /* 75 150 - 250 140 100 - 75 */
	DWORD Area2; /* - 1000 - - - 700 900 225 */
	DWORD Rng2; /* - 128 - - - 99999 - 800 */
	SHEETSTR Data21; /* 1 2 15 150 hwt2 12 2 8 */
	DWORD Data22; /* 0.4 0 3 - - 5 - 40 */
	DWORD Data23; /* - 2 0 - - - - 7 */
	DWORD Data24; /* - 0.5 - - - - - 0.5 */
	DWORD UnitID2; /* */
	DWORD Cast3; /* - - - - - - - - */
	float Dur3; /* 60 60 - 5 75 - - 1 */
	DWORD HeroDur3; /* 60 60 - 5 75 - - 1 */
	DWORD Cool3; /* - 3 - 240 20 30 - 6 */
	DWORD Cost3; /* 25 150 - 250 140 100 - 75 */
	DWORD Area3; /* - 1000 - - - 700 900 250 */
	DWORD Rng3; /* - 128 - - - 99999 - 800 */
	SHEETSTR Data31; /* 1 3 15 150 hwt3 12 3 10 */
	DWORD Data32; /* 0.7 0 4 - - 5 - 50 */
	DWORD Data33; /* - 2 0 - - - - 10 */
	DWORD Data34; /* - 0.5 - - - - - 0.5 */
	DWORD UnitID3; /* */
	DWORD CastCheck; /* - - - - - - - - */
	DWORD DurCheck; /* - - - - - - - - */
	DWORD HeroDurCheck; /* - - - - - - - - */
	DWORD CoolCheck; /* - - - - - - - - */
	DWORD CostCheck; /* - - - - - - - - */
	DWORD AreaCheck; /* - - - - - - - - */
	DWORD RngCheck; /* - - - - - - - - */
};

typedef struct AbilityData ABILITYDATA;
typedef struct AbilityData *LPABILITYDATA;
typedef struct AbilityData const *LPCABILITYDATA;

LPCABILITYDATA FindAbilityData(DWORD alias);
void InitAbilityData(void);
void ShutdownAbilityData(void);

#endif
