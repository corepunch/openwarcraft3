#include "r_local.h"

LPCSTR vs_default =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"in vec3 i_normal;\n"
"out vec4 v_color;\n"
"out vec4 v_shadow;\n"
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"uniform mat4 uViewProjectionMatrix;\n"
"uniform mat4 uTextureMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"void main() {\n"
"    vec4 pos = uModelMatrix * vec4(i_position, 1.0);"
"    v_color = i_color;\n"
"    v_texcoord = i_texcoord;\n"
"    v_texcoord2 = (uTextureMatrix * pos).xy;\n"
"    v_normal = normalize(uNormalMatrix * i_normal);\n"
"    v_shadow = uLightMatrix * pos;\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[0][2], uLightMatrix[1][2], uLightMatrix[2][2]))*1.2;\n"
"    gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(i_position, 1.0);\n"
"}\n";

LPCSTR vs_skin =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"in vec3 i_normal;\n"
"in vec4 i_skin1;\n"
"in vec4 i_boneWeight1;\n"
#if MAX_SKIN_BONES > 4
"in vec4 i_boneWeight2;\n"
"in vec4 i_skin2;\n"
#endif
"out vec4 v_color;\n"
"out vec4 v_shadow;\n"
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"uniform mat4 uBones[64];\n"
"uniform mat4 uViewProjectionMatrix;\n"
"uniform mat4 uTextureMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"void main() {\n"
"    vec4 pos4 = vec4(i_position, 1.0);\n"
"    vec4 norm4 = vec4(i_normal, 0.0);\n"
"    vec4 position = vec4(0.0);\n"
"    vec4 normal = vec4(0.0);\n"
"    position += uBones[int(i_skin1[0])] * pos4 * i_boneWeight1[0];\n"
"    position += uBones[int(i_skin1[1])] * pos4 * i_boneWeight1[1];\n"
"    position += uBones[int(i_skin1[2])] * pos4 * i_boneWeight1[2];\n"
"    position += uBones[int(i_skin1[3])] * pos4 * i_boneWeight1[3];\n"
"    normal += uBones[int(i_skin1[0])] * norm4 * i_boneWeight1[0];\n"
"    normal += uBones[int(i_skin1[1])] * norm4 * i_boneWeight1[1];\n"
"    normal += uBones[int(i_skin1[2])] * norm4 * i_boneWeight1[2];\n"
"    normal += uBones[int(i_skin1[3])] * norm4 * i_boneWeight1[3];\n"
#if MAX_SKIN_BONES > 4
"    position += uBones[int(i_skin2[0])] * pos4 * i_boneWeight2[0];\n"
"    position += uBones[int(i_skin2[1])] * pos4 * i_boneWeight2[1];\n"
"    position += uBones[int(i_skin2[2])] * pos4 * i_boneWeight2[2];\n"
"    position += uBones[int(i_skin2[3])] * pos4 * i_boneWeight2[3];\n"
"    normal += uBones[int(i_skin2[0])] * norm4 * i_boneWeight2[0];\n"
"    normal += uBones[int(i_skin2[1])] * norm4 * i_boneWeight2[1];\n"
"    normal += uBones[int(i_skin2[2])] * norm4 * i_boneWeight2[2];\n"
"    normal += uBones[int(i_skin2[3])] * norm4 * i_boneWeight2[3];\n"
#endif
"    position.w = 1.0;\n"
"    v_color = i_color;\n"
"    v_texcoord = i_texcoord;\n"
"    v_texcoord2 = (uTextureMatrix * uModelMatrix * position).xy;\n"
"    v_normal = normalize(uNormalMatrix * normal.xyz);\n"
"    v_shadow = uLightMatrix * uModelMatrix * position;\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[0][2], uLightMatrix[1][2], uLightMatrix[2][2]))*1.2;\n"
"    gl_Position = uViewProjectionMatrix * uModelMatrix * position;\n"
"}\n";

LPCSTR fs_default =
"#version 140\n"
"in vec4 v_color;\n"
"in vec2 v_texcoord;\n"
"in vec2 v_texcoord2;\n"
"in vec4 v_shadow;\n"
"in vec3 v_normal;\n"
"in vec3 v_lightDir;\n"
"out vec4 o_color;\n"
"uniform sampler2D uTexture;\n"
"uniform sampler2D uShadowmap;\n"
"uniform sampler2D uFogOfWar;\n"
"uniform bool uUseDiscard;\n"
"float get_light() {\n"
"    return dot(v_normal, v_lightDir);\n"
"}\n"
"float get_shadow() {\n"
"    float depth = texture(uShadowmap, vec2(v_shadow.x + 1.0, v_shadow.y + 1.0) * 0.5).r;\n"
"    return depth < (v_shadow.z + 0.99) * 0.5 ? 0.0 : 1.0;\n"
"}\n"
"float get_lighting() {\n"
"    return min(1.0, mix(0.35, 1.0, get_shadow() * get_light()) * 1.1);"
"}\n"
"float get_fogofwar() {\n"
"    return texture(uFogOfWar, v_texcoord2).r;\n"
"}\n"
"void main() {\n"
#ifdef DEBUG_PATHFINDING
"    vec4 debug = texture(uShadowmap, v_texcoord2);\n"
"    vec4 color = texture(uTexture, v_texcoord);\n"
"    float sin_factor = sin(debug.g * 3.14159 * 2.0);\n"
"    float cos_factor = cos(debug.g * 3.14159 * 2.0);\n"
"    vec2 tc = fract(v_texcoord2 * 384.0);\n"
"    tc = (tc - 0.5) * mat2(cos_factor, sin_factor, -sin_factor, cos_factor);\n"
"    tc += 0.5;\n"
"    float stp = step(abs(0.5 - tc.y), tc.x * 0.25);"
"    /*debug.a = color.a*/;\n"
"    o_color = debug * 0.7;// mix(debug, color, 0.5) + vec4(stp);\n"
"    return;\n"
#endif
"    vec4 col = texture(uTexture, v_texcoord);\n"
"    col.rgb *= get_fogofwar() * get_lighting();\n"
"    o_color = col * v_color;\n"
"    if (o_color.a < 0.5 && uUseDiscard) discard;\n"
"}\n";

LPCSTR fs_ui =
"#version 140\n"
"in vec4 v_color;\n"
"in vec2 v_texcoord;\n"
"out vec4 o_color;\n"
"uniform sampler2D uTexture;\n"
"float crop_edges(vec2 tc) {\n"
"   return step(abs(tc.x - 0.5), 0.5) * step(abs(tc.y - 0.5), 0.5);\n"
"}\n"
"void main() {\n"
"    o_color = texture(uTexture, v_texcoord) * v_color;\n"
"    o_color.a *= crop_edges(v_texcoord);\n"
"}\n";

LPSHADER R_InitShader(LPCSTR vs_default, LPCSTR fs_default){
    GLuint vs = R_Call(glCreateShader, GL_VERTEX_SHADER);
    GLuint fs = R_Call(glCreateShader, GL_FRAGMENT_SHADER);

    int length = (int)strlen(vs_default);
    R_Call(glShaderSource, vs, 1, (const GLchar **)&vs_default, &length);
    R_Call(glCompileShader, vs);

    GLint status;
    R_Call(glGetShaderiv, vs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        fprintf(stderr, "vertex shader compilation failed\n");
        return NULL;
    }

    length = (int)strlen(fs_default);
    R_Call(glShaderSource, fs, 1, (const GLchar **)&fs_default, &length);
    R_Call(glCompileShader, fs);

    R_Call(glGetShaderiv, fs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        fprintf(stderr, "fragment shader compilation failed\n");
        return NULL;
    }
    
    LPSHADER program = ri.MemAlloc(sizeof(struct shader_program));
    program->progid = R_Call(glCreateProgram, );

    R_Call(glAttachShader, program->progid, vs);
    R_Call(glAttachShader, program->progid, fs);

    R_Call(glBindAttribLocation, program->progid, attrib_position, "i_position");
    R_Call(glBindAttribLocation, program->progid, attrib_color, "i_color");
    R_Call(glBindAttribLocation, program->progid, attrib_texcoord, "i_texcoord");
    R_Call(glBindAttribLocation, program->progid, attrib_normal, "i_normal");
    R_Call(glBindAttribLocation, program->progid, attrib_skin1, "i_skin1");
    R_Call(glBindAttribLocation, program->progid, attrib_skin2, "i_skin2");
    R_Call(glBindAttribLocation, program->progid, attrib_boneWeight1, "i_boneWeight1");
    R_Call(glBindAttribLocation, program->progid, attrib_boneWeight2, "i_boneWeight2");
    R_Call(glBindAttribLocation, program->progid, attrib_particleSize, "i_size");
    R_Call(glBindAttribLocation, program->progid, attrib_particleAxis, "i_axis");

    R_Call(glLinkProgram, program->progid);
    R_Call(glUseProgram, program->progid);
    
    program->uViewProjectionMatrix = R_Call(glGetUniformLocation, program->progid, "uViewProjectionMatrix");
    program->uModelMatrix = R_Call(glGetUniformLocation, program->progid, "uModelMatrix");
    program->uLightMatrix = R_Call(glGetUniformLocation, program->progid, "uLightMatrix");
    program->uNormalMatrix = R_Call(glGetUniformLocation, program->progid, "uNormalMatrix");
    program->uTextureMatrix = R_Call(glGetUniformLocation, program->progid, "uTextureMatrix");
    program->uTexture = R_Call(glGetUniformLocation, program->progid, "uTexture");
    program->uShadowmap = R_Call(glGetUniformLocation, program->progid, "uShadowmap");
    program->uFogOfWar = R_Call(glGetUniformLocation, program->progid, "uFogOfWar");
    program->uBones = R_Call(glGetUniformLocation, program->progid, "uBones");
    program->uUseDiscard = R_Call(glGetUniformLocation, program->progid, "uUseDiscard");
    program->uEyePosition = R_Call(glGetUniformLocation, program->progid, "uEyePosition");
    
    R_Call(glUniform1i, program->uTexture, 0);
    R_Call(glUniform1i, program->uShadowmap, 1);
    R_Call(glUniform1i, program->uFogOfWar, 2);

    return program;
}

void R_ReleaseShader(LPSHADER shader) {
    ri.MemFree(shader);
}
