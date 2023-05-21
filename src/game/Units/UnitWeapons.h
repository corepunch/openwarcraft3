#ifndef __UnitWeapons_h__
#define __UnitWeapons_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitWeapons {
	DWORD unitWeapID; /* hmil hkni hgyr hgry hfoo Hpal Hmkg Hamg */
	DWORD sortWeap; /* a2 a2 a2 a2 a2 a1 a1 a1 */
	DWORD sort2; /* me1 me2 fly1 fly2 me1 uher uher uher */
	SHEETSTR comment; /* Militia Knight Gyrocopter GryphonRider Footman HeroPaladin HeroMountainKing HeroArchMage */
	DWORD weapsOn; /* 1 1 1 3 1 1 1 1 */
	SHEETSTR weapType1; /* MetalMediumSlice MetalMediumSlice _ _ MetalMediumSlice MetalHeavyBash MetalHeavyBash _ */
	SHEETSTR targs1; /* ground,structure,debris,item,ward ground,structure,debris,item,ward air ground,structure,debris,item,ward ground,structure,debris,item,ward ground,structure,debris,item,ward ground,structure,debris,item,ward ground,structure,debris,air,item,ward */
	DWORD acquire; /* 500 500 500 500 500 500 500 600 */
	DWORD minRange; /* - - - - - - - - */
	DWORD rangeN1; /* 90 100 500 450 90 100 100 600 */
	DWORD RngTst; /* - - - - - - - - */
	DWORD RngBuff1; /* 250 250 250 250 250 250 250 250 */
	SHEETSTR atkType1; /* normal normal pierce pierce normal normal normal normal */
	SHEETSTR weapTp1; /* normal normal instant mline normal normal normal missile */
	float cool1; /* 1.2 1.36 1 2.4 1.35 2.2 2.22 2.13 */
	float mincool1; /* - - - - - - - - */
	DWORD dice1; /* 1 2 1 1 1 2 2 2 */
	DWORD sides1; /* 2 5 6 11 2 6 6 4 */
	DWORD dmgplus1; /* 11 19 24 44 11 0 0 0 */
	SHEETSTR DmgUpg; /* - - - - - - - - */
	SHEETSTR dmod1; /* - - 26 - - - - - */
	DWORD dmgUp1; /* - - - - - - - - */
	DWORD mindmg1; /* 12 21 25 45 12 2 2 2 */
	float avgdmg1; /* 12.5 25 27.5 50 12.5 7 7 5 */
	SHEETSTR DPS; /* 10.416666666666713 18.382352941176529 27.5 20.833333333333355 9.2592592592592613 3.1818181818181812 3.1531531531531512 2.347417840375598 */
	DWORD maxdmg1; /* 13 29 30 55 13 12 12 8 */
	float dmgpt1; /* 0.39 0.66 0.03 0.8 0.5 0.433 0.35 0.55 */
	float backSw1; /* 0.44 0.44 0.97 0.87 0.5 0.567 0.65 0.85 */
	DWORD Farea1; /*  -   -   -   -   -   -   -   -  */
	DWORD Harea1; /*  -   -   -   -   -   -   -   -  */
	DWORD Qarea1; /*  -   -   -   -   -   -   -   -  */
	float Hfact1; /* - - - - - - - - */
	float Qfact1; /* - - - - - - - - */
	SHEETSTR splashTargs1; /* _ _ _ ground,structure,enemy,debris _ _ _ _ */
	DWORD targCount1; /* 1 1 1 1 1 1 1 1 */
	float damageLoss1; /* 0 0 0 0.2 0 0 0 0 */
	DWORD spillDist1; /* 0 0 0 0 0 0 0 0 */
	DWORD spillRadius1; /* 0 0 0 50 0 0 0 0 */
	SHEETSTR weapType2; /* _ _ _ _ _ _ _ _ */
	SHEETSTR targs2; /* _ _ ground,structure,debris,item,ward air _ _ _ _ */
	DWORD rangeN2; /* - - 100 450 - - - - */
	DWORD RngTst2; /* - - - - - - - - */
	DWORD RngBuff2; /* - - 125 250 - - - - */
	SHEETSTR atkType2; /* unknown unknown siege pierce unknown unknown unknown unknown */
	SHEETSTR weapTp2; /* _ _ missile missile _ _ _ _ */
	float cool2; /* - - 2.5 2.4 - - - - */
	DWORD mincool2; /* - - - - - - - - */
	DWORD dice2; /* - - 1 1 - - - - */
	DWORD sides2; /* - - 3 11 - - - - */
	DWORD dmgplus2; /* - - 18 44 - - - - */
	DWORD dmgUp2; /* - - - - - - - - */
	DWORD mindmg2; /*  -   -  19 45  -   -   -   -  */
	float avgdmg2; /* - - 20 50 - - - - */
	float maxdmg2; /*  -   -  21 55  -   -   -   -  */
	float dmgpt2; /* - - 0.633 0.8 - - - - */
	float backSw2; /* - - 0.7 0.87 - - - - */
	DWORD Farea2; /* - - - - - - - - */
	DWORD Harea2; /* - - -  -  - - - - */
	DWORD Qarea2; /* - - -  -  - - - - */
	float Hfact2; /* - - -  -  - - - - */
	float Qfact2; /* - - - - - - - - */
	SHEETSTR splashTargs2; /* _ _ _ - _ _ _ _ */
	DWORD targCount2; /* 1 1 1 1 1 1 1 1 */
	DWORD damageLoss2; /* 0 0 0 0 0 0 0 0 */
	DWORD spillDist2; /* 0 0 0 0 0 0 0 0 */
	DWORD spillRadius2; /* 0 0 0 0 0 0 0 0 */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

typedef struct UnitWeapons UNITWEAPONS;
typedef struct UnitWeapons *LPUNITWEAPONS;
typedef struct UnitWeapons const *LPCUNITWEAPONS;

LPCUNITWEAPONS FindUnitWeapons(DWORD unitWeapID);
void InitUnitWeapons(void);
void ShutdownUnitWeapons(void);

#endif
