#include "../g_local.h"
#include "PortraitAnims.h"

static LPPORTRAITANIMS g_PortraitAnims = NULL;

static struct SheetLayout PortraitAnims[] = {
	{ "Sound Label", ST_STRING, FOFS(PortraitAnims, Sound_Label) },
	{ "Anim Indices (MUST BE PRECEDED BY NON-NUMERIC CHAR)", ST_FLOAT, FOFS(PortraitAnims, Anim_Indices_) },
	{ "Speech Duration (MUST BE PRECEDED BY NON-NUMERIC CHAR)", ST_FLOAT, FOFS(PortraitAnims, Speech_Duration_) },
	{ NULL },
};

LPCPORTRAITANIMS FindPortraitAnims(LPCSTR Sound_Label) {
	struct PortraitAnims *lpValue = g_PortraitAnims;
	for (; *lpValue->Sound_Label && strcmp(lpValue->Sound_Label, Sound_Label); lpValue++);
	if (*lpValue->Sound_Label == 0) lpValue = NULL;
	return lpValue;
}

void InitPortraitAnims(void) {
	g_PortraitAnims = gi.ParseSheet("UI\\SoundInfo\\PortraitAnims.slk", PortraitAnims, sizeof(struct PortraitAnims));
}
void ShutdownPortraitAnims(void) {
	gi.MemFree(g_PortraitAnims);
}
