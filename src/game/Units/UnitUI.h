#ifndef __UnitUI_h__
#define __UnitUI_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitUI {
	DWORD unitUIID; /* echm ebal eate earc eaow eaom eaoe egol */
	SHEETSTR file; /* units\nightelf\Chimaera\Chimaera units\nightelf\Ballista\Ballista buildings\nightelf\AltarOfElders\AltarOfElders units\nightelf\Archer\Archer buildings\nightelf\AncientOfWind\AncientOfWind buildings\nightelf\AncientOfWar\AncientOfWar buildings\nightelf\AncientOfLore\AncientOfLore buildings\nightelf\EntangledGoldMine\EntangledGoldMine */
	SHEETSTR unitSound; /* Chimaera Ballista AltarOfElders Archer AncientOfWind AncientOfWar AncientOfLore EntangledGoldMine */
	SHEETSTR tilesets; /* * * * * * * * * */
	DWORD tilesetSpecific; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR name; /* chimaera ballista altarofelders archer ancientofwind ancientofwar ancientoflore entangledgoldmine */
	SHEETSTR unitClass; /* EUnit08 EUnit05 EBuilding05 EUnit02 EBuilding08 EBuilding06 EBuilding07 EBuilding12 */
	DWORD special; /* 0 0 0 0 0 0 0 0 */
	DWORD inEditor; /* 1 1 1 1 1 1 1 0 */
	DWORD hiddenInEditor; /* 0 0 0 0 0 0 0 0 */
	DWORD hostilePal; /* - - - - - - - - */
	DWORD dropItems; /* 1 1 1 1 1 1 1 1 */
	DWORD nbrandom; /* - - - - - - - - */
	DWORD nbmmIcon; /* - - - - - - - - */
	DWORD useClickHelper; /* 0 0 0 0 0 0 0 0 */
	float blend; /* 0.4 0.15 0.15 0.15 0.4 0.4 0.4 0.15 */
	float scale; /* 2 3 4 1 4 4 4 5.5 */
	DWORD scaleBull; /* 1 1 1 1 1 1 1 1 */
	SHEETSTR preventPlace; /* _ _ unbuildable _ unbuildable unbuildable unbuildable unbuildable */
	SHEETSTR requirePlace; /* _ _ _ _ _ _ _ _ */
	DWORD isbldg; /* 0 0 1 0 1 1 1 1 */
	DWORD maxPitch; /* 33 45 7 10 0 0 0 0 */
	DWORD maxRoll; /* 0 45 7 10 0 0 0 0 */
	DWORD elevPts; /* 2 4 4 - 4 4 4 - */
	DWORD elevRad; /* 100 50 50 20 50 50 50 50 */
	DWORD fogRad; /* 0 0 0 0 0 0 0 0 */
	DWORD walk; /* 200 150 200 290 100 100 100 100 */
	DWORD run; /* 200 150 200 290 100 100 100 200 */
	DWORD selZ; /* 230 0 0 0 0 0 0 0 */
	SHEETSTR weap1; /* _ _ _ _ WoodHeavyBash WoodHeavyBash WoodHeavyBash _ */
	SHEETSTR weap2; /* _ _ _ _ _ _ _ _ */
	DWORD teamColor; /* -1 -1 -1 -1 -1 -1 -1 -1 */
	DWORD customTeamColor; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR armor; /* Flesh Wood Stone Flesh Stone Stone Stone Wood */
	float modelScale; /* 1 1 1 1 1 1 1 1 */
	DWORD red; /* 255 255 255 255 255 255 255 255 */
	DWORD green; /* 255 255 255 255 255 255 255 255 */
	DWORD blue; /* 255 255 255 255 255 255 255 255 */
	DWORD uberSplat; /* _ _ EMDB _ EMDA EMDA EMDA EMDB */
	SHEETSTR unitShadow; /* ShadowFlyer Shadow _ Shadow Shadow Shadow Shadow _ */
	SHEETSTR buildingShadow; /* _ _ ShadowAltarofElders _ ShadowAncientofWind ShadowAncientofWar ShadowAncientofLore BuildingShadowLarge */
	DWORD shadowW; /* 240 160 120 400 500 400 */
	DWORD shadowH; /* 240 160 120 400 500 400 */
	DWORD shadowX; /* 120 80 50 150 200 150 */
	DWORD shadowY; /* 120 80 50 150 200 150 */
	DWORD occH; /* 0 0 0 0 0 0 0 0 */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

typedef struct UnitUI UNITUI;
typedef struct UnitUI *LPUNITUI;
typedef struct UnitUI const *LPCUNITUI;

LPCUNITUI FindUnitUI(DWORD unitUIID);
void InitUnitUI(void);
void ShutdownUnitUI(void);

#endif
