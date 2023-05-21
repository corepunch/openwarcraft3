#include "../g_local.h"
#include "EnvironmentSounds.h"

static struct EnvironmentSounds *g_EnvironmentSounds = NULL;

static struct SheetLayout EnvironmentSounds[] = {
	{ "EnvironmentType", ST_STRING, FOFS(EnvironmentSounds, EnvironmentType) },
	{ "RoomType", ST_STRING, FOFS(EnvironmentSounds, RoomType) },
	{ "EffectVolume", ST_INT, FOFS(EnvironmentSounds, EffectVolume) },
	{ "DecayTime", ST_FLOAT, FOFS(EnvironmentSounds, DecayTime) },
	{ "Damping", ST_INT, FOFS(EnvironmentSounds, Damping) },
	{ "Size", ST_INT, FOFS(EnvironmentSounds, Size) },
	{ "Diffusion", ST_INT, FOFS(EnvironmentSounds, Diffusion) },
	{ "Room", ST_INT, FOFS(EnvironmentSounds, Room) },
	{ "RoomHF", ST_INT, FOFS(EnvironmentSounds, RoomHF) },
	{ "DecayHFRatio", ST_INT, FOFS(EnvironmentSounds, DecayHFRatio) },
	{ "Reflections", ST_INT, FOFS(EnvironmentSounds, Reflections) },
	{ "ReflectionsDelay", ST_FLOAT, FOFS(EnvironmentSounds, ReflectionsDelay) },
	{ "Reverb", ST_INT, FOFS(EnvironmentSounds, Reverb) },
	{ "ReverbDelay", ST_FLOAT, FOFS(EnvironmentSounds, ReverbDelay) },
	{ "RoomRolloff", ST_INT, FOFS(EnvironmentSounds, RoomRolloff) },
	{ "AirAbsorption", ST_INT, FOFS(EnvironmentSounds, AirAbsorption) },
	{ NULL },
};

struct EnvironmentSounds *FindEnvironmentSounds(LPCSTR EnvironmentType) {
	struct EnvironmentSounds *lpValue = g_EnvironmentSounds;
	for (; *lpValue->EnvironmentType && strcmp(lpValue->EnvironmentType, EnvironmentType); lpValue++);
	if (*lpValue->EnvironmentType == 0) lpValue = NULL;
	return lpValue;
}

void InitEnvironmentSounds(void) {
	g_EnvironmentSounds = gi.ParseSheet("UI\\SoundInfo\\EnvironmentSounds.slk", EnvironmentSounds, sizeof(struct EnvironmentSounds));
}
void ShutdownEnvironmentSounds(void) {
	gi.MemFree(g_EnvironmentSounds);
}
