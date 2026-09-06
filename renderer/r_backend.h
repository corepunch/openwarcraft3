#ifndef r_backend_h
#define r_backend_h

/* Backend state cache and thin-pipeline types.
 * Tracking: #89 (renderer thin pipelines, root struct, GL state cache).
 *
 * RB_* owns GL blend/depth/cull/scissor/viewport/VAO/FBO state.  Frontend
 * draw functions never issue raw GL state changes; they call RB_* helpers
 * which diff against the cache and only emit GL on delta.  This is the GL
 * implementation of Aaltonen's cheap blend/depth packet -- not a public
 * fixed-function API.
 *
 * Thin pipeline types (pipelineDesc_t / pipeline_t) are defined here for
 * Phase 3 forward reference.  Phase 1 only implements the state cache.
 *
 * Include this header AFTER r_local.h has defined the GL types and R_Call. */

/* -----------------------------------------------------------------------
 * Packed GL state bits  (Doom 3 GL_State XOR style)
 *
 * Bit layout chosen to match the common GL enum values so callers can OR
 * them directly:
 *   bits 0-3:  blend src  (GL_SRC_ALPHA=4, etc.)
 *   bits 4-7:  blend dst  (GL_ONE_MINUS_SRC_ALPHA=5, etc.)
 *   bit  8:    depth write  (GL_TRUE)
 *   bits 9-11: depth func  (GL_LEQUAL=5, GL_ALWAYS=7, etc.)
 *   bits 12-15: color mask (4 bits: R G B A)
 * ----------------------------------------------------------------------- */

enum {
    RB_BLEND_SRC_MASK   = 0x000F,
    RB_BLEND_DST_MASK   = 0x00F0,
    RB_DEPTH_WRITE_BIT  = 0x0100,
    RB_DEPTH_FUNC_MASK  = 0x0E00,
    RB_COLOR_MASK_SHIFT = 12,
    RB_COLOR_MASK_MASK  = 0xF000,
};

/* Shift a 4-bit value into a field position. */
static inline DWORD RB_Bits4(DWORD val, int shift) { return (val & 0xF) << shift; }

/* Convenience: build packed state bits from individual GL enums. */
static inline DWORD RB_MakeStateBits(GLenum blendSrc, GLenum blendDst, BOOL depthWrite,
                                      GLenum depthFunc, DWORD colorMask) {
    return RB_Bits4(blendSrc, 0)
         | RB_Bits4(blendDst, 4)
         | (depthWrite ? RB_DEPTH_WRITE_BIT : 0)
         | RB_Bits4(depthFunc, 9)
         | ((colorMask & 0xF) << RB_COLOR_MASK_SHIFT);
}

/* GL_SRC_ALPHA=0x0302, GL_ONE_MINUS_SRC_ALPHA=0x0303, GL_ONE=1,
 * GL_ZERO=0, GL_DST_COLOR=0x0306, GL_SRC_COLOR=0x0307,
 * GL_LEQUAL=0x0203, GL_ALWAYS=0x0207.
 * Enum values don't fold to integer constant expressions, so the pre-built
 * bits are computed manually from the 4-bit field layout:
 *   bits 0-3: blend src, bits 4-7: blend dst, bit 8: depth write,
 *   bits 9-11: depth func, bits 12-15: color mask. */
enum {
    RB_STATE_OPAQUE        = 0,
    RB_STATE_ALPHA_TEST    = 0x0100,
    RB_STATE_BLEND_ALPHA   = (0x0302 << 0) | (0x0303 << 4) | (0x0203 << 9) | (0xF << 12),
    RB_STATE_BLEND_ADD     = (0x0001 << 0) | (0x0001 << 4) | (0x0203 << 9) | (0xF << 12),
    RB_STATE_BLEND_ONE     = (0x0001 << 0) | (0x0303 << 4) | (0x0203 << 9) | (0xF << 12),
    RB_STATE_BLEND_MOD2X   = (0x0306 << 0) | (0x0307 << 4) | (0x0203 << 9) | (0xF << 12),
    RB_STATE_BLEND_MOD4X   = (0x0306 << 0) | (0x0307 << 4) | (0x0203 << 9) | (0xF << 12),
    RB_STATE_NO_DEPTH      = (0x0302 << 0) | (0x0303 << 4) | (0x0203 << 9) | (0xF << 12),
};

