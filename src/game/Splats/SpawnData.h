#ifndef __SpawnData_h__
#define __SpawnData_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct SpawnData {
	SHEETSTR Name; /* ODIS DDIS EDIS UDIS GCBL UEGG TEST INIT */
	SHEETSTR Model; /* Objects\Spawnmodels\Orc\OrcDissipate\OrcDissipate.mdl Objects\Spawnmodels\Demon\DemonDissipate\DemonDissipate.mdl Objects\Spawnmodels\NightElf\NightelfDissipate\NightElfDissipate.mdl Objects\Spawnmodels\Undead\UndeadDissipate\UndeadDissipate.mdl Objects\Spawnmodels\Undead\GargoyleCrumble\GargoyleCrumble.mdl Objects\Spawnmodels\Undead\CryptFiendEggsack\CryptFiendEggsack.mdl bugger.mdl INIT */
};

typedef struct SpawnData SPAWNDATA;
typedef struct SpawnData *LPSPAWNDATA;
typedef struct SpawnData const *LPCSPAWNDATA;

LPCSPAWNDATA FindSpawnData(LPCSTR Name);
void InitSpawnData(void);
void ShutdownSpawnData(void);

#endif
