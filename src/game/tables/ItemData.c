#include "../g_local.h"
#include "ItemData.h"

static struct ItemData *g_ItemData = NULL;

static struct SheetLayout ItemData[] = {
  { "itemID", ST_ID, FOFS(ItemData, itemID) },
  { "comment", ST_STRING, FOFS(ItemData, comment) },
  { "file", ST_STRING, FOFS(ItemData, file) },
  { "itemClass", ST_STRING, FOFS(ItemData, itemClass) },
  { "Level", ST_INT, FOFS(ItemData, Level) },
  { "uses", ST_INT, FOFS(ItemData, uses) },
  { "idealPrio", ST_INT, FOFS(ItemData, idealPrio) },
  { "prio", ST_INT, FOFS(ItemData, prio) },
  { "usable", ST_INT, FOFS(ItemData, usable) },
  { "perishable", ST_INT, FOFS(ItemData, perishable) },
  { "droppable", ST_INT, FOFS(ItemData, droppable) },
  { "drop", ST_INT, FOFS(ItemData, drop) },
  { "stockMax", ST_INT, FOFS(ItemData, stockMax) },
  { "stockRegen", ST_INT, FOFS(ItemData, stockRegen) },
  { "stockStart", ST_INT, FOFS(ItemData, stockStart) },
  { "goldcost", ST_INT, FOFS(ItemData, goldcost) },
  { "lumbercost", ST_INT, FOFS(ItemData, lumbercost) },
  { "HP", ST_INT, FOFS(ItemData, HP) },
  { "armor", ST_ID, FOFS(ItemData, armor) },
  { "abilList", ST_STRING, FOFS(ItemData, abilList) },
  { "InBeta", ST_INT, FOFS(ItemData, InBeta) },
  { NULL },
};

struct ItemData *FindItemData(DWORD itemID) {
  struct ItemData *lpValue = g_ItemData;
  for (; lpValue->itemID != itemID && lpValue->itemID; lpValue++);
  if (lpValue->itemID == 0) lpValue = NULL;
  return lpValue;
}

void InitItemData(void) {
  g_ItemData = gi.ParseSheet("Units\\ItemData.slk", ItemData, sizeof(struct ItemData));
}
void ShutdownItemData(void) {
  gi.MemFree(g_ItemData);
}
