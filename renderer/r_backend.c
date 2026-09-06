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
