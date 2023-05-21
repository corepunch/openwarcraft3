#include "../g_local.h"
#include "AbilitySounds.h"

static struct AbilitySounds *g_AbilitySounds = NULL;

static struct SheetLayout AbilitySounds[] = {
  { "SoundName", ST_STRING, FOFS(AbilitySounds, SoundName) },
  { "FileNames", ST_STRING, FOFS(AbilitySounds, FileNames) },
  { "DirectoryBase", ST_STRING, FOFS(AbilitySounds, DirectoryBase) },
  { "Volume", ST_INT, FOFS(AbilitySounds, Volume) },
  { "Pitch", ST_INT, FOFS(AbilitySounds, Pitch) },
  { "PitchVariance", ST_INT, FOFS(AbilitySounds, PitchVariance) },
  { "Priority", ST_INT, FOFS(AbilitySounds, Priority) },
  { "Channel", ST_INT, FOFS(AbilitySounds, Channel) },
  { "Flags", ST_STRING, FOFS(AbilitySounds, Flags) },
  { "MinDistance", ST_INT, FOFS(AbilitySounds, MinDistance) },
  { "MaxDistance", ST_INT, FOFS(AbilitySounds, MaxDistance) },
  { "DistanceCutoff", ST_INT, FOFS(AbilitySounds, DistanceCutoff) },
  { "InsideAngle", ST_INT, FOFS(AbilitySounds, InsideAngle) },
  { "OutsideAngle", ST_INT, FOFS(AbilitySounds, OutsideAngle) },
  { "OutsideVolume", ST_INT, FOFS(AbilitySounds, OutsideVolume) },
  { "OrientationX", ST_INT, FOFS(AbilitySounds, OrientationX) },
  { "OrientationY", ST_INT, FOFS(AbilitySounds, OrientationY) },
  { "OrientationZ", ST_INT, FOFS(AbilitySounds, OrientationZ) },
  { "EAXFlags", ST_STRING, FOFS(AbilitySounds, EAXFlags) },
  { NULL },
};

struct AbilitySounds *FindAbilitySounds(LPCSTR SoundName) {
  struct AbilitySounds *lpValue = g_AbilitySounds;
  for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
  if (*lpValue->SoundName == 0) lpValue = NULL;
  return lpValue;
}

void InitAbilitySounds(void) {
  g_AbilitySounds = gi.ParseSheet("UI\\SoundInfo\\AbilitySounds.slk", AbilitySounds, sizeof(struct AbilitySounds));
}
void ShutdownAbilitySounds(void) {
  gi.MemFree(g_AbilitySounds);
}
