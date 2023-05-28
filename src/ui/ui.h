#ifndef ui_h
#define ui_h

#include "../common/common.h"

#define UI_SCALE 1000

typedef char uiName_t[80];

typedef struct {
    entityState_t const *current;
    model_t const *portraitModel;
} uiPortrait_t;

typedef enum {
    FT_SIMPLEFRAME,
} uiFrameType_t;

typedef enum {
    AM_ALPHAKEY,
} uiAlphaMode_t;

typedef enum {
    AC_TOPLEFT,
    AC_TOPRIGHT,
    AC_BOTTOMLEFT,
    AC_BOTTOMRIGHT,
} uiAnchorCorner_t;

typedef struct {
    uiAnchorCorner_t corner;
    float x;
    float y;
} uiAnchor_t;

typedef struct textureDef_s {
    struct textureDef_s *next;
    uiName_t Name;
    float Width;
    float Height;
    LPCTEXTURE Texture;
    uiAlphaMode_t AlphaMode;
    uiAnchor_t Anchor;
    RECT  TexCoord;
} uiTextureDef_t;

typedef struct frameDef_s {
    struct frameDef_s *next;
    uiFrameType_t Type;
    uiName_t Name;
    bool DecorateFileNames;
    uiTextureDef_t *textures;
} uiFrameDef_t;

typedef struct configValue_s {
    uiName_t Name;
    LPSTR Value;
    struct configValue_s *next;
} configValue_t;

typedef struct configSection_s {
    uiName_t Name;
    struct configValue_s *values;
    struct configSection_s *next;
} configSection_t;

// FDF
uiFrameDef_t *FDF_ParseFile(LPCSTR filename);

// INI
configSection_t *INI_ParseFile(LPCSTR filename);
configSection_t *INI_FindSection(configSection_t *ini, LPCSTR sectionName);
LPCSTR INI_FindValue(configSection_t *ini, LPCSTR valueName);

// UI
void UI_Init(void);
void UI_Shutdown(void);
void UI_Draw(void);

#endif /* ui_h */
