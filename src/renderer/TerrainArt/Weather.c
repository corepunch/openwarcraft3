#include "../r_local.h"
#include "Weather.h"

static struct Weather *g_Weather = NULL;

static struct SheetLayout Weather[] = {
  { "effectID", ST_ID, FOFS(Weather, effectID) },
  { "name", ST_STRING, FOFS(Weather, name) },
  { "texDir", ST_STRING, FOFS(Weather, texDir) },
  { "texFile", ST_STRING, FOFS(Weather, texFile) },
  { "alphaMode", ST_INT, FOFS(Weather, alphaMode) },
  { "useFog", ST_INT, FOFS(Weather, useFog) },
  { "height", ST_INT, FOFS(Weather, height) },
  { "angx", ST_INT, FOFS(Weather, angx) },
  { "angy", ST_INT, FOFS(Weather, angy) },
  { "emrate", ST_FLOAT, FOFS(Weather, emrate) },
  { "lifespan", ST_FLOAT, FOFS(Weather, lifespan) },
  { "particles", ST_INT, FOFS(Weather, particles) },
  { "veloc", ST_INT, FOFS(Weather, veloc) },
  { "accel", ST_INT, FOFS(Weather, accel) },
  { "var", ST_FLOAT, FOFS(Weather, var) },
  { "texr", ST_INT, FOFS(Weather, texr) },
  { "texc", ST_INT, FOFS(Weather, texc) },
  { "head", ST_INT, FOFS(Weather, head) },
  { "tail", ST_INT, FOFS(Weather, tail) },
  { "taillen", ST_FLOAT, FOFS(Weather, taillen) },
  { "lati", ST_FLOAT, FOFS(Weather, lati) },
  { "long", ST_INT, FOFS(Weather, long_) },
  { "midTime", ST_FLOAT, FOFS(Weather, midTime) },
  { "redStart", ST_INT, FOFS(Weather, redStart) },
  { "greenStart", ST_INT, FOFS(Weather, greenStart) },
  { "blueStart", ST_INT, FOFS(Weather, blueStart) },
  { "redMid", ST_INT, FOFS(Weather, redMid) },
  { "greenMid", ST_INT, FOFS(Weather, greenMid) },
  { "blueMid", ST_INT, FOFS(Weather, blueMid) },
  { "redEnd", ST_INT, FOFS(Weather, redEnd) },
  { "greenEnd", ST_INT, FOFS(Weather, greenEnd) },
  { "blueEnd", ST_INT, FOFS(Weather, blueEnd) },
  { "alphaStart", ST_INT, FOFS(Weather, alphaStart) },
  { "alphaMid", ST_INT, FOFS(Weather, alphaMid) },
  { "alphaEnd", ST_INT, FOFS(Weather, alphaEnd) },
  { "scaleStart", ST_FLOAT, FOFS(Weather, scaleStart) },
  { "scaleMid", ST_FLOAT, FOFS(Weather, scaleMid) },
  { "scaleEnd", ST_FLOAT, FOFS(Weather, scaleEnd) },
  { "hUVStart", ST_INT, FOFS(Weather, hUVStart) },
  { "hUVMid", ST_INT, FOFS(Weather, hUVMid) },
  { "hUVEnd", ST_INT, FOFS(Weather, hUVEnd) },
  { "tUVStart", ST_INT, FOFS(Weather, tUVStart) },
  { "tUVMid", ST_INT, FOFS(Weather, tUVMid) },
  { "tUVEnd", ST_INT, FOFS(Weather, tUVEnd) },
  { "AmbientSound", ST_STRING, FOFS(Weather, AmbientSound) },
  { NULL },
};

struct Weather *FindWeather(DWORD effectID) {
  struct Weather *lpValue = g_Weather;
  for (; lpValue->effectID != effectID && lpValue->effectID; lpValue++);
  if (lpValue->effectID == 0) lpValue = NULL;
  return lpValue;
}

void InitWeather(void) {
  g_Weather = ri.ParseSheet("TerrainArt\\Weather.slk", Weather, sizeof(struct Weather));
}
void ShutdownWeather(void) {
    ri.MemFree(g_Weather);
}
