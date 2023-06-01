#ifndef __AnimLookups_h__
#define __AnimLookups_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct AnimLookups {
	DWORD AnimSoundEvent; /* ABTR ABSK ABPU ABPD ABLO AAMS FBCR FBCL */
	SHEETSTR SoundLabel; /* BattleRoar BerserkerRage BothGlueScreenPopUp BothGlueScreenPopDown Bloodlust AntiMagicshell TestFootstep TestFootstep */
};

typedef struct AnimLookups ANIMLOOKUPS;
typedef struct AnimLookups *LPANIMLOOKUPS;
typedef struct AnimLookups const *LPCANIMLOOKUPS;

LPCANIMLOOKUPS FindAnimLookups(DWORD AnimSoundEvent);
void InitAnimLookups(void);
void ShutdownAnimLookups(void);

#endif
