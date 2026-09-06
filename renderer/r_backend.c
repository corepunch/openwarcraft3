#include "r_local.h"

backEndState_t backEnd;

void RB_State(DWORD bits) {
    DWORD delta = bits ^ backEnd.glStateBits;
    if (!delta) return;

    /* Blend enable/disable (bit 0 = any blend src != 0 or dst != 0). */
    bool wantBlend = (bits & RB_BLEND_SRC_MASK) || (bits & RB_BLEND_DST_MASK);
    bool haveBlend = (backEnd.glStateBits & RB_BLEND_SRC_MASK) || (backEnd.glStateBits & RB_BLEND_DST_MASK);
    if (wantBlend != haveBlend) {
        R_Call(wantBlend ? glEnable : glDisable, GL_BLEND);
    }

    /* Blend function (src in bits 0-3, dst in bits 4-7). */
    if ((delta & RB_BLEND_SRC_MASK) || (delta & RB_BLEND_DST_MASK)) {
        GLenum src = (bits & RB_BLEND_SRC_MASK);
        GLenum dst = (bits & RB_BLEND_DST_MASK) >> 4;
        R_Call(glBlendFunc, src, dst);
    }

    /* Depth write (bit 8). */
    if (delta & RB_DEPTH_WRITE_BIT) {
        R_Call(glDepthMask, (bits & RB_DEPTH_WRITE_BIT) ? GL_TRUE : GL_FALSE);
    }

    /* Depth func (bits 9-11). */
    if (delta & RB_DEPTH_FUNC_MASK) {
        GLenum func = (bits & RB_DEPTH_FUNC_MASK) >> 9;
        R_Call(glDepthFunc, func);
    }

    /* Depth test enable: derived from depth func != GL_ALWAYS. */
    bool wantDepth = ((bits & RB_DEPTH_FUNC_MASK) >> 9) != GL_ALWAYS;
    bool haveDepth = ((backEnd.glStateBits & RB_DEPTH_FUNC_MASK) >> 9) != GL_ALWAYS;
    if (wantDepth != haveDepth) {
        R_Call(wantDepth ? glEnable : glDisable, GL_DEPTH_TEST);
    }

    /* Color mask (bits 12-15). */
    if (delta & RB_COLOR_MASK_MASK) {
        DWORD cm = (bits & RB_COLOR_MASK_MASK) >> RB_COLOR_MASK_SHIFT;
        R_Call(glColorMask, (cm >> 3) & 1, (cm >> 2) & 1, (cm >> 1) & 1, cm & 1);
    }

    backEnd.glStateBits = bits;
}

void RB_Cull(GLenum mode) {
    if (mode == backEnd.faceCulling) return;
    if (mode) {
        R_Call(glEnable, GL_CULL_FACE);
        R_Call(glCullFace, mode);
    } else {
        R_Call(glDisable, GL_CULL_FACE);
    }
    backEnd.faceCulling = mode;
}

void RB_PolygonOffset(float scale, float bias) {
    DWORD s = *(DWORD *)&scale, b = *(DWORD *)&bias;
    if (s == backEnd.polygonOffsetScale && b == backEnd.polygonOffsetBias) return;
    if (scale != 0.0f || bias != 0.0f) {
        R_Call(glEnable, GL_POLYGON_OFFSET_FILL);
        R_Call(glPolygonOffset, scale, bias);
    } else {
        R_Call(glDisable, GL_POLYGON_OFFSET_FILL);
    }
    backEnd.polygonOffsetScale = s;
    backEnd.polygonOffsetBias = b;
}

void RB_BlendEquation(GLenum eq) {
    if (eq == backEnd.blendEquation) return;
    R_Call(glBlendEquation, eq);
    backEnd.blendEquation = eq;
}

void RB_Scissor(LPCRECT r) {
    backEnd.scissorEnabled = true;
    R_Call(glEnable, GL_SCISSOR_TEST);
    if (r->x == backEnd.currentScissor.x && r->y == backEnd.currentScissor.y &&
        r->w == backEnd.currentScissor.w && r->h == backEnd.currentScissor.h) return;
    backEnd.currentScissor = *r;
    R_Call(glScissor, r->x, r->y, r->w, r->h);
}

void RB_ScissorDisable(void) {
    if (!backEnd.scissorEnabled) return;
    R_Call(glDisable, GL_SCISSOR_TEST);
    backEnd.scissorEnabled = false;
}

void RB_Viewport(LPCRECT r) {
    if (r->x == backEnd.currentViewport.x && r->y == backEnd.currentViewport.y &&
        r->w == backEnd.currentViewport.w && r->h == backEnd.currentViewport.h) return;
    backEnd.currentViewport = *r;
    R_Call(glViewport, r->x, r->y, r->w, r->h);
}

void RB_BindVAO(DWORD vao) {
    if (vao == backEnd.currentVAO) return;
    R_Call(glBindVertexArray, vao);
    backEnd.currentVAO = vao;
}

