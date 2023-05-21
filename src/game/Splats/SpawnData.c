#include "../g_local.h"
#include "SpawnData.h"

static LPSPAWNDATA g_SpawnData = NULL;

static struct SheetLayout SpawnData[] = {
	{ "Name", ST_STRING, FOFS(SpawnData, Name) },
	{ "Model", ST_STRING, FOFS(SpawnData, Model) },
	{ NULL },
};

LPCSPAWNDATA FindSpawnData(LPCSTR Name) {
	struct SpawnData *lpValue = g_SpawnData;
	for (; *lpValue->Name && strcmp(lpValue->Name, Name); lpValue++);
	if (*lpValue->Name == 0) lpValue = NULL;
	return lpValue;
}

void InitSpawnData(void) {
	g_SpawnData = gi.ParseSheet("Splats\\SpawnData.slk", SpawnData, sizeof(struct SpawnData));
}
void ShutdownSpawnData(void) {
	gi.MemFree(g_SpawnData);
}
