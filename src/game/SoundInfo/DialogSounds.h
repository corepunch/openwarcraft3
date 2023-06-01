#ifndef __DialogSounds_h__
#define __DialogSounds_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct DialogSounds {
	SHEETSTR SoundName; /* H01Arthas26 H01Arthas24 H01Arthas18 H01Arthas10 H01Arthas08 H01Arthas06 H01Arthas04 H01Arthas02 */
	SHEETSTR FileNames; /* H01Arthas26.mp3 H01Arthas24.mp3 H01Arthas18.mp3 H01Arthas10.mp3 H01Arthas08.mp3 H01Arthas06.mp3 H01Arthas04.mp3 H01Arthas02.mp3 */
	SHEETSTR DirectoryBase; /* Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ Sound\Dialogue\HumanCampaign\Human01\ */
	DWORD Volume; /* 120 120 120 120 120 120 120 120 */
	SHEETSTR Flags; /* CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT CHANNELFULLPREEMPT|LISTFULLPREEMPT */
};

typedef struct DialogSounds DIALOGSOUNDS;
typedef struct DialogSounds *LPDIALOGSOUNDS;
typedef struct DialogSounds const *LPCDIALOGSOUNDS;

LPCDIALOGSOUNDS FindDialogSounds(LPCSTR SoundName);
void InitDialogSounds(void);
void ShutdownDialogSounds(void);

#endif
