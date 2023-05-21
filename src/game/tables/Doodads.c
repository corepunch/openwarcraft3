#include "../g_local.h"
#include "Doodads.h"

static struct Doodads *g_Doodads = NULL;

static struct SheetLayout Doodads[] = {
  { "doodID", ST_ID, FOFS(Doodads, doodID) },
  { "category", ST_ID, FOFS(Doodads, category) },
  { "tilesets", ST_STRING, FOFS(Doodads, tilesets) },
  { "tilesetSpecific", ST_INT, FOFS(Doodads, tilesetSpecific) },
  { "dir", ST_STRING, FOFS(Doodads, dir) },
  { "file", ST_STRING, FOFS(Doodads, file) },
  { "comment", ST_STRING, FOFS(Doodads, comment) },
  { "name", ST_STRING, FOFS(Doodads, name) },
  { "doodClass", ST_STRING, FOFS(Doodads, doodClass) },
  { "soundLoop", ST_STRING, FOFS(Doodads, soundLoop) },
  { "selSize", ST_INT, FOFS(Doodads, selSize) },
  { "defScale", ST_FLOAT, FOFS(Doodads, defScale) },
  { "minScale", ST_FLOAT, FOFS(Doodads, minScale) },
  { "maxScale", ST_FLOAT, FOFS(Doodads, maxScale) },
  { "canPlaceRandScale", ST_INT, FOFS(Doodads, canPlaceRandScale) },
  { "useClickHelper", ST_INT, FOFS(Doodads, useClickHelper) },
  { "maxPitch", ST_INT, FOFS(Doodads, maxPitch) },
  { "maxRoll", ST_INT, FOFS(Doodads, maxRoll) },
  { "visRadius", ST_INT, FOFS(Doodads, visRadius) },
  { "walkable", ST_INT, FOFS(Doodads, walkable) },
  { "numVar", ST_INT, FOFS(Doodads, numVar) },
  { "onCliffs", ST_INT, FOFS(Doodads, onCliffs) },
  { "onWater", ST_INT, FOFS(Doodads, onWater) },
  { "floats", ST_INT, FOFS(Doodads, floats) },
  { "shadow", ST_INT, FOFS(Doodads, shadow) },
  { "showInFog", ST_INT, FOFS(Doodads, showInFog) },
  { "animInFog", ST_INT, FOFS(Doodads, animInFog) },
  { "fixedRot", ST_INT, FOFS(Doodads, fixedRot) },
  { "pathTex", ST_STRING, FOFS(Doodads, pathTex) },
  { "showInMM", ST_INT, FOFS(Doodads, showInMM) },
  { "useMMColor", ST_INT, FOFS(Doodads, useMMColor) },
  { "MMRed", ST_INT, FOFS(Doodads, MMRed) },
  { "MMGreen", ST_INT, FOFS(Doodads, MMGreen) },
  { "MMBlue", ST_INT, FOFS(Doodads, MMBlue) },
  { "vertR01", ST_INT, FOFS(Doodads, vertR01) },
  { "vertG01", ST_INT, FOFS(Doodads, vertG01) },
  { "vertB01", ST_INT, FOFS(Doodads, vertB01) },
  { "vertR02", ST_INT, FOFS(Doodads, vertR02) },
  { "vertG02", ST_INT, FOFS(Doodads, vertG02) },
  { "vertB02", ST_INT, FOFS(Doodads, vertB02) },
  { "vertR03", ST_INT, FOFS(Doodads, vertR03) },
  { "vertG03", ST_INT, FOFS(Doodads, vertG03) },
  { "vertB03", ST_INT, FOFS(Doodads, vertB03) },
  { "vertR04", ST_INT, FOFS(Doodads, vertR04) },
  { "vertG04", ST_INT, FOFS(Doodads, vertG04) },
  { "vertB04", ST_INT, FOFS(Doodads, vertB04) },
  { "vertR05", ST_INT, FOFS(Doodads, vertR05) },
  { "vertG05", ST_INT, FOFS(Doodads, vertG05) },
  { "vertB05", ST_INT, FOFS(Doodads, vertB05) },
  { "vertR06", ST_INT, FOFS(Doodads, vertR06) },
  { "vertG06", ST_INT, FOFS(Doodads, vertG06) },
  { "vertB06", ST_INT, FOFS(Doodads, vertB06) },
  { "vertR07", ST_INT, FOFS(Doodads, vertR07) },
  { "vertG07", ST_INT, FOFS(Doodads, vertG07) },
  { "vertB07", ST_INT, FOFS(Doodads, vertB07) },
  { "vertR08", ST_INT, FOFS(Doodads, vertR08) },
  { "vertG08", ST_INT, FOFS(Doodads, vertG08) },
  { "vertB08", ST_INT, FOFS(Doodads, vertB08) },
  { "vertR09", ST_INT, FOFS(Doodads, vertR09) },
  { "vertG09", ST_INT, FOFS(Doodads, vertG09) },
  { "vertB09", ST_INT, FOFS(Doodads, vertB09) },
  { "vertR10", ST_INT, FOFS(Doodads, vertR10) },
  { "vertG10", ST_INT, FOFS(Doodads, vertG10) },
  { "vertB10", ST_INT, FOFS(Doodads, vertB10) },
  { "InBeta", ST_INT, FOFS(Doodads, InBeta) },
  { NULL },
};

struct Doodads *FindDoodads(DWORD doodID) {
  struct Doodads *lpValue = g_Doodads;
  for (; lpValue->doodID != doodID && lpValue->doodID; lpValue++);
  if (lpValue->doodID == 0) lpValue = NULL;
  return lpValue;
}

void InitDoodads(void) {
  g_Doodads = gi.ParseSheet("Doodads\\Doodads.slk", Doodads, sizeof(struct Doodads));
}
void ShutdownDoodads(void) {
  gi.MemFree(g_Doodads);
}
