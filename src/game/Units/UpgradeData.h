#ifndef __UpgradeData_h__
#define __UpgradeData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UpgradeData {
	DWORD upgradeid; /* Rhan Rhde Rhac Rhgb Rhar Rhhb Rhra Rhme */
	SHEETSTR comments; /* human animal breeding human footman defend human architecture human gyro bombs human armor human hammer bounce human ranged attack human melee attack */
	SHEETSTR class; /* _ _ armor _ armor _ ranged melee */
	SHEETSTR race; /* human human human human human human human human */
	DWORD flag; /* 128 64 32 16 8 4 2 1 */
	DWORD used; /* 1 1 1 1 1 1 1 1 */
	DWORD maxlevel; /* 1 1 3 1 3 1 3 3 */
	DWORD inherit; /* 1 1 0 1 0 1 0 0 */
	DWORD goldbase; /* 250 150 100 150 125 200 100 100 */
	DWORD goldmod; /* 0 0 50 50 75 0 150 150 */
	DWORD lumberbase; /* 200 100 100 100 75 150 50 50 */
	DWORD lumbermod; /* 0 0 50 0 50 0 50 50 */
	DWORD timebase; /* 60 45 60 60 60 60 60 60 */
	DWORD timemod; /* 0 0 15 0 15 0 15 15 */
	DWORD effect1; /* rhpx _ rarm renw rarm rasd ratd ratd */
	float base1; /* 150 - - 3 - 200 1 1 */
	float mod1; /* - - - - - - 1 1 */
	DWORD effect2; /* _ _ rhpo _ _ _ _ _ */
	float base2; /* - - 0.2 - - - - - */
	float mod2; /* - - 0.2 - - - - - */
	DWORD effect3; /* _ _ _ _ _ _ _ _ */
	DWORD base3; /* - - - - - - - - */
	DWORD mod3; /* - - - - - - - - */
	DWORD effect4; /* _ _ _ _ _ _ _ _ */
	DWORD base4; /* - - - - - - - - */
	DWORD mod4; /* - - - - - - - - */
};

struct UpgradeData *FindUpgradeData(DWORD upgradeid);
void InitUpgradeData(void);
void ShutdownUpgradeData(void);

#endif
