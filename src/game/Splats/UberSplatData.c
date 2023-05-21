#include "../g_local.h"
#include "UberSplatData.h"

static struct UberSplatData *g_UberSplatData = NULL;

static struct SheetLayout UberSplatData[] = {
	{ "Name", ST_ID, FOFS(UberSplatData, Name) },
	{ "comment", ST_STRING, FOFS(UberSplatData, comment) },
	{ "Dir", ST_STRING, FOFS(UberSplatData, Dir) },
	{ "file", ST_STRING, FOFS(UberSplatData, file) },
	{ "BlendMode", ST_INT, FOFS(UberSplatData, BlendMode) },
	{ "Scale", ST_INT, FOFS(UberSplatData, Scale) },
	{ "BirthTime", ST_FLOAT, FOFS(UberSplatData, BirthTime) },
	{ "PauseTime", ST_INT, FOFS(UberSplatData, PauseTime) },
	{ "Decay", ST_INT, FOFS(UberSplatData, Decay) },
	{ "StartR", ST_INT, FOFS(UberSplatData, StartR) },
	{ "StartG", ST_INT, FOFS(UberSplatData, StartG) },
	{ "StartB", ST_INT, FOFS(UberSplatData, StartB) },
	{ "StartA", ST_INT, FOFS(UberSplatData, StartA) },
	{ "MiddleR", ST_INT, FOFS(UberSplatData, MiddleR) },
	{ "MiddleG", ST_INT, FOFS(UberSplatData, MiddleG) },
	{ "MiddleB", ST_INT, FOFS(UberSplatData, MiddleB) },
	{ "MiddleA", ST_INT, FOFS(UberSplatData, MiddleA) },
	{ "EndR", ST_INT, FOFS(UberSplatData, EndR) },
	{ "EndG", ST_INT, FOFS(UberSplatData, EndG) },
	{ "EndB", ST_INT, FOFS(UberSplatData, EndB) },
	{ "EndA", ST_INT, FOFS(UberSplatData, EndA) },
	{ "Sound", ST_ID, FOFS(UberSplatData, Sound) },
	{ NULL },
};

struct UberSplatData *FindUberSplatData(DWORD Name) {
	struct UberSplatData *lpValue = g_UberSplatData;
	for (; lpValue->Name != Name && lpValue->Name; lpValue++);
	if (lpValue->Name == 0) lpValue = NULL;
	return lpValue;
}

void InitUberSplatData(void) {
	g_UberSplatData = gi.ParseSheet("Splats\\UberSplatData.slk", UberSplatData, sizeof(struct UberSplatData));
}
void ShutdownUberSplatData(void) {
	gi.MemFree(g_UberSplatData);
}
