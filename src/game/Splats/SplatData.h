#ifndef __SplatData_h__
#define __SplatData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct SplatData {
	DWORD Name; /* DBS2 DBS1 DBS0 DBL3 DBL2 DBL1 DBL0 INIT */
	SHEETSTR comment; /* DemonBloodSmall2 DemonBloodSmall1 DemonBloodSmall0 DemonBloodLarge3 DemonBloodLarge2 DemonBloodLarge1 DemonBloodLarge0 INIT */
	SHEETSTR Dir; /* ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats */
	SHEETSTR file; /* Splat01Mature Splat01Mature Splat01Mature Splat01Mature Splat01Mature Splat01Mature Splat01Mature Splat01Mature */
	DWORD Rows; /* 16 16 16 16 16 16 16 16 */
	DWORD Columns; /* 16 16 16 16 16 16 16 16 */
	DWORD BlendMode; /* 1 1 1 1 1 1 1 0 */
	DWORD Scale; /* 25 25 25 50 50 50 50 25 */
	float Lifespan; /* 2 2 2 2 2 2 2 2 */
	float Decay; /* 120 120 120 120 120 120 120 120 */
	DWORD UVLifespanStart; /* 32 16 0 48 32 16 0 0 */
	DWORD UVLifespanEnd; /* 47 31 15 63 47 31 15 3 */
	DWORD LifespanRepeat; /* 1 1 1 1 1 1 1 1 */
	DWORD UVDecayStart; /* 47 31 15 63 47 31 15 3 */
	DWORD UVDecayEnd; /* 47 31 15 63 47 31 15 3 */
	DWORD DecayRepeat; /* 1 1 1 1 1 1 1 1 */
	DWORD StartR; /* 60 60 60 60 60 60 60 255 */
	DWORD StartG; /* 120 120 120 120 120 120 120 255 */
	DWORD StartB; /* 20 20 20 20 20 20 20 255 */
	DWORD StartA; /* 255 255 255 255 255 255 255 255 */
	DWORD MiddleR; /* 60 60 60 60 60 60 60 255 */
	DWORD MiddleG; /* 120 120 120 120 120 120 120 255 */
	DWORD MiddleB; /* 20 20 20 20 20 20 20 255 */
	DWORD MiddleA; /* 200 200 200 200 200 200 200 255 */
	DWORD EndR; /* 60 60 60 60 60 60 60 100 */
	DWORD EndG; /* 120 120 120 120 120 120 120 100 */
	DWORD EndB; /* 20 20 20 20 20 20 20 100 */
	DWORD EndA; /* 0 0 0 0 0 0 0 0 */
	DWORD Water; /* WDS1 WDS0 WDS0 WDL1 WDL1 WDL0 WDL0 INIT */
	DWORD Sound; /* NULL NULL NULL NULL NULL NULL NULL INIT */
};

struct SplatData *FindSplatData(DWORD Name);
void InitSplatData(void);
void ShutdownSplatData(void);

#endif
