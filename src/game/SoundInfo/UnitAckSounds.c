#include "../g_local.h"
#include "UnitAckSounds.h"

static struct UnitAckSounds *g_UnitAckSounds = NULL;

static struct SheetLayout UnitAckSounds[] = {
	{ "SoundName", ST_STRING, FOFS(UnitAckSounds, SoundName) },
	{ "FileNames", ST_STRING, FOFS(UnitAckSounds, FileNames) },
	{ "DirectoryBase", ST_STRING, FOFS(UnitAckSounds, DirectoryBase) },
	{ "Volume", ST_INT, FOFS(UnitAckSounds, Volume) },
	{ "Pitch", ST_FLOAT, FOFS(UnitAckSounds, Pitch) },
	{ "PitchVariance", ST_FLOAT, FOFS(UnitAckSounds, PitchVariance) },
	{ "Priority", ST_INT, FOFS(UnitAckSounds, Priority) },
	{ "Channel", ST_INT, FOFS(UnitAckSounds, Channel) },
	{ "Flags", ST_STRING, FOFS(UnitAckSounds, Flags) },
	{ "MinDistance", ST_INT, FOFS(UnitAckSounds, MinDistance) },
	{ "MaxDistance", ST_INT, FOFS(UnitAckSounds, MaxDistance) },
	{ "DistanceCutoff", ST_INT, FOFS(UnitAckSounds, DistanceCutoff) },
	{ "InsideAngle", ST_INT, FOFS(UnitAckSounds, InsideAngle) },
	{ "OutsideAngle", ST_INT, FOFS(UnitAckSounds, OutsideAngle) },
	{ "OutsideVolume", ST_INT, FOFS(UnitAckSounds, OutsideVolume) },
	{ "OrientationX", ST_INT, FOFS(UnitAckSounds, OrientationX) },
	{ "OrientationY", ST_INT, FOFS(UnitAckSounds, OrientationY) },
	{ "OrientationZ", ST_INT, FOFS(UnitAckSounds, OrientationZ) },
	{ "EAXFlags", ST_STRING, FOFS(UnitAckSounds, EAXFlags) },
	{ NULL },
};

struct UnitAckSounds *FindUnitAckSounds(LPCSTR SoundName) {
	struct UnitAckSounds *lpValue = g_UnitAckSounds;
	for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
	if (*lpValue->SoundName == 0) lpValue = NULL;
	return lpValue;
}

void InitUnitAckSounds(void) {
	g_UnitAckSounds = gi.ParseSheet("UI\\SoundInfo\\UnitAckSounds.slk", UnitAckSounds, sizeof(struct UnitAckSounds));
}
void ShutdownUnitAckSounds(void) {
	gi.MemFree(g_UnitAckSounds);
}
