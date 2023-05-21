#ifndef __DestructableData_h__
#define __DestructableData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct DestructableData {
	DWORD DestructableID; /* LTg1 LTcr LTba LOcg FTtw CTtr BTtw ATtr */
	DWORD category; /* D D D D D D D D */
	SHEETSTR tilesets; /* * * * * F,Q C B A */
	DWORD tilesetSpecific; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR dir; /* Doodads\LordaeronSummer\Terrain Doodads\LordaeronSummer\Terrain Doodads\LordaeronSummer\Terrain Doodads\LordaeronSummer\Props Doodads\Terrain Doodads\Terrain Doodads\Terrain Doodads\Terrain */
	SHEETSTR file; /* Gate Crates Barricade Cage LordaeronTree AshenTree BarrensTree AshenTree */
	DWORD lightweight; /* 0 0 0 0 1 1 1 1 */
	DWORD fatLOS; /* 0 0 0 0 0 0 0 0 */
	DWORD texID; /* - - - - 31 32 33 32 */
	SHEETSTR texFile; /* _ _ _ _ ReplaceableTextures\LordaeronTree\LordaeronFallTree ReplaceableTextures\AshenvaleTree\FelwoodTree ReplaceableTextures\BarrensTree\BarrensTree ReplaceableTextures\AshenvaleTree\AshenTree */
	SHEETSTR comment; /* Gate Crates Barricade Cage Tree Wall Tree Wall Tree Wall Tree Wall */
	SHEETSTR name; /* WESTRING_DEST_GATE_HORIZONTAL WESTRING_DOOD_LOCS WESTRING_DOOD_LOBA WESTRING_DEST_CAGE WESTRING_DEST_FALL_TREE_WALL WESTRING_DEST_TREE_WALL WESTRING_DEST_TREE_WALL WESTRING_DEST_TREE_WALL */
	DWORD doodClass; /* _ _ _ _ A A A A */
	DWORD useClickHelper; /* 0 0 0 0 0 0 0 0 */
	DWORD onCliffs; /* 1 1 1 0 0 0 0 0 */
	DWORD onWater; /* 1 1 1 1 1 1 1 1 */
	DWORD canPlaceDead; /* 0 0 0 0 1 1 1 1 */
	DWORD walkable; /* 0 0 0 0 0 0 0 0 */
	DWORD cliffHeight; /* 0 0 0 0 0 0 0 0 */
	SHEETSTR targType; /* debris debris debris debris tree tree tree tree */
	SHEETSTR armor; /* Wood Wood Wood Wood Wood Wood Wood Wood */
	DWORD numVar; /* 1 2 2 1 10 5 10 5 */
	DWORD HP; /* 500 20 50 50 50 50 50 50 */
	DWORD occH; /* 400 0 0 0 230 230 230 230 */
	DWORD flyH; /* 0 0 0 0 100 100 100 100 */
	DWORD fixedRot; /* 270 -1 -1 -1 270 270 270 270 */
	DWORD selSize; /* 0 0 0 0 0 0 0 0 */
	float minScale; /* 0.9 0.8 0.8 0.8 0.8 0.8 0.8 0.8 */
	float maxScale; /* 0.9 1.2 1.2 1.2 1.2 1.2 1.2 1.2 */
	DWORD canPlaceRandScale; /* 1 1 1 1 1 1 1 1 */
	DWORD maxPitch; /* - - - - - - - - */
	DWORD maxRoll; /* - - - - - - - - */
	DWORD radius; /* 50 0 0 0 0 0 0 0 */
	DWORD fogRadius; /* 50 0 0 0 0 0 0 0 */
	DWORD fogVis; /* 1 0 0 0 0 0 0 0 */
	SHEETSTR pathTex; /* PathTextures\Gate1Path.tga PathTextures\4x4Default.tga PathTextures\4x4Default.tga PathTextures\4x4Default.tga PathTextures\4x4Default.tga PathTextures\4x4Default.tga PathTextures\4x4Default.tga PathTextures\4x4Default.tga */
	SHEETSTR pathTexDeath; /* PathTextures\Gate1PathDeath.tga _ _ _ _ _ _ _ */
	SHEETSTR deathSnd; /* _ CrateDeath _ _ TreeWallDeath TreeWallDeath TreeWallDeath TreeWallDeath */
	SHEETSTR shadow; /* ShadowGate1 ShadowCrates ShadowCrates ShadowCage BuildingShadowSmall BuildingShadowSmall BuildingShadowSmall BuildingShadowSmall */
	DWORD showInMM; /* 1 1 1 1 1 1 1 1 */
	DWORD useMMColor; /* 0 0 0 0 1 1 1 1 */
	DWORD MMRed; /* 0 0 0 0 110 0 75 0 */
	DWORD MMGreen; /* 0 0 0 0 60 48 50 54 */
	DWORD MMBlue; /* 0 0 0 0 10 0 5 0 */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

typedef struct DestructableData DESTRUCTABLEDATA;
typedef struct DestructableData *LPDESTRUCTABLEDATA;
typedef struct DestructableData const *LPCDESTRUCTABLEDATA;

LPCDESTRUCTABLEDATA FindDestructableData(DWORD DestructableID);
void InitDestructableData(void);
void ShutdownDestructableData(void);

#endif
