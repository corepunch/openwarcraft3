#include "../g_local.h"
#include "SplatData.h"

static struct SplatData *g_SplatData = NULL;

static struct SheetLayout SplatData[] = {
  { "Name", ST_ID, FOFS(SplatData, Name) },
  { "comment", ST_STRING, FOFS(SplatData, comment) },
  { "Dir", ST_STRING, FOFS(SplatData, Dir) },
  { "file", ST_STRING, FOFS(SplatData, file) },
  { "Rows", ST_INT, FOFS(SplatData, Rows) },
  { "Columns", ST_INT, FOFS(SplatData, Columns) },
  { "BlendMode", ST_INT, FOFS(SplatData, BlendMode) },
  { "Scale", ST_INT, FOFS(SplatData, Scale) },
  { "Lifespan", ST_FLOAT, FOFS(SplatData, Lifespan) },
  { "Decay", ST_FLOAT, FOFS(SplatData, Decay) },
  { "UVLifespanStart", ST_INT, FOFS(SplatData, UVLifespanStart) },
  { "UVLifespanEnd", ST_INT, FOFS(SplatData, UVLifespanEnd) },
  { "LifespanRepeat", ST_INT, FOFS(SplatData, LifespanRepeat) },
  { "UVDecayStart", ST_INT, FOFS(SplatData, UVDecayStart) },
  { "UVDecayEnd", ST_INT, FOFS(SplatData, UVDecayEnd) },
  { "DecayRepeat", ST_INT, FOFS(SplatData, DecayRepeat) },
  { "StartR", ST_INT, FOFS(SplatData, StartR) },
  { "StartG", ST_INT, FOFS(SplatData, StartG) },
  { "StartB", ST_INT, FOFS(SplatData, StartB) },
  { "StartA", ST_INT, FOFS(SplatData, StartA) },
  { "MiddleR", ST_INT, FOFS(SplatData, MiddleR) },
  { "MiddleG", ST_INT, FOFS(SplatData, MiddleG) },
  { "MiddleB", ST_INT, FOFS(SplatData, MiddleB) },
  { "MiddleA", ST_INT, FOFS(SplatData, MiddleA) },
  { "EndR", ST_INT, FOFS(SplatData, EndR) },
  { "EndG", ST_INT, FOFS(SplatData, EndG) },
  { "EndB", ST_INT, FOFS(SplatData, EndB) },
  { "EndA", ST_INT, FOFS(SplatData, EndA) },
  { "Water", ST_ID, FOFS(SplatData, Water) },
  { "Sound", ST_ID, FOFS(SplatData, Sound) },
  { NULL },
};

struct SplatData *FindSplatData(DWORD Name) {
  struct SplatData *lpValue = g_SplatData;
  for (; lpValue->Name != Name && lpValue->Name; lpValue++);
  if (lpValue->Name == 0) lpValue = NULL;
  return lpValue;
}

void InitSplatData(void) {
  g_SplatData = gi.ParseSheet("Splats\\SplatData.slk", SplatData, sizeof(struct SplatData));
}
void ShutdownSplatData(void) {
  gi.MemFree(g_SplatData);
}
