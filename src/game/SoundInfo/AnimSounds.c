#include "../g_local.h"
#include "AnimSounds.h"

static LPANIMSOUNDS g_AnimSounds = NULL;

static struct SheetLayout AnimSounds[] = {
	{ "SoundName", ST_STRING, FOFS(AnimSounds, SoundName) },
	{ "FileNames", ST_STRING, FOFS(AnimSounds, FileNames) },
	{ "DirectoryBase", ST_STRING, FOFS(AnimSounds, DirectoryBase) },
	{ "Volume", ST_INT, FOFS(AnimSounds, Volume) },
	{ "Pitch", ST_INT, FOFS(AnimSounds, Pitch) },
	{ "PitchVariance", ST_FLOAT, FOFS(AnimSounds, PitchVariance) },
	{ "Priority", ST_INT, FOFS(AnimSounds, Priority) },
	{ "Channel", ST_INT, FOFS(AnimSounds, Channel) },
	{ "Flags", ST_STRING, FOFS(AnimSounds, Flags) },
	{ "MinDistance", ST_INT, FOFS(AnimSounds, MinDistance) },
	{ "MaxDistance", ST_INT, FOFS(AnimSounds, MaxDistance) },
	{ "DistanceCutoff", ST_INT, FOFS(AnimSounds, DistanceCutoff) },
	{ "InsideAngle", ST_INT, FOFS(AnimSounds, InsideAngle) },
	{ "OutsideAngle", ST_INT, FOFS(AnimSounds, OutsideAngle) },
	{ "OutsideVolume", ST_INT, FOFS(AnimSounds, OutsideVolume) },
	{ "OrientationX", ST_INT, FOFS(AnimSounds, OrientationX) },
	{ "OrientationY", ST_INT, FOFS(AnimSounds, OrientationY) },
	{ "OrientationZ", ST_INT, FOFS(AnimSounds, OrientationZ) },
	{ "EAXFlags", ST_STRING, FOFS(AnimSounds, EAXFlags) },
	{ NULL },
};

LPCANIMSOUNDS FindAnimSounds(LPCSTR SoundName) {
	struct AnimSounds *lpValue = g_AnimSounds;
	for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
	if (*lpValue->SoundName == 0) lpValue = NULL;
	return lpValue;
}

void InitAnimSounds(void) {
	g_AnimSounds = gi.ParseSheet("UI\\SoundInfo\\AnimSounds.slk", AnimSounds, sizeof(struct AnimSounds));
}
void ShutdownAnimSounds(void) {
	gi.MemFree(g_AnimSounds);
}
