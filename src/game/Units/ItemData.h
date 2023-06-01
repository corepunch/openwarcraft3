#ifndef __ItemData_h__
#define __ItemData_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct ItemData {
	DWORD itemID; /* fgsk kybl bgst pams ankh ajen amrc afac */
	SHEETSTR comment; /* Book of the Dead bloody key Belt of Giant Strength +6 Anti-Magic Potion Ankh of Reincarnation Ancient Janggo of Endurance Amulet of Recall Alleria's Flute of Accuracy */
	SHEETSTR file; /* Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl Objects\InventoryItems\TreasureChest\treasurechest.mdl */
	SHEETSTR itemClass; /* misc misc clothing potiond amulet misc amulet misc */
	DWORD Level; /* 5 0 8 2 6 6 4 6 */
	DWORD uses; /* 1 - - 1 1 - 1 - */
	DWORD idealPrio; /* */
	DWORD prio; /* 30 0 71 34 84 47 9 36 */
	DWORD usable; /* 1 0 0 1 0 0 1 0 */
	DWORD perishable; /* 1 0 0 1 1 0 1 0 */
	DWORD droppable; /* 1 1 1 1 1 1 1 1 */
	DWORD drop; /* 0 0 0 0 0 0 0 0 */
	DWORD stockMax; /* 1 3 1 3 1 1 2 1 */
	DWORD stockRegen; /* 120 120 120 120 120 120 120 120 */
	DWORD stockStart; /* 0 0 0 0 0 0 0 0 */
	DWORD goldcost; /* 325 200 650 150 400 400 250 400 */
	DWORD lumbercost; /* 0 0 0 0 0 0 0 0 */
	DWORD HP; /* 100 100 100 100 100 100 100 100 */
	DWORD armor; /* Wood Wood Wood Wood Wood Wood Wood Wood */
	SHEETSTR abilList; /* AIfs _ AIs6 AIxs AIrc AIae AIrt AIar */
	DWORD InBeta; /* 1 0 1 1 1 1 1 1 */
};

typedef struct ItemData ITEMDATA;
typedef struct ItemData *LPITEMDATA;
typedef struct ItemData const *LPCITEMDATA;

LPCITEMDATA FindItemData(DWORD itemID);
void InitItemData(void);
void ShutdownItemData(void);

#endif
