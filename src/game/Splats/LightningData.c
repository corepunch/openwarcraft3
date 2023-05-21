#include "../g_local.h"
#include "LightningData.h"

static LPLIGHTNINGDATA g_LightningData = NULL;

static struct SheetLayout LightningData[] = {
	{ "Name", ST_ID, FOFS(LightningData, Name) },
	{ "comment", ST_STRING, FOFS(LightningData, comment) },
	{ "Dir", ST_STRING, FOFS(LightningData, Dir) },
	{ "file", ST_STRING, FOFS(LightningData, file) },
	{ "AvgSegLen", ST_INT, FOFS(LightningData, AvgSegLen) },
	{ "Width", ST_INT, FOFS(LightningData, Width) },
	{ "R", ST_INT, FOFS(LightningData, R) },
	{ "G", ST_INT, FOFS(LightningData, G) },
	{ "B", ST_INT, FOFS(LightningData, B) },
	{ "A", ST_INT, FOFS(LightningData, A) },
	{ "NoiseScale", ST_FLOAT, FOFS(LightningData, NoiseScale) },
	{ "TexCoordScale", ST_FLOAT, FOFS(LightningData, TexCoordScale) },
	{ "Duration", ST_INT, FOFS(LightningData, Duration) },
	{ NULL },
};

LPCLIGHTNINGDATA FindLightningData(DWORD Name) {
	struct LightningData *lpValue = g_LightningData;
	for (; lpValue->Name != Name && lpValue->Name; lpValue++);
	if (lpValue->Name == 0) lpValue = NULL;
	return lpValue;
}

void InitLightningData(void) {
	g_LightningData = gi.ParseSheet("Splats\\LightningData.slk", LightningData, sizeof(struct LightningData));
}
void ShutdownLightningData(void) {
	gi.MemFree(g_LightningData);
}
