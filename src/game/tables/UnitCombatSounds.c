#include "../g_local.h"
#include "UnitCombatSounds.h"

static struct UnitCombatSounds *g_UnitCombatSounds = NULL;

static struct SheetLayout UnitCombatSounds[] = {
  { "SoundName", ST_STRING, FOFS(UnitCombatSounds, SoundName) },
  { "FileNames", ST_STRING, FOFS(UnitCombatSounds, FileNames) },
  { "DirectoryBase", ST_STRING, FOFS(UnitCombatSounds, DirectoryBase) },
  { "Volume", ST_INT, FOFS(UnitCombatSounds, Volume) },
  { "Pitch", ST_INT, FOFS(UnitCombatSounds, Pitch) },
  { "PitchVariance", ST_FLOAT, FOFS(UnitCombatSounds, PitchVariance) },
  { "Priority", ST_INT, FOFS(UnitCombatSounds, Priority) },
  { "Channel", ST_INT, FOFS(UnitCombatSounds, Channel) },
  { "Flags", ST_STRING, FOFS(UnitCombatSounds, Flags) },
  { "MinDistance", ST_INT, FOFS(UnitCombatSounds, MinDistance) },
  { "MaxDistance", ST_INT, FOFS(UnitCombatSounds, MaxDistance) },
  { "DistanceCutoff", ST_INT, FOFS(UnitCombatSounds, DistanceCutoff) },
  { "InsideAngle", ST_INT, FOFS(UnitCombatSounds, InsideAngle) },
  { "OutsideAngle", ST_INT, FOFS(UnitCombatSounds, OutsideAngle) },
  { "OutsideVolume", ST_INT, FOFS(UnitCombatSounds, OutsideVolume) },
  { "OrientationX", ST_INT, FOFS(UnitCombatSounds, OrientationX) },
  { "OrientationY", ST_INT, FOFS(UnitCombatSounds, OrientationY) },
  { "OrientationZ", ST_INT, FOFS(UnitCombatSounds, OrientationZ) },
  { "EAXFlags", ST_STRING, FOFS(UnitCombatSounds, EAXFlags) },
  { NULL },
};

struct UnitCombatSounds *FindUnitCombatSounds(LPCSTR SoundName) {
  struct UnitCombatSounds *lpValue = g_UnitCombatSounds;
  for (; *lpValue->SoundName && strcmp(lpValue->SoundName, SoundName); lpValue++);
  if (*lpValue->SoundName == 0) lpValue = NULL;
  return lpValue;
}

void InitUnitCombatSounds(void) {
  g_UnitCombatSounds = gi.ParseSheet("UI\\SoundInfo\\UnitCombatSounds.slk", UnitCombatSounds, sizeof(struct UnitCombatSounds));
}
void ShutdownUnitCombatSounds(void) {
  gi.MemFree(g_UnitCombatSounds);
}
