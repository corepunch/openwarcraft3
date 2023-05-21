#ifndef __UnitBalance_h__
#define __UnitBalance_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitBalance {
	DWORD unitBalanceID; /* hgyr hsor hmpr hmtt hmtm Hpal Hmkg Hamg */
	DWORD sortBalance; /* a2 a2 a2 a2 a2 a1 a1 a1 */
	DWORD sort2; /* fly1 cas cas art art uher uher uher */
	SHEETSTR comment; /* Gyrocopter Sorceress Priest SteamTank MortarTeam HeroPaladin HeroMountainKing HeroArchMage */
	DWORD level; /* 2 2 2 3 3 5 5 5 */
	DWORD goldcost; /* 220 180 160 230 210 500 500 500 */
	DWORD lumbercost; /* 60 20 10 60 70 100 100 100 */
	DWORD goldRep; /* 220 180 160 230 210 500 500 500 */
	DWORD lumberRep; /* 60 20 10 60 70 100 100 100 */
	DWORD fmade; /*  -   -   -   -   -  - - - */
	DWORD fused; /* 2 2 2 3 3 5 5 5 */
	DWORD bountydice; /* 6 7 6 7 7 8 8 8 */
	DWORD bountysides; /* 3 3 3 3 3 3 3 3 */
	DWORD bountyplus; /* 20 25 20 25 25 30 30 30 */
	DWORD stockMax; /* 3 3 3 3 3 3 3 3 */
	DWORD stockRegen; /* 30 30 30 30 30 30 30 30 */
	DWORD stockStart; /* 0 0 0 0 0 0 0 0 */
	DWORD HP; /* 350 250 220 700 380 100 100 100 */
	DWORD realHP; /* 350 250 220 700 380 650 700 450 */
	float regenHP; /* - 0.25 0.25 - 0.25 0.25 0.25 0.25 */
	SHEETSTR regenType; /* none always always none always always always always */
	DWORD manaN; /*  -  200 200  -   -  0 0 0 */
	DWORD realM; /*  -  200 200  -   -  255 225 285 */
	DWORD mana0; /*  -  75 75  -   -  100 100 100 */
	float regenMana; /*  -  0.5 0.5  -   -  0.01 0.01 0.01 */
	DWORD def; /* 2 0 0 2 0 2 1 0 */
	DWORD defUp; /* 2 2 2 2 2 0 0 0 */
	float realdef; /* 2 0 0 2 0 3.9 2.3 3.1 */
	SHEETSTR defType; /* medium small small fort medium hero hero hero */
	DWORD spd; /* 320 270 270 220 220 270 270 320 */
	DWORD bldtm; /* 25 30 28 55 40 55 55 55 */
	DWORD sight; /* 1800 1400 1400 1400 1400 1800 1800 1800 */
	DWORD nsight; /* 1100 800 800 800 1000 800 800 800 */
	DWORD STR; /*  -   -   -   -   -  22 24 14 */
	DWORD INT; /*  -  - - - - 17 15 19 */
	DWORD AGI; /*  -  - - - - 13 11 17 */
	float STRplus; /*  -   -   -   -   -  2.7 3 1.8 */
	float INTplus; /*  -   -   -   -   -  1.8 1.5 3.2 */
	float AGIplus; /*  -   -   -   -   -  1.5 1.5 1 */
	float abilTest; /* - - - - - 6 6 6 */
	DWORD Primary; /* _ _ _ _ _ STR STR INT */
	SHEETSTR upgrades; /* Rhar,Rhra,Rhgb Rhst Rhpt Rhar,Rhra Rhla,Rhra,Rhfl _ _ _ */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

struct UnitBalance *FindUnitBalance(DWORD unitBalanceID);
void InitUnitBalance(void);
void ShutdownUnitBalance(void);

#endif
