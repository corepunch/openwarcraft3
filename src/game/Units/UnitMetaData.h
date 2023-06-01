#ifndef __UnitMetaData_h__
#define __UnitMetaData_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct UnitMetaData {
	DWORD ID; /* uclg uclr iico uico usca umdl unsf unam */
	SHEETSTR field; /* green red Art Art modelScale file EditorSuffix Name */
	SHEETSTR slk; /* UnitUI UnitUI Profile Profile UnitUI UnitUI Profile Profile */
	DWORD index; /* -1 -1 0 0 -1 -1 0 0 */
	SHEETSTR displayName; /* WESTRING_UEVAL_UCLG WESTRING_UEVAL_UCLR WESTRING_UEVAL_IICO WESTRING_UEVAL_UICO WESTRING_UEVAL_USCA WESTRING_UEVAL_UMDL WESTRING_UEVAL_UNSF WESTRING_UEVAL_UNAM */
	SHEETSTR sort; /* c1a05 c1a04 c1a02 c1a02 c1a03 c1a01 c1a000 c1a00 */
	SHEETSTR type; /* int int item unit real unit string string */
	DWORD stringExt; /* 0 0 0 0 0 0 1 1 */
	DWORD caseSens; /* 1 1 1 1 1 1 1 1 */
	DWORD canBeEmpty; /* 0 0 0 0 0 0 1 0 */
	SHEETSTR minVal; /* 0 0 0.1 */
	SHEETSTR maxVal; /* 255 255 10 50 TTName */
	DWORD useHero; /* 1 1 0 1 1 1 1 1 */
	DWORD useUnit; /* 1 1 0 1 1 1 1 1 */
	DWORD useBuilding; /* 1 1 0 1 1 1 1 1 */
	DWORD useItem; /* 0 0 1 0 0 0 0 1 */
	DWORD useSpecific; /* */
};

typedef struct UnitMetaData UNITMETADATA;
typedef struct UnitMetaData *LPUNITMETADATA;
typedef struct UnitMetaData const *LPCUNITMETADATA;

LPCUNITMETADATA FindUnitMetaData(DWORD ID);
void InitUnitMetaData(void);
void ShutdownUnitMetaData(void);

#endif
