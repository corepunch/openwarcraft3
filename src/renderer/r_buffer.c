#include "r_local.h"

VERTEX *R_AddQuad(VERTEX *buffer, LPCRECT screen, LPCRECT uv, COLOR32 color) {
    VERTEX const data[] = {
        {
            .position = { screen->x, screen->y, 0 },
            .texcoord = { uv->x, uv->y },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y, 0 },
            .texcoord = { uv->x+uv->width, uv->y },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y+screen->height, 0 },
            .texcoord = { uv->x+uv->width, uv->y+uv->height },
            .color = color,
        },
        {
            .position = { screen->x, screen->y, 0 },
            .texcoord = { uv->x, uv->y },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y+screen->height, 0 },
            .texcoord = { uv->x+uv->width, uv->y+uv->height },
            .color = color,
        },
        {
            .position = { screen->x, screen->y+screen->height, 0 },
            .texcoord = { uv->x, uv->y+uv->height },
            .color = color,
        },
    };
    memcpy(buffer, data, sizeof(data));
    return buffer + 6;
}

VERTEX *R_AddStrip(VERTEX *buffer, LPCRECT screen, COLOR32 color) {
    VERTEX const data[] = {
        {
            .position = { screen->x, screen->y, 0 },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y, 0 },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y+screen->height, 0 },
            .color = color,
        },
        {
            .position = { screen->x, screen->y+screen->height, 0 },
            .color = color,
        },
        {
            .position = { screen->x, screen->y, 0 },
            .color = color,
        },
    };
    memcpy(buffer, data, sizeof(data));
    return buffer + 5;
}

LPBUFFER R_MakeVertexArrayObject(LPCVERTEX vertices, DWORD size) {
    LPBUFFER buf = ri.MemAlloc(sizeof(BUFFER));

    R_Call(glGenVertexArrays, 1, &buf->vao);
    R_Call(glGenBuffers, 1, &buf->vbo);
    R_Call(glBindVertexArray, buf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buf->vbo);

    R_Call(glEnableVertexAttribArray, attrib_position);
    R_Call(glEnableVertexAttribArray, attrib_color);
    R_Call(glEnableVertexAttribArray, attrib_texcoord);
    R_Call(glEnableVertexAttribArray, attrib_texcoord2);
    R_Call(glEnableVertexAttribArray, attrib_skin1);
    R_Call(glEnableVertexAttribArray, attrib_skin2);
    R_Call(glEnableVertexAttribArray, attrib_boneWeight1);
    R_Call(glEnableVertexAttribArray, attrib_boneWeight2);
    R_Call(glEnableVertexAttribArray, attrib_normal);

    R_Call(glVertexAttribPointer, attrib_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, color));
    R_Call(glVertexAttribPointer, attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, position));
    R_Call(glVertexAttribPointer, attrib_texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord));
    R_Call(glVertexAttribPointer, attrib_texcoord2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord2));
    R_Call(glVertexAttribPointer, attrib_skin1, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vertex), FOFS(vertex, skin[0]));
    R_Call(glVertexAttribPointer, attrib_skin2, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vertex), FOFS(vertex, skin[4]));
    R_Call(glVertexAttribPointer, attrib_boneWeight1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, boneWeight[0]));
    R_Call(glVertexAttribPointer, attrib_boneWeight2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, boneWeight[4]));
    R_Call(glVertexAttribPointer, attrib_normal, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, normal));

    if (vertices) {
        R_Call(glBufferData, GL_ARRAY_BUFFER, size * sizeof(VERTEX), vertices, GL_STATIC_DRAW);
    }

    return buf;
}

void R_ReleaseVertexArrayObject(LPBUFFER buffer) {
    R_Call(glDeleteBuffers, 1, &buffer->vbo);
    R_Call(glDeleteVertexArrays, 1, &buffer->vao);
}

