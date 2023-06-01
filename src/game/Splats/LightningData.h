#ifndef __LightningData_h__
#define __LightningData_h__

#include "../../common/shared.h"

typedef char SHEETSTR[80];

struct LightningData {
	DWORD Name; /* AFOD CHIM MBUR CLSB CLPB INIT */
	SHEETSTR comment; /* Finger of Death Lightning Attack - Chimera Mana Burn - Bolt Chain Lightning - Secondary Bolt Chain Lightning - Primary Bolt INIT */
	SHEETSTR Dir; /* ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather ReplaceableTextures\Weather */
	SHEETSTR file; /* LightningRed.blp Lightning.blp ManaBurnBeam.blp Lightning.blp Lightning.blp Lightning.blp */
	DWORD AvgSegLen; /* 100 100 100 100 100 100 */
	DWORD Width; /* 50 45 45 30 50 40 */
	DWORD R; /* 255 255 255 255 255 255 */
	DWORD G; /* 255 255 255 255 255 255 */
	DWORD B; /* 255 255 255 255 255 255 */
	DWORD A; /* 255 255 255 255 255 255 */
	float NoiseScale; /* 0.05 0.04 0.04 0.05 0.05 0.05 */
	float TexCoordScale; /* 0.5 0.5 0.6 0.5 0.5 0.5 */
	DWORD Duration; /* 2 2 2 2 2 2 */
};

typedef struct LightningData LIGHTNINGDATA;
typedef struct LightningData *LPLIGHTNINGDATA;
typedef struct LightningData const *LPCLIGHTNINGDATA;

LPCLIGHTNINGDATA FindLightningData(DWORD Name);
void InitLightningData(void);
void ShutdownLightningData(void);

#endif