/* -----------------------------------------------------------------------
 * Back-end state cache
 * ----------------------------------------------------------------------- */

typedef struct backEndState {
    DWORD   glStateBits;       /* packed blend/depth/color-mask */
    GLenum  faceCulling;       /* current glCullFace value; 0 = not set */
    GLenum  depthFunc;         /* current glDepthFunc value */
    DWORD   polygonOffsetScale;
    DWORD   polygonOffsetBias;
    DWORD   blendEquation;     /* current glBlendEquation */
    DWORD   activeTextureUnit;
    DWORD   currentVAO;
    DWORD   currentFBO;
    RECT    currentScissor;
    RECT    currentViewport;
    BOOL    scissorEnabled;
} backEndState_t;

extern backEndState_t backEnd;

/* -----------------------------------------------------------------------
 * RB_* API — state cache helpers
 *
 * Each helper compares against the cached value and only issues the GL
 * call on delta.  R_Call() wraps every raw GL call for diagnostics.
 * ----------------------------------------------------------------------- */

/* Set packed blend/depth/color-mask state from a bitmask. */
void RB_State(DWORD bits);

/* Set face culling mode (GL_CULL_FACE enable/disable + glCullFace). */
void RB_Cull(GLenum mode);

/* Set polygon offset fill.  Pass 0,0 to disable. */
void RB_PolygonOffset(float scale, float bias);

/* Set blend equation (GL_FUNC_ADD, GL_MAX, etc.). */
void RB_BlendEquation(GLenum eq);

/* Set scissor rectangle (pixels).  Enables GL_SCISSOR_TEST. */
void RB_Scissor(LPCRECT r);

/* Disable scissor test. */
void RB_ScissorDisable(void);

/* Set viewport (pixels). */
void RB_Viewport(LPCRECT r);

/* Bind a VAO. */
void RB_BindVAO(DWORD vao);

/* Bind an FBO.  Pass 0 for the default framebuffer. */
void RB_BindFBO(DWORD fbo);

/* Set color mask (4 booleans: R, G, B, A). */
void RB_ColorMask(BOOL r, BOOL g, BOOL b, BOOL a);

/* Reset all cached state to the GL defaults used at frame start. */
void RB_ResetState(void);

/* -----------------------------------------------------------------------
 * Thin pipeline types (Phase 3 forward declaration)
 * ----------------------------------------------------------------------- */

/* Raster state blob baked into a pipeline object. */
typedef struct pipelineRasterState {
    BOOL     depthWrite;
    GLenum   depthFunc;
    BOOL     blendEnable;
    GLenum   blendSrc;
    GLenum   blendDst;
    GLenum   blendEquation;
    DWORD    colorMask;       /* 4-bit RGBA mask */
    GLenum   cullFace;        /* GL_BACK, GL_FRONT, or 0 (disabled) */
} pipelineRasterState_t;

/* Pipeline descriptor: shaders + raster state + formats. */
typedef struct pipelineDesc {
    GLuint   vertShader;      /* compiled vertex shader */
    GLuint   fragShader;      /* compiled fragment shader */
    GLenum   colorFormat;     /* render target color format */
    GLenum   depthFormat;     /* render target depth format */
    pipelineRasterState_t raster;
} pipelineDesc_t;

/* Created pipeline handle: program + raster state. */
typedef struct pipeline {
    GLuint   progid;          /* linked program */
    pipelineRasterState_t raster;
    DWORD    id;              /* monotonic for change detection */
} pipeline_t;

/* Initialize the backend (call once after GL context creation). */
void RB_Init(void);

/* Create a pipeline from a descriptor.  Links the shader program and
 * stores the raster state.  Returns a pipeline_t ready for RB_BindPipeline. */
pipeline_t RB_CreatePipeline(pipelineDesc_t *desc);

/* Bind a pipeline: set the GL program and apply the raster state through
 * the state cache.  Skips GL calls if the pipeline hasn't changed. */
void RB_BindPipeline(pipeline_t *p);

/* Build a pipeline from an existing SHADERPROG + raster state.
 * Convenience for the current shader system where programs are already linked. */
pipeline_t RB_MakePipeline(SHADERPROG *prog, pipelineRasterState_t *raster);

#endif /* r_backend_h */
