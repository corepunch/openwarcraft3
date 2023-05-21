#ifndef __MIDISounds_h__
#define __MIDISounds_h__

#include "../../common/common.h"

typedef char SHEETSTR[80];

struct MIDISounds {
	SHEETSTR SoundLabel; /* LordaeronFallNight LordaeronFallDay CityScapeNight CityScapeDay BarrensNight BarrensDay AshenvaleNight AshenvaleDay */
	SHEETSTR DirectoryBase; /* Sound\Ambient\LordaeronFall Sound\Ambient\LordaeronFall Sound\Ambient\CityScape Sound\Ambient\CityScape Sound\Ambient\Barrens Sound\Ambient\Barrens Sound\Ambient\Ashenvale Sound\Ambient\Ashenvale */
	SHEETSTR MIDIFileName; /* LordaeronFallNight.mid LordaeronFallDay.mid CityScapeNight.mid CityScapeDay.mid BarrensNight.mid BarrensDay.mid AshenvaleNight.mid AshenvaleDay.mid */
	SHEETSTR DLSFileName; /* LordaeronFall.dls LordaeronFall.dls CityScape.dls CityScape.dls Barrens.dls Barrens.dls Ashenvale.dls Ashenvale.dls */
	DWORD Volume; /* 80 80 80 80 80 70 80 80 */
	DWORD Priority_; /* 1000 1000 1000 1000 1000 1000 1000 1000 */
	DWORD Pitch; /* 1 1 1 1 1 1 1 1 */
	DWORD Channel; /* 15 15 15 15 15 15 15 15 */
	DWORD Radius; /* 5000 5000 5000 5000 5000 5000 5000 5000 */
	SHEETSTR Flags; /* CHANNELFULLPREEMPT CHANNELFULLPREEMPT CHANNELFULLPREEMPT CHANNELFULLPREEMPT CHANNELFULLPREEMPT CHANNELFULLPREEMPT CHANNELFULLPREEMPT CHANNELFULLPREEMPT */
};

typedef struct MIDISounds MIDISOUNDS;
typedef struct MIDISounds *LPMIDISOUNDS;
typedef struct MIDISounds const *LPCMIDISOUNDS;

LPCMIDISOUNDS FindMIDISounds(LPCSTR SoundLabel);
void InitMIDISounds(void);
void ShutdownMIDISounds(void);

#endif
