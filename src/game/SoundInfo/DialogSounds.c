#include "../g_local.h"
#include "DialogSounds.h"

static struct DialogSounds *g_DialogSounds = NULL;

static struct SheetLayout DialogSounds[] = {
	{ "SoundName", ST_STRING, FOFS(DialogSounds, SoundName) },
	{ "FileNames", ST_STRING, FOFS(DialogSounds, FileNames) },
	{ "DirectoryBase", ST_STRING, FOFS(DialogSounds, DirectoryBase) },
	{ "Volume", ST_INT, FOFS(DialogSounds, Volume) },
	{ "Flags", ST_STRING, FOFS(DialogSounds, Flags) },
	{ NULL },
};

struct DialogSounds *FindDialogSounds(LPCSTR SoundName) {
	struct DialogSounds *lpValue = g_DialogSounds;
	for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
	if (*lpValue->SoundName == 0) lpValue = NULL;
	return lpValue;
}

void InitDialogSounds(void) {
	g_DialogSounds = gi.ParseSheet("UI\\SoundInfo\\DialogSounds.slk", DialogSounds, sizeof(struct DialogSounds));
}
void ShutdownDialogSounds(void) {
	gi.MemFree(g_DialogSounds);
}
