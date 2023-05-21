#include "../g_local.h"
#include "T_SpawnData.h"

static struct T_SpawnData *g_T_SpawnData = NULL;

static struct SheetLayout T_SpawnData[] = {
	{ "Name", ST_STRING, FOFS(T_SpawnData, Name) },
	{ "Model", ST_STRING, FOFS(T_SpawnData, Model) },
	{ NULL },
};

struct T_SpawnData *FindT_SpawnData(LPCSTR Name) {
	struct T_SpawnData *lpValue = g_T_SpawnData;
	for (; *lpValue->Name && strcmp(lpValue->Name, Name); lpValue++);
	if (*lpValue->Name == 0) lpValue = NULL;
	return lpValue;
}

void InitT_SpawnData(void) {
	g_T_SpawnData = gi.ParseSheet("Splats\\T_SpawnData.slk", T_SpawnData, sizeof(struct T_SpawnData));
}
void ShutdownT_SpawnData(void) {
	gi.MemFree(g_T_SpawnData);
}
