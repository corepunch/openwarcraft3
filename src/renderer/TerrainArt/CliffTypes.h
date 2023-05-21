#ifndef __CliffTypes_h__
#define __CliffTypes_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct CliffTypes {
	DWORD cliffID; /* CBgr CBde CWsn CWgr CFgr CFdi CLgr CLdi */
	SHEETSTR cliffModelDir; /* Cliffs Cliffs Cliffs Cliffs Cliffs Cliffs Cliffs Cliffs */
	SHEETSTR rampModelDir; /* CliffTrans CliffTrans CliffTrans CliffTrans CliffTrans CliffTrans CliffTrans CliffTrans */
	SHEETSTR texDir; /* ReplaceableTextures\Cliff ReplaceableTextures\Cliff ReplaceableTextures\Cliff ReplaceableTextures\Cliff ReplaceableTextures\Cliff ReplaceableTextures\Cliff ReplaceableTextures\Cliff ReplaceableTextures\Cliff */
	SHEETSTR texFile; /* Cliff1 Cliff0 Cliff1 Cliff0 Cliff1 Cliff0 Cliff1 Cliff0 */
	SHEETSTR name; /* WESTRING_CLIFF_CBgr WESTRING_CLIFF_CBde WESTRING_CLIFF_CWsn WESTRING_CLIFF_CWgr WESTRING_CLIFF_CFgr WESTRING_CLIFF_CFdi WESTRING_CLIFF_CLgr WESTRING_CLIFF_CLdi */
	DWORD groundTile; /* Bgrr Bdsr Wsnw Wgrs Fgrs Fdrt Lgrs Ldrt */
	DWORD upperTile; /* _ _ _ _ _ _ _ _ */
	DWORD cliffClass; /* c1 c2 c1 c2 c1 c2 c1 c2 */
	DWORD oldID; /* 7 6 4 5 3 2 1 0 */
};

struct CliffTypes *FindCliffTypes(DWORD cliffID);
void InitCliffTypes(void);
void ShutdownCliffTypes(void);

#endif
