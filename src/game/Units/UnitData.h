#ifndef __UnitData_h__
#define __UnitData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitData {
	DWORD unitID; /* hmil hkni hgyr hgry hfoo Hpal Hmkg Hamg */
	DWORD sort; /* a2 a2 a2 a2 a2 a1 a1 a1 */
	SHEETSTR comment; /* Militia Knight Gyrocopter GryphonRider Footman HeroPaladin HeroMountainKing HeroArchMage */
	SHEETSTR race; /* human human human human human human human human */
	DWORD prio; /* 1 3 2 2 6 9 9 9 */
	DWORD threat; /* 1 1 1 1 1 1 1 1 */
	SHEETSTR type; /* _ _ Mechanical _ _ _ _ _ */
	DWORD valid; /* 1 1 1 1 1 1 1 1 */
	DWORD deathType; /* 3 3 0 0 3 2 2 2 */
	float death; /* 3.34 5.1 2.67 1.67 3.04 1.5 2.5 2.8 */
	DWORD canSleep; /* 0 0 0 0 0 0 0 0 */
	DWORD cargoSize; /* 1 1 - - 1 1 1 1 */
	SHEETSTR movetp; /* foot horse fly fly foot foot foot foot */
	DWORD moveHeight; /* 0 0 280 240 0 0 0 0 */
	DWORD moveFloor; /* 0 0 90 90 0 0 0 0 */
	DWORD launchX; /* 0 0 0 12 0 0 0 15 */
	DWORD launchY; /* 0 0 0 -17 0 0 0 0 */
	DWORD launchZ; /* 60 60 60 33 60 60 60 66 */
	DWORD impactZ; /* 60 60 60 20 60 60 60 60 */
	float turnRate; /* 0.6 0.5 0.5 0.4 0.6 0.6 0.6 0.5 */
	DWORD propWin; /* 60 60 61 61 60 60 60 60 */
	DWORD orientInterp; /* 0 4 2 1 0 5 5 4 */
	DWORD formation; /* 0 0 2 2 0 0 0 2 */
	float castpt; /* 0.3 0.3 0.3 0.3 0.3 0.5 0.4 0.3 */
	float castbsw; /* 0.51 0.51 0.51 0.5 0.51 1.67 0.5 2.4 */
	SHEETSTR targType; /* ground ground air air ground ground ground ground */
	SHEETSTR pathTex; /* _ _ _ _ _ _ _ _ */
	DWORD fatLOS; /* 0 0 0 0 0 0 0 0 */
	DWORD collision; /* 16 32 48 32 31 32 32 32 */
	DWORD points; /* 100 100 100 100 100 100 100 100 */
	SHEETSTR buffType; /* _ _ _ _ _ _ _ _ */
	DWORD buffRadius; /* - - - - - - - - */
	DWORD nameCount; /* - - - - - 15 13 13 */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

typedef struct UnitData UNITDATA;
typedef struct UnitData *LPUNITDATA;
typedef struct UnitData const *LPCUNITDATA;

LPCUNITDATA FindUnitData(DWORD unitID);
void InitUnitData(void);
void ShutdownUnitData(void);

#endif
