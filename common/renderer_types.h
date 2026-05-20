/*
 * renderer_types.h — Shared renderer types for UI and renderer modules
 *
 * This header defines renderer data structures that are shared between
 * the renderer module (renderer/) and UI module (ui/). Following the
 * Quake 3 pattern, these types define the renderer API contract.
 */
#ifndef renderer_types_h
#define renderer_types_h

#include "common.h"

/* Shader types for different rendering paths */
typedef enum {
    SHADER_DEFAULT,
    SHADER_UI,
    SHADER_SPLAT,
    SHADER_COMMANDBUTTON,
    SHADER_COUNT,
} SHADERTYPE;

/* Text drawing parameters */
typedef struct drawText_s {
    LPCFONT font;
    LPCSTR text;
    RECT rect;
    COLOR32 color;
    FLOAT textWidth;
    FLOAT lineHeight;
    BOOL wordWrap;
    uiFontJustificationH_t halign;
    uiFontJustificationV_t valign;
    LPCTEXTURE *icons;
} drawText_t;

/* Image drawing parameters */
typedef struct drawImage_s {
    LPCTEXTURE texture;
    SHADERTYPE shader;
    BLEND_MODE alphamode;
    RECT screen;
    RECT uv;
    COLOR32 color;
    BOOL rotate;
    FLOAT angle;
    FLOAT uActiveGlow;
} drawImage_t;

/* Standard pointer typedefs */
typedef drawText_t const *LPCDRAWTEXT;
typedef drawImage_t const *LPCDRAWIMAGE;

#endif
