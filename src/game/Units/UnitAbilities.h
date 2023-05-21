#ifndef __UnitAbilities_h__
#define __UnitAbilities_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct UnitAbilities {
	DWORD unitAbilID; /* hmtm hmpr hgyr hgry hfoo Hpal Hmkg Hamg */
	DWORD sortAbil; /* a2 a2 a2 a2 a2 a1 a1 a1 */
	SHEETSTR comment; /* MortarTeam Priest Gyrocopter GryphonRider Footman HeroPaladin HeroMountainKing HeroArchMage */
	DWORD auto_; /* _ Ahea _ _ _ _ _ _ */
	SHEETSTR abilList; /* Afla Ahea,Ainf,Adis Agyb,Agyv Asth Adef _ _ _ */
	SHEETSTR heroAbilList; /* AHhb,AHds,AHre,AHad AHtc,AHtb,AHbh,AHav AHbz,AHab,AHwe,AHmt */
	DWORD InBeta; /* 1 1 1 1 1 1 1 1 */
};

struct UnitAbilities *FindUnitAbilities(DWORD unitAbilID);
void InitUnitAbilities(void);
void ShutdownUnitAbilities(void);

#endif
