#include "../g_local.h"
#include "T_SplatData.h"

static struct T_SplatData *g_T_SplatData = NULL;

static struct SheetLayout T_SplatData[] = {
  { "Name", ST_ID, FOFS(T_SplatData, Name) },
  { "comment", ST_STRING, FOFS(T_SplatData, comment) },
  { "Dir", ST_STRING, FOFS(T_SplatData, Dir) },
  { "file", ST_STRING, FOFS(T_SplatData, file) },
  { "Rows", ST_INT, FOFS(T_SplatData, Rows) },
  { "Columns", ST_INT, FOFS(T_SplatData, Columns) },
  { "BlendMode", ST_INT, FOFS(T_SplatData, BlendMode) },
  { "Scale", ST_INT, FOFS(T_SplatData, Scale) },
  { "Lifespan", ST_FLOAT, FOFS(T_SplatData, Lifespan) },
  { "Decay", ST_FLOAT, FOFS(T_SplatData, Decay) },
  { "UVLifespanStart", ST_INT, FOFS(T_SplatData, UVLifespanStart) },
  { "UVLifespanEnd", ST_INT, FOFS(T_SplatData, UVLifespanEnd) },
  { "LifespanRepeat", ST_INT, FOFS(T_SplatData, LifespanRepeat) },
  { "UVDecayStart", ST_INT, FOFS(T_SplatData, UVDecayStart) },
  { "UVDecayEnd", ST_INT, FOFS(T_SplatData, UVDecayEnd) },
  { "DecayRepeat", ST_INT, FOFS(T_SplatData, DecayRepeat) },
  { "StartR", ST_INT, FOFS(T_SplatData, StartR) },
  { "StartG", ST_INT, FOFS(T_SplatData, StartG) },
  { "StartB", ST_INT, FOFS(T_SplatData, StartB) },
  { "StartA", ST_INT, FOFS(T_SplatData, StartA) },
  { "MiddleR", ST_INT, FOFS(T_SplatData, MiddleR) },
  { "MiddleG", ST_INT, FOFS(T_SplatData, MiddleG) },
  { "MiddleB", ST_INT, FOFS(T_SplatData, MiddleB) },
  { "MiddleA", ST_INT, FOFS(T_SplatData, MiddleA) },
  { "EndR", ST_INT, FOFS(T_SplatData, EndR) },
  { "EndG", ST_INT, FOFS(T_SplatData, EndG) },
  { "EndB", ST_INT, FOFS(T_SplatData, EndB) },
  { "EndA", ST_INT, FOFS(T_SplatData, EndA) },
  { "Water", ST_ID, FOFS(T_SplatData, Water) },
  { "Sound", ST_ID, FOFS(T_SplatData, Sound) },
  { NULL },
};

struct T_SplatData *FindT_SplatData(DWORD Name) {
  struct T_SplatData *lpValue = g_T_SplatData;
  for (; lpValue->Name != Name && lpValue->Name; lpValue++);
  if (lpValue->Name == 0) lpValue = NULL;
  return lpValue;
}

void InitT_SplatData(void) {
  g_T_SplatData = gi.ParseSheet("Splats\\T_SplatData.slk", T_SplatData, sizeof(struct T_SplatData));
}
void ShutdownT_SplatData(void) {
  gi.MemFree(g_T_SplatData);
}
