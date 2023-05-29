#include "r_local.h"

void R_PrintText(LPCSTR string, DWORD x, DWORD y, COLOR32 color) {
    static VERTEX simp[256 * 6];
    LPVERTEX it = simp;
    for (LPCSTR s = string; *s; s++) {
        DWORD ch = *s;
        float fx = ch % 16;
        float fy = ch / 16;
        it = R_AddQuad(it, &(RECT ) {
            x + 10 * (s - string), y, 8, 16
        }, &(RECT ) {
            fx/16,fy/8,1.f/16,1.f/8
        }, color);
    }

    DWORD num_vertices = (DWORD)(it - simp);
    SIZE2 window = R_GetWindowSize();
    MATRIX4 ui_matrix;
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, window.height, 0.0f, 0.0f, 100.0f);

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * num_vertices, simp, GL_STATIC_DRAW);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, ui_matrix.v);

    R_BindTexture(tr.sysFont, 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, num_vertices);
}

void R_DrawImage(LPCTEXTURE texture, LPCRECT screen, LPCRECT uv) {
    VERTEX simp[6];
    R_AddQuad(simp, screen, uv, (COLOR32){255,255,255,255});

    MATRIX4 ui_matrix;
    Matrix4_ortho(&ui_matrix, 0.0f, 800, 600, 0.0f, 0.0f, 100.0f);
    
    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);

    R_BindTexture(texture, 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);
}

void R_DrawPic(LPCTEXTURE texture, DWORD x, DWORD y) {
    R_DrawImage(texture,
                &(RECT ) { x, y, texture->width, texture->height },
                &(RECT const) { 0, 0, 1, 1 });
}

void R_DrawSelectionRect(LPCRECT rect, COLOR32 color) {
    static VERTEX simp[5];
    R_AddStrip(simp, rect, color);

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 5, simp, GL_STATIC_DRAW);

    R_BindTexture(tr.whiteTexture, 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_LINE_STRIP, 0, 5);
}
