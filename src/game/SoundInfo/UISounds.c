#include "../g_local.h"
#include "UISounds.h"

static LPUISOUNDS g_UISounds = NULL;

static struct SheetLayout UISounds[] = {
	{ "SoundName", ST_STRING, FOFS(UISounds, SoundName) },
	{ "FileNames", ST_STRING, FOFS(UISounds, FileNames) },
	{ "DirectoryBase", ST_STRING, FOFS(UISounds, DirectoryBase) },
	{ "Volume", ST_INT, FOFS(UISounds, Volume) },
	{ "Pitch", ST_INT, FOFS(UISounds, Pitch) },
	{ "PitchVariance", ST_INT, FOFS(UISounds, PitchVariance) },
	{ "Priority", ST_INT, FOFS(UISounds, Priority) },
	{ "Channel", ST_INT, FOFS(UISounds, Channel) },
	{ "Flags", ST_STRING, FOFS(UISounds, Flags) },
	{ "MinDistance", ST_INT, FOFS(UISounds, MinDistance) },
	{ "MaxDistance", ST_INT, FOFS(UISounds, MaxDistance) },
	{ "DistanceCutoff", ST_INT, FOFS(UISounds, DistanceCutoff) },
	{ "InsideAngle", ST_INT, FOFS(UISounds, InsideAngle) },
	{ "OutsideAngle", ST_INT, FOFS(UISounds, OutsideAngle) },
	{ "OutsideVolume", ST_INT, FOFS(UISounds, OutsideVolume) },
	{ "OrientationX", ST_INT, FOFS(UISounds, OrientationX) },
	{ "OrientationY", ST_INT, FOFS(UISounds, OrientationY) },
	{ "OrientationZ", ST_INT, FOFS(UISounds, OrientationZ) },
	{ "EAXFlags", ST_INT, FOFS(UISounds, EAXFlags) },
	{ NULL },
};

LPCUISOUNDS FindUISounds(LPCSTR SoundName) {
	struct UISounds *lpValue = g_UISounds;
	for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
	if (*lpValue->SoundName == 0) lpValue = NULL;
	return lpValue;
}

void InitUISounds(void) {
	g_UISounds = gi.ParseSheet("UI\\SoundInfo\\UISounds.slk", UISounds, sizeof(struct UISounds));
}
void ShutdownUISounds(void) {
	gi.MemFree(g_UISounds);
}
