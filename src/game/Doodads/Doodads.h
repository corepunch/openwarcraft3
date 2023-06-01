#ifndef __Doodads_h__
#define __Doodads_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct Doodads {
	DWORD doodID; /* AOgs AWfs AObr AObd APct APtv AOhs APms */
	DWORD category; /* _ _ _ _ E E E E */
	SHEETSTR tilesets; /* A * C,D,G * A,L,F,W,Y,X,V,Q C A A,G */
	DWORD tilesetSpecific; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR dir; /* Doodads\Ashenvale\Props Doodads\Ashenvale\Water Doodads\Ashenvale\Props Doodads\Ashenvale\Props Doodads\Ashenvale\Plants Doodads\Felwood\Plants Doodads\Ashenvale\Props Doodads\Ashenvale\Plants */
	SHEETSTR file; /* SentinelStatue Fish Brazier Bird AshenCatTail Felwood_Thorns AshenHollowStump AshenShrooms */
	SHEETSTR comment; /* Guardian Statue of Aszune Fish Skull Brazier Birds Cattail Thorny Vines Hollow Stump Mushrooms */
	SHEETSTR name; /* WESTRING_DOOD_AOGS WESTRING_DOOD_AOFS WESTRING_DOOD_AOBR WESTRING_DOOD_AOBD WESTRING_DOOD_APCT WESTRING_DOOD_APTV WESTRING_DOOD_APHS WESTRING_DOOD_APMS */
	SHEETSTR doodClass; /* _ _ _ _ _ _ _ _ */
	SHEETSTR soundLoop; /* _ _ AshenvaleBrazierLoop _ _ _ _ _ */
	DWORD selSize; /* 0 0 0 0 0 0 0 0 */
	float defScale; /* 1 1 1 1 1 1 1 1 */
	float minScale; /* 0.8 0.8 0.8 0.8 0.8 0.8 0.8 0.8 */
	float maxScale; /* 1.2 1.2 1.2 1.2 1.2 1.2 1.2 1.2 */
	DWORD canPlaceRandScale; /* 1 1 1 1 1 1 1 1 */
	DWORD useClickHelper; /* 0 0 0 1 0 0 0 0 */
	DWORD maxPitch; /* - - - - - - - - */
	DWORD maxRoll; /* - - - - - - - - */
	DWORD visRadius; /* 50 50 50 50 50 50 50 50 */
	DWORD walkable; /* 0 0 0 0 0 0 0 0 */
	DWORD num_var; /* 1 1 1 1 1 7 1 4 */
	DWORD onCliffs; /* 1 1 1 1 0 1 1 1 */
	DWORD onWater; /* 1 1 1 1 1 1 1 1 */
	DWORD floats; /* 0 0 0 0 0 0 0 0 */
	DWORD shadow; /* 1 0 1 0 0 1 1 0 */
	DWORD showInFog; /* 1 0 1 0 1 1 1 1 */
	DWORD animInFog; /* 0 0 0 0 0 0 0 0 */
	DWORD fixedRot; /* -1 -1 -1 -1 -1 -1 -1 -1 */
	SHEETSTR pathTex; /* PathTextures\8x8Cross.tga none PathTextures\4x4Default.tga none none PathTextures\2x2Default.tga PathTextures\4x4Default.tga none */
	DWORD showInMM; /* 0 0 0 0 0 0 0 0 */
	DWORD useMMColor; /* 0 0 0 0 0 0 0 0 */
	DWORD MMRed; /* 0 0 0 0 0 0 0 0 */
	DWORD MMGreen; /* 0 0 0 0 0 0 0 0 */
	DWORD MMBlue; /* 0 0 0 0 0 0 0 0 */
	DWORD vertR01; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG01; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB01; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR02; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG02; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB02; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR03; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG03; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB03; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR04; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG04; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB04; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR05; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG05; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB05; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR06; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG06; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB06; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR07; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG07; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB07; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR08; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG08; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB08; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR09; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG09; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB09; /* 255 255 255 255 255 255 255 255 */
	DWORD vertR10; /* 255 255 255 255 255 255 255 255 */
	DWORD vertG10; /* 255 255 255 255 255 255 255 255 */
	DWORD vertB10; /* 255 255 255 255 255 255 255 255 */
	DWORD InBeta; /* 0 0 0 0 0 0 0 0 */
};

typedef struct Doodads DOODADS;
typedef struct Doodads *LPDOODADS;
typedef struct Doodads const *LPCDOODADS;

LPCDOODADS FindDoodads(DWORD doodID);
void InitDoodads(void);
void ShutdownDoodads(void);

#endif
