#ifndef __AnimLookups_h__
#define __AnimLookups_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct AnimLookups {
  DWORD AnimSoundEvent; /* ABTR ABSK ABPU ABPD ABLO AAMS FBCR FBCL */
  SHEETSTR SoundLabel; /* BattleRoar BerserkerRage BothGlueScreenPopUp BothGlueScreenPopDown Bloodlust AntiMagicshell TestFootstep TestFootstep */
};

struct AnimLookups *FindAnimLookups(DWORD AnimSoundEvent);
void InitAnimLookups(void);
void ShutdownAnimLookups(void);

#endif
