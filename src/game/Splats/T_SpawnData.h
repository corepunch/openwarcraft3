#ifndef __T_SpawnData_h__
#define __T_SpawnData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct T_SpawnData {
	SHEETSTR Name; /* ODIS DDIS EDIS UDIS GCBL UEGG TEST INIT */
	SHEETSTR Model; /* Objects\Spawnmodels\Orc\OrcDissipate\OrcDissipate.mdl Objects\Spawnmodels\Demon\DemonDissipate\DemonDissipate.mdl Objects\Spawnmodels\NightElf\NightelfDissipate\NightElfDissipate.mdl Objects\Spawnmodels\Undead\UndeadDissipate\UndeadDissipate.mdl Objects\Spawnmodels\Undead\GargoyleCrumble\GargoyleCrumble.mdl Objects\Spawnmodels\Undead\CryptFiendEggsack\CryptFiendEggsack.mdl bugger.mdl INIT */
};

typedef struct T_SpawnData T_SPAWNDATA;
typedef struct T_SpawnData *LPT_SPAWNDATA;
typedef struct T_SpawnData const *LPCT_SPAWNDATA;

LPCT_SPAWNDATA FindT_SpawnData(LPCSTR Name);
void InitT_SpawnData(void);
void ShutdownT_SpawnData(void);

#endif
