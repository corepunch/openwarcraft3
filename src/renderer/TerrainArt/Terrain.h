#ifndef __Terrain_h__
#define __Terrain_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct Terrain {
	DWORD tileID; /* Fdro Fdrt Lgrd Lgrs Lrok Ldrg Ldro Ldrt */
	SHEETSTR dir; /* TerrainArt\LordaeronFall TerrainArt\LordaeronFall TerrainArt\LordaeronSummer TerrainArt\LordaeronSummer TerrainArt\LordaeronSummer TerrainArt\LordaeronSummer TerrainArt\LordaeronSummer TerrainArt\LordaeronSummer */
	SHEETSTR file; /* Lordf_DirtRough Lordf_Dirt Lords_GrassDark Lords_Grass Lords_Rock Lords_DirtGrass Lords_DirtRough Lords_Dirt */
	SHEETSTR comment; /* Rough Dirt Dirt Dark Grass Grass Rock Grassy Dirt Rough Dirt Dirt */
	SHEETSTR name; /* WESTRING_TILE_ROUGH_DIRT WESTRING_TILE_DIRT WESTRING_TILE_DARK_GRASS WESTRING_TILE_GRASS WESTRING_TILE_ROCK WESTRING_TILE_GRASSY_DIRT WESTRING_TILE_ROUGH_DIRT WESTRING_TILE_DIRT */
	DWORD buildable; /* 1 1 1 1 0 1 1 1 */
	DWORD footprints; /* 1 1 1 1 0 1 1 1 */
	DWORD walkable; /* 1 1 1 1 1 1 1 1 */
	DWORD flyable; /* 1 1 1 1 1 1 1 1 */
	DWORD blightPri; /* 1 2 5 4 6 3 1 2 */
	SHEETSTR convertTo; /* Ldro,Wdro,Bdsd,Adrd,Cdrd,Ndrd,Ydtr,Vdrr,Qdrr,Xdtr,Dbrk,Gbrk Ldrt,Wdrt,Bdsr,Adrt,Cdrt,Ndrt,Ydrt,Vdrt,Qdrt,Xdrt,Ddrt,Gdrt Fgrd,Wsnw,Bdrg,Agrd,Clvg,Nice,Ygsb,Vgrt,Qgrt,Xgsb,Dlav,Glav Fgrs,Wgrs,Bgrr,Agrs,Cgrs,Nsnw,Ygsb,Vgrs,Qgrs,Xhdg,Dsqd,Gsqd Frok,Wrok,Bflr,Arck,Crck,Nrck,Ybtl,Vrck,Qrck,Xbtl,Ddkr.Gdkr Fdrg,Wsng,Bdrr,Cpos,Ngrs,Ygsb,Vgrs,Qgrs,Xgsb,Dgrs,Ggrs Fdro,Wdro,Bdsd,Adrd,Cdrd,Ndrd,Ydtr,Vdrr,Qdrr,Xdtr,Dbrk,Gbrk Fdrt,Wdrt,Bdsr,Adrt,Cdrt,Ndrt,Ydrt,Vdrt,Qdrt,Xdrt,Ddrt,Gdrt */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

typedef struct Terrain TERRAIN;
typedef struct Terrain *LPTERRAIN;
typedef struct Terrain const *LPCTERRAIN;

LPCTERRAIN FindTerrain(DWORD tileID);
void InitTerrain(void);
void ShutdownTerrain(void);

#endif
