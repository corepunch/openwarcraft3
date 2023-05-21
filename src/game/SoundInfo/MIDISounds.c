#include "../g_local.h"
#include "MIDISounds.h"

static struct MIDISounds *g_MIDISounds = NULL;

static struct SheetLayout MIDISounds[] = {
	{ "SoundLabel", ST_STRING, FOFS(MIDISounds, SoundLabel) },
	{ "DirectoryBase", ST_STRING, FOFS(MIDISounds, DirectoryBase) },
	{ "MIDIFileName", ST_STRING, FOFS(MIDISounds, MIDIFileName) },
	{ "DLSFileName", ST_STRING, FOFS(MIDISounds, DLSFileName) },
	{ "Volume", ST_INT, FOFS(MIDISounds, Volume) },
	{ "Priority ", ST_INT, FOFS(MIDISounds, Priority_) },
	{ "Pitch", ST_INT, FOFS(MIDISounds, Pitch) },
	{ "Channel", ST_INT, FOFS(MIDISounds, Channel) },
	{ "Radius", ST_INT, FOFS(MIDISounds, Radius) },
	{ "Flags", ST_STRING, FOFS(MIDISounds, Flags) },
	{ NULL },
};

struct MIDISounds *FindMIDISounds(LPCSTR SoundLabel) {
	struct MIDISounds *lpValue = g_MIDISounds;
	for (; *lpValue->SoundLabel && strcmp(lpValue->SoundLabel, SoundLabel); lpValue++);
	if (*lpValue->SoundLabel == 0) lpValue = NULL;
	return lpValue;
}

void InitMIDISounds(void) {
	g_MIDISounds = gi.ParseSheet("UI\\SoundInfo\\MIDISounds.slk", MIDISounds, sizeof(struct MIDISounds));
}
void ShutdownMIDISounds(void) {
	gi.MemFree(g_MIDISounds);
}
