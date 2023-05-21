#include "../g_local.h"
#include "AmbienceSounds.h"

static struct AmbienceSounds *g_AmbienceSounds = NULL;

static struct SheetLayout AmbienceSounds[] = {
	{ "SoundName", ST_STRING, FOFS(AmbienceSounds, SoundName) },
	{ "FileNames", ST_STRING, FOFS(AmbienceSounds, FileNames) },
	{ "DirectoryBase", ST_STRING, FOFS(AmbienceSounds, DirectoryBase) },
	{ "Volume", ST_INT, FOFS(AmbienceSounds, Volume) },
	{ "Pitch", ST_INT, FOFS(AmbienceSounds, Pitch) },
	{ "PitchVariance", ST_INT, FOFS(AmbienceSounds, PitchVariance) },
	{ "Priority", ST_INT, FOFS(AmbienceSounds, Priority) },
	{ "Channel", ST_INT, FOFS(AmbienceSounds, Channel) },
	{ "Flags", ST_STRING, FOFS(AmbienceSounds, Flags) },
	{ "MinDistance", ST_INT, FOFS(AmbienceSounds, MinDistance) },
	{ "MaxDistance", ST_INT, FOFS(AmbienceSounds, MaxDistance) },
	{ "DistanceCutoff", ST_INT, FOFS(AmbienceSounds, DistanceCutoff) },
	{ "insideAngle", ST_INT, FOFS(AmbienceSounds, insideAngle) },
	{ "outsideAngle", ST_INT, FOFS(AmbienceSounds, outsideAngle) },
	{ "outsideConeVolume", ST_INT, FOFS(AmbienceSounds, outsideConeVolume) },
	{ "coneOrientationX", ST_INT, FOFS(AmbienceSounds, coneOrientationX) },
	{ "coneOrientationY", ST_INT, FOFS(AmbienceSounds, coneOrientationY) },
	{ "ConeOrientationZ", ST_INT, FOFS(AmbienceSounds, ConeOrientationZ) },
	{ "EAXFlags", ST_STRING, FOFS(AmbienceSounds, EAXFlags) },
	{ NULL },
};

struct AmbienceSounds *FindAmbienceSounds(LPCSTR SoundName) {
	struct AmbienceSounds *lpValue = g_AmbienceSounds;
	for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
	if (*lpValue->SoundName == 0) lpValue = NULL;
	return lpValue;
}

void InitAmbienceSounds(void) {
	g_AmbienceSounds = gi.ParseSheet("UI\\SoundInfo\\AmbienceSounds.slk", AmbienceSounds, sizeof(struct AmbienceSounds));
}
void ShutdownAmbienceSounds(void) {
	gi.MemFree(g_AmbienceSounds);
}