void RB_BindFBO(DWORD fbo) {
    if (fbo == backEnd.currentFBO) return;
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, fbo);
    backEnd.currentFBO = fbo;
}

void RB_ColorMask(BOOL r, BOOL g, BOOL b, BOOL a) {
    DWORD cm = ((DWORD)r << 3) | ((DWORD)g << 2) | ((DWORD)b << 1) | (DWORD)a;
    DWORD bits = (backEnd.glStateBits & ~RB_COLOR_MASK_MASK) | (cm << RB_COLOR_MASK_SHIFT);
    if (bits != backEnd.glStateBits) {
        RB_State(bits);
    }
}

void RB_ResetState(void) {
    memset(&backEnd, 0, sizeof(backEnd));
    backEnd.faceCulling = GL_BACK;
    backEnd.depthFunc = GL_LEQUAL;
    backEnd.blendEquation = GL_FUNC_ADD;
    backEnd.glStateBits = 0;
    backEnd.scissorEnabled = false;
}

void RB_Init(void) {
    RB_ResetState();
}

/* -----------------------------------------------------------------------
 * Pipeline objects
 * ----------------------------------------------------------------------- */

static DWORD pipeline_next_id;

pipeline_t RB_CreatePipeline(pipelineDesc_t *desc) {
    pipeline_t p = {
        .progid = glCreateProgram(),
        .raster = desc->raster,
        .id = ++pipeline_next_id,
    };
    if (desc->vertShader) R_Call(glAttachShader, p.progid, desc->vertShader);
    if (desc->fragShader) R_Call(glAttachShader, p.progid, desc->fragShader);
    R_Call(glLinkProgram, p.progid);
    GLint ok;
    glGetProgramiv(p.progid, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p.progid, sizeof(log), NULL, log);
        fprintf(stderr, "RB_CreatePipeline: link failed: %s\n", log);
    }
    return p;
}

void RB_BindPipeline(pipeline_t *p) {
    if (!p) return;
    R_Call(glUseProgram, p->progid);
    RB_State((p->raster.blendSrc << 0)
           | (p->raster.blendDst << 4)
           | (p->raster.depthWrite ? RB_DEPTH_WRITE_BIT : 0)
           | (p->raster.depthFunc << 9)
           | ((p->raster.colorMask & 0xF) << RB_COLOR_MASK_SHIFT));
    RB_Cull(p->raster.cullFace);
}

pipeline_t RB_MakePipeline(SHADERPROG *prog, pipelineRasterState_t *raster) {
    pipeline_t p = {
        .progid = prog->progid,
        .raster = *raster,
        .id = ++pipeline_next_id,
    };
    return p;
}

/* -----------------------------------------------------------------------
 * Root struct UBO
 * ----------------------------------------------------------------------- */

/* Per-program UBO state: one UBO per shader program, lazily created. */
typedef struct rootUBO {
    DWORD uboid;        /* GL buffer name */
    DWORD size;         /* allocated size in bytes */
    DWORD version;      /* monotonic; skip upload if unchanged */
} rootUBO_t;

#define RB_MAX_ROOT_UBOS 32
static rootUBO_t rootUBOs[RB_MAX_ROOT_UBOS];
static DWORD rootUBO_count;

/* Create or resize a UBO for a program.  Binding happens in RB_BindPipeline. */
static rootUBO_t *RB_EnsureRootUBO(GLuint progid, DWORD neededSize) {
    /* Simple linear search; programs are few. */
    for (DWORD i = 0; i < rootUBO_count; i++) {
        if (rootUBOs[i].uboid && rootUBOs[i].size >= neededSize)
            return &rootUBOs[i];
    }
    if (rootUBO_count >= RB_MAX_ROOT_UBOS) return NULL;
    rootUBO_t *ru = &rootUBOs[rootUBO_count++];
    if (!ru->uboid) {
        GLuint buf;
        glGenBuffers(1, &buf);
        ru->uboid = buf;
    }
    R_Call(glBindBuffer, GL_UNIFORM_BUFFER, ru->uboid);
    R_Call(glBufferData, GL_UNIFORM_BUFFER, neededSize, NULL, GL_DYNAMIC_DRAW);
    ru->size = neededSize;
    ru->version = 0;
    return ru;
}

bool RB_UploadRoot(SHADERPROG *prog, LPCVOID state, DWORD stateSize) {
    if (!prog || !state || stateSize == 0) return false;
    rootUBO_t *ru = RB_EnsureRootUBO(prog->progid, stateSize);
    if (!ru) return false;
    R_Call(glBindBuffer, GL_UNIFORM_BUFFER, ru->uboid);
    R_Call(glBufferSubData, GL_UNIFORM_BUFFER, 0, stateSize, state);
    R_Call(glBindBufferBase, GL_UNIFORM_BUFFER, RB_UBO_BINDING_POINT, ru->uboid);
    ru->version++;
    return true;
}
