#ifndef __UberSplatData_h__
#define __UberSplatData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UberSplatData {
  DWORD Name; /* DNCS UDSU HCRT LSDL LSDM LSDS TEST INIT */
  SHEETSTR comment; /* DeathNeutralCityBuildingSmall UndeadUberSplat HumanCrater LordSummerPlainDirtLarge LordSummerPlainDirtMed LordSummerPlainDirtSmall TestSplat INIT */
  SHEETSTR Dir; /* ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats ReplaceableTextures\Splats */
  SHEETSTR file; /* ScorchedUberSplat DarkSummonSpecial CraterUberSplat DirtUberSplat DirtUberSplat DirtUberSplat TestUberSplat TEST */
  DWORD BlendMode; /* 0 0 0 0 0 0 0 0 */
  DWORD Scale; /* 200 200 75 240 200 110 100 1 */
  float BirthTime; /* 0.2 1 0.2 1 1 1 1 1 */
  DWORD PauseTime; /* 300 0 7 0 0 0 5 50 */
  DWORD Decay; /* 600 5 6 2 2 2 2 10 */
  DWORD StartR; /* 255 255 255 255 255 255 255 255 */
  DWORD StartG; /* 255 255 255 255 255 255 255 255 */
  DWORD StartB; /* 255 255 255 255 255 255 255 255 */
  DWORD StartA; /* 0 0 0 0 0 0 0 0 */
  DWORD MiddleR; /* 255 255 255 255 255 255 255 255 */
  DWORD MiddleG; /* 255 255 255 255 255 255 255 255 */
  DWORD MiddleB; /* 255 255 255 255 255 255 255 255 */
  DWORD MiddleA; /* 255 255 255 255 255 255 255 255 */
  DWORD EndR; /* 255 255 255 255 255 255 255 255 */
  DWORD EndG; /* 255 255 255 255 255 255 255 255 */
  DWORD EndB; /* 255 255 255 255 255 255 255 255 */
  DWORD EndA; /* 0 0 0 0 0 0 0 0 */
  DWORD Sound; /* NULL NULL NULL NULL NULL NULL NULL INIT */
};

struct UberSplatData *FindUberSplatData(DWORD Name);
void InitUberSplatData(void);
void ShutdownUberSplatData(void);

#endif
