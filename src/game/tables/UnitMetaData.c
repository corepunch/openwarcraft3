#include "../g_local.h"
#include "UnitMetaData.h"

static struct UnitMetaData *g_UnitMetaData = NULL;

static struct SheetLayout UnitMetaData[] = {
  { "ID", ST_ID, FOFS(UnitMetaData, ID) },
  { "field", ST_STRING, FOFS(UnitMetaData, field) },
  { "slk", ST_STRING, FOFS(UnitMetaData, slk) },
  { "index", ST_INT, FOFS(UnitMetaData, index) },
  { "displayName", ST_STRING, FOFS(UnitMetaData, displayName) },
  { "sort", ST_STRING, FOFS(UnitMetaData, sort) },
  { "type", ST_STRING, FOFS(UnitMetaData, type) },
  { "stringExt", ST_INT, FOFS(UnitMetaData, stringExt) },
  { "caseSens", ST_INT, FOFS(UnitMetaData, caseSens) },
  { "canBeEmpty", ST_INT, FOFS(UnitMetaData, canBeEmpty) },
  { "minVal", ST_STRING, FOFS(UnitMetaData, minVal) },
  { "maxVal", ST_STRING, FOFS(UnitMetaData, maxVal) },
  { "useHero", ST_INT, FOFS(UnitMetaData, useHero) },
  { "useUnit", ST_INT, FOFS(UnitMetaData, useUnit) },
  { "useBuilding", ST_INT, FOFS(UnitMetaData, useBuilding) },
  { "useItem", ST_INT, FOFS(UnitMetaData, useItem) },
  { "useSpecific", ST_INT, FOFS(UnitMetaData, useSpecific) },
  { NULL },
};

struct UnitMetaData *FindUnitMetaData(DWORD ID) {
  struct UnitMetaData *lpValue = g_UnitMetaData;
  for (; lpValue->ID != ID && lpValue->ID; lpValue++);
  if (lpValue->ID == 0) lpValue = NULL;
  return lpValue;
}

void InitUnitMetaData(void) {
  g_UnitMetaData = gi.ParseSheet("Units\\UnitMetaData.slk", UnitMetaData, sizeof(struct UnitMetaData));
}
void ShutdownUnitMetaData(void) {
  gi.MemFree(g_UnitMetaData);
}
