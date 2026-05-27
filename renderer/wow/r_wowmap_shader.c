#include "r_wowmap.h"

LPSHADER wow_terrain_shader;
GLint wow_uTexture0 = -1;
GLint wow_uTexture1 = -1;
GLint wow_uTexture2 = -1;
GLint wow_uTexture3 = -1;
GLint wow_uAlphaTexture = -1;
GLint wow_uUseWeightedBlend = -1;
GLint wow_uAlphaOrigin = -1;
GLint wow_uAlphaAtlasChunks = -1;

void Wow_InitTerrainShader(void) {
    static LPCSTR vs_wow_terrain =
    "#version 140\n"
    "in vec3 i_position;\n"
    "in vec3 i_normal;\n"
    "in vec2 i_texcoord;\n"
    "in vec4 i_color;\n"
    "out vec2 v_texcoord;\n"
    "out vec3 v_normal;\n"
    "out vec4 v_color;\n"
    "out vec3 v_lightDir;\n"
    "uniform mat4 uViewProjectionMatrix;\n"
    "uniform mat4 uModelMatrix;\n"
    "uniform mat4 uLightMatrix;\n"
    "uniform mat3 uNormalMatrix;\n"
    "void main() {\n"
    "    vec4 pos = uModelMatrix * vec4(i_position, 1.0);\n"
    "    v_texcoord = i_texcoord;\n"
    "    v_normal = normalize(uNormalMatrix * i_normal);\n"
    "    v_color = i_color;\n"
    "    v_lightDir = -normalize(vec3(uLightMatrix[0][2], uLightMatrix[1][2], uLightMatrix[2][2])) * 1.2;\n"
    "    gl_Position = uViewProjectionMatrix * pos;\n"
    "}\n";
    static LPCSTR fs_wow_terrain =
    "#version 140\n"
    "in vec2 v_texcoord;\n"
    "in vec3 v_normal;\n"
    "in vec4 v_color;\n"
    "in vec3 v_lightDir;\n"
    "out vec4 o_color;\n"
    "uniform sampler2D uTexture0;\n"
    "uniform sampler2D uTexture1;\n"
    "uniform sampler2D uTexture2;\n"
    "uniform sampler2D uTexture3;\n"
    "uniform sampler2D uAlphaTexture;\n"
    "uniform int uUseWeightedBlend;\n"
    "uniform vec2 uAlphaOrigin;\n"
    "uniform float uAlphaAtlasChunks;\n"
    "vec2 adtAlphaCoord(vec2 chunkCoord) {\n"
    "    const float alphaTexelsPerChunk = 64.0;\n"
    "    float alphaAtlasSize = alphaTexelsPerChunk * uAlphaAtlasChunks;\n"
    "    chunkCoord = clamp(chunkCoord, vec2(0.0), vec2(1.0));\n"
    "    vec2 atlasTexel = uAlphaOrigin * alphaTexelsPerChunk + chunkCoord * (alphaTexelsPerChunk - 1.0) + vec2(0.5);\n"
    "    return atlasTexel / alphaAtlasSize;\n"
    "}\n"
    "float get_lighting() {\n"
    "    float light = clamp(dot(v_normal, v_lightDir), 0.0, 1.0);\n"
    "    return mix(0.5, 1.0, light);\n"
    "}\n"
    "void main() {\n"
    "    vec2 alphaCoord = adtAlphaCoord(v_texcoord * 0.125);\n"
    "    vec3 alphaBlend = texture(uAlphaTexture, alphaCoord).gba;\n"
    "    vec4 tex1 = texture(uTexture0, v_texcoord);\n"
    "    vec4 tex2 = texture(uTexture1, v_texcoord);\n"
    "    vec4 tex3 = texture(uTexture2, v_texcoord);\n"
    "    vec4 tex4 = texture(uTexture3, v_texcoord);\n"
    "    vec4 color;\n"
    "    if (uUseWeightedBlend != 0) {\n"
    "        float baseWeight = 1.0 - clamp(dot(alphaBlend, vec3(1.0)), 0.0, 1.0);\n"
    "        vec4 weights = vec4(baseWeight, alphaBlend);\n"
    "        color = tex1 * weights.r + tex2 * weights.g + tex3 * weights.b + tex4 * weights.a;\n"
    "    } else {\n"
    "        color = mix(mix(mix(tex1, tex2, alphaBlend.r), tex3, alphaBlend.g), tex4, alphaBlend.b);\n"
    "    }\n"
    "    color.rgb *= 2.0 * v_color.rgb * get_lighting();\n"
    "    color.a = 1.0;\n"
    "    o_color = color;\n"
    "}\n";

    if (wow_terrain_shader) {
        return;
    }

    wow_terrain_shader = R_InitShader(vs_wow_terrain, fs_wow_terrain);
    if (!wow_terrain_shader) {
        return;
    }

    wow_uTexture0 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture0");
    wow_uTexture1 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture1");
    wow_uTexture2 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture2");
    wow_uTexture3 = glGetUniformLocation(wow_terrain_shader->progid, "uTexture3");
    wow_uAlphaTexture = glGetUniformLocation(wow_terrain_shader->progid, "uAlphaTexture");
    wow_uUseWeightedBlend = glGetUniformLocation(wow_terrain_shader->progid, "uUseWeightedBlend");
    wow_uAlphaOrigin = glGetUniformLocation(wow_terrain_shader->progid, "uAlphaOrigin");
    wow_uAlphaAtlasChunks = glGetUniformLocation(wow_terrain_shader->progid, "uAlphaAtlasChunks");
    R_Call(glUseProgram, wow_terrain_shader->progid);
    R_Call(glUniform1i, wow_uTexture0, 0);
    R_Call(glUniform1i, wow_uTexture1, 1);
    R_Call(glUniform1i, wow_uTexture2, 2);
    R_Call(glUniform1i, wow_uTexture3, 3);
    R_Call(glUniform1i, wow_uAlphaTexture, 4);
    R_Call(glUniform1f, wow_uAlphaAtlasChunks, (GLfloat)WOW_ALPHA_ATLAS_CHUNKS);
}
