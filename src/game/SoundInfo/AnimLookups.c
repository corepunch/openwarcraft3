#include "../g_local.h"
#include "AnimLookups.h"

static LPANIMLOOKUPS g_AnimLookups = NULL;

static struct SheetLayout AnimLookups[] = {
	{ "AnimSoundEvent", ST_ID, FOFS(AnimLookups, AnimSoundEvent) },
	{ "SoundLabel", ST_STRING, FOFS(AnimLookups, SoundLabel) },
	{ NULL },
};

LPCANIMLOOKUPS FindAnimLookups(DWORD AnimSoundEvent) {
	struct AnimLookups *lpValue = g_AnimLookups;
	for (; lpValue->AnimSoundEvent != AnimSoundEvent && lpValue->AnimSoundEvent; lpValue++);
	if (lpValue->AnimSoundEvent == 0) lpValue = NULL;
	return lpValue;
}

void InitAnimLookups(void) {
	g_AnimLookups = gi.ParseSheet("UI\\SoundInfo\\AnimLookups.slk", AnimLookups, sizeof(struct AnimLookups));
}
void ShutdownAnimLookups(void) {
	gi.MemFree(g_AnimLookups);
}
