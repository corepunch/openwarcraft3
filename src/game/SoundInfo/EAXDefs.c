#include "../g_local.h"
#include "EAXDefs.h"

static LPEAXDEFS g_EAXDefs = NULL;

static struct SheetLayout EAXDefs[] = {
	{ "EAXLabel", ST_STRING, FOFS(EAXDefs, EAXLabel) },
	{ "DisplayText", ST_STRING, FOFS(EAXDefs, DisplayText) },
	{ "EffectsLevel", ST_INT, FOFS(EAXDefs, EffectsLevel) },
	{ "EAX2Direct", ST_INT, FOFS(EAXDefs, EAX2Direct) },
	{ "EAX2DirectHF", ST_INT, FOFS(EAXDefs, EAX2DirectHF) },
	{ "EAX2Room", ST_INT, FOFS(EAXDefs, EAX2Room) },
	{ "EAX2RoomHF", ST_INT, FOFS(EAXDefs, EAX2RoomHF) },
	{ "EAX2Obstruction", ST_INT, FOFS(EAXDefs, EAX2Obstruction) },
	{ "EAX2ObstructionLFRatio", ST_INT, FOFS(EAXDefs, EAX2ObstructionLFRatio) },
	{ "EAX2Occlusion", ST_INT, FOFS(EAXDefs, EAX2Occlusion) },
	{ "EAX2OcclusionLFRatio", ST_INT, FOFS(EAXDefs, EAX2OcclusionLFRatio) },
	{ "EAX2OcclusionRoomRatio", ST_INT, FOFS(EAXDefs, EAX2OcclusionRoomRatio) },
	{ "EAX2RoomRolloff", ST_INT, FOFS(EAXDefs, EAX2RoomRolloff) },
	{ "EAX2AirAbsorption", ST_INT, FOFS(EAXDefs, EAX2AirAbsorption) },
	{ "EAX2OutsideVolumeHF", ST_INT, FOFS(EAXDefs, EAX2OutsideVolumeHF) },
	{ "EAX2AutoRoomLevels", ST_INT, FOFS(EAXDefs, EAX2AutoRoomLevels) },
	{ "Fast2DPredelayTime", ST_INT, FOFS(EAXDefs, Fast2DPredelayTime) },
	{ "Fast2DDamping", ST_INT, FOFS(EAXDefs, Fast2DDamping) },
	{ "Fast2DReverbTime", ST_INT, FOFS(EAXDefs, Fast2DReverbTime) },
	{ "Fast2DReverbQuality", ST_INT, FOFS(EAXDefs, Fast2DReverbQuality) },
	{ "Fast2DOcclusionScale", ST_FLOAT, FOFS(EAXDefs, Fast2DOcclusionScale) },
	{ NULL },
};

LPCEAXDEFS FindEAXDefs(LPCSTR EAXLabel) {
	struct EAXDefs *lpValue = g_EAXDefs;
	for (; *lpValue->EAXLabel && strcmp(lpValue->EAXLabel, EAXLabel); lpValue++);
	if (*lpValue->EAXLabel == 0) lpValue = NULL;
	return lpValue;
}

void InitEAXDefs(void) {
	g_EAXDefs = gi.ParseSheet("UI\\SoundInfo\\EAXDefs.slk", EAXDefs, sizeof(struct EAXDefs));
}
void ShutdownEAXDefs(void) {
	gi.MemFree(g_EAXDefs);
}
