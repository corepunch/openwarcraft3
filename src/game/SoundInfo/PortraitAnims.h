#ifndef __PortraitAnims_h__
#define __PortraitAnims_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct PortraitAnims {
	SHEETSTR Sound_Label; /* OgreYes OgreWhat OgreWarcry OgrePissed MortarTeamYesAttack MortarTeamYes MortarTeamWhat MortarTeamPissed */
	float Anim_Indices_; /* _2,1,2,1 _2,1,2,1 _2 _1,2,1,2,1 _3,3,4,3,4 _3,3,3,3,3,4 _3,4,3,4 _4,3,4,3,3,4,4,4,4 */
	float Speech_Duration_; /* _-1,-1,-1,-1 _-1,-1,-1,-1 _-1 _-1,-1,-1,-1,-1 _-1,-1,-1,-1,-1 _-1,-1,-1,-1,-1,-1 _-1,-1,-1,-1 _-1,-1,-1,-1,-1,-1,-1,-1,-1 */
};

struct PortraitAnims *FindPortraitAnims(LPCSTR Sound_Label);
void InitPortraitAnims(void);
void ShutdownPortraitAnims(void);

#endif
