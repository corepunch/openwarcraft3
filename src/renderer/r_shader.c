#include "r_local.h"

LPCSTR vertex_shader =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"in vec2 i_texcoord2;\n"
"in vec3 i_normal;\n"
"out vec4 v_color;\n"
"out vec3 v_position;\n"
"out vec4 v_shadow;\n"
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"uniform mat4 uProjectionMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"void main() {\n"
"    v_color = i_color;\n"
"    v_position = i_position;\n"
"    v_texcoord = i_texcoord;\n"
"    v_texcoord2 = i_texcoord2;\n"
"    v_normal = normalize(uNormalMatrix * i_normal);\n"
"    v_shadow = uLightMatrix * uModelMatrix * vec4(i_position, 1.0);\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[2][0], uLightMatrix[2][1], uLightMatrix[2][2]));\n"
"    gl_Position = uProjectionMatrix * uModelMatrix * vec4(i_position, 1.0);\n"
"}\n";

LPCSTR vertex_shader_skin =
"#version 140\n"
"in vec3 i_position;\n"
"in vec4 i_color;\n"
"in vec2 i_texcoord;\n"
"in vec2 i_texcoord2;\n"
"in vec3 i_normal;\n"
"in vec4 i_skin;\n"
"in vec4 i_boneWeight;\n"
"out vec4 v_color;\n"
"out vec3 v_position;\n"
"out vec4 v_shadow;\n"
"out vec2 v_texcoord;\n"
"out vec2 v_texcoord2;\n"
"out vec3 v_normal;\n"
"out vec3 v_lightDir;\n"
"uniform mat4 uBones[64];\n"
"uniform mat4 uProjectionMatrix;\n"
"uniform mat4 uModelMatrix;\n"
"uniform mat4 uLightMatrix;\n"
"uniform mat3 uNormalMatrix;\n"
"void main() {\n"
"    vec4 sum = uBones[int(i_skin[0])] * vec4(i_position, 1.0) * i_boneWeight[0];\n"
"    sum += uBones[int(i_skin[1])] * vec4(i_position, 1.0) * i_boneWeight[1];\n"
"    sum += uBones[int(i_skin[2])] * vec4(i_position, 1.0) * i_boneWeight[2];\n"
"    sum += uBones[int(i_skin[3])] * vec4(i_position, 1.0) * i_boneWeight[3];\n"
"    sum.w = 1.0;\n"
"    v_color = i_color;\n"
"    v_position = sum.xyz;\n"
"    v_texcoord = i_texcoord;\n"
"    v_texcoord2 = i_texcoord2;\n"
"    v_normal = normalize(uNormalMatrix * i_normal);\n"
"    v_shadow = uLightMatrix * uModelMatrix * sum;\n"
"    v_lightDir = -normalize(vec3(uLightMatrix[2][0], uLightMatrix[2][1], uLightMatrix[2][2]));\n"
"    gl_Position = uProjectionMatrix * uModelMatrix * sum;\n"
"}\n";

LPCSTR fragment_shader =
"#version 140\n"
"in vec4 v_color;\n"
"in vec3 v_position;\n"
"in vec2 v_texcoord;\n"
"in vec2 v_texcoord2;\n"
"in vec4 v_shadow;\n"
"in vec3 v_normal;\n"
"in vec3 v_lightDir;\n"
"out vec4 o_color;\n"
"uniform sampler2D uTexture;\n"
"uniform sampler2D uShadowmap;\n"
"uniform bool uUseDiscard;\n"
"void main() {\n"
"    //vec4 shadow = mix(vec4(1), texture(uShadowmap, v_texcoord2), 0.35);\n"
"    float depth = texture(uShadowmap, vec2(v_shadow.x + 1.0, v_shadow.y + 1.0) * 0.5).r;\n"
"    float shade = depth < (v_shadow.z + 0.99) * 0.5 ? 0.0 : 1.0;\n"
"    vec4 col = texture(uTexture, v_texcoord);\n"
"    shade *= max(0.0, dot(v_normal, v_lightDir));\n"
"    col.rgb *= mix(vec3(0.3), vec3(1.0), vec3(shade));\n"
"    o_color = col * v_color;\n"
"    if (o_color.a < 0.5 && uUseDiscard) discard;\n"
"}\n";

LPCSHADER R_InitShader(LPCSTR vertex_shader, LPCSTR fragment_shader){
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    int length = (int)strlen(vertex_shader);
    glShaderSource(vs, 1, (const GLchar **)&vertex_shader, &length);
    glCompileShader(vs);

    GLint status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        fprintf(stderr, "vertex shader compilation failed\n");
        return NULL;
    }

    length = (int)strlen(fragment_shader);
    glShaderSource(fs, 1, (const GLchar **)&fragment_shader, &length);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        fprintf(stderr, "fragment shader compilation failed\n");
        return NULL;
    }
    
    LPSHADER program = ri.MemAlloc(sizeof(struct shader_program));
    program->progid = glCreateProgram();

    glAttachShader(program->progid, vs);
    glAttachShader(program->progid, fs);

    glBindAttribLocation(program->progid, attrib_position, "i_position");
    glBindAttribLocation(program->progid, attrib_color, "i_color");
    glBindAttribLocation(program->progid, attrib_texcoord, "i_texcoord");
    glBindAttribLocation(program->progid, attrib_texcoord2, "i_texcoord2");
    glBindAttribLocation(program->progid, attrib_normal, "i_normal");
    glBindAttribLocation(program->progid, attrib_skin, "i_skin");
    glBindAttribLocation(program->progid, attrib_boneWeight, "i_boneWeight");

    glLinkProgram(program->progid);
    glUseProgram(program->progid);
    
    program->uProjectionMatrix = glGetUniformLocation(program->progid, "uProjectionMatrix");
    program->uModelMatrix = glGetUniformLocation(program->progid, "uModelMatrix");
    program->uLightMatrix = glGetUniformLocation(program->progid, "uLightMatrix");
    program->uNormalMatrix = glGetUniformLocation(program->progid, "uNormalMatrix");
    program->uTexture = glGetUniformLocation(program->progid, "uTexture");
    program->uShadowmap = glGetUniformLocation(program->progid, "uShadowmap");
    program->uBones = glGetUniformLocation(program->progid, "uBones");
    program->uUseDiscard = glGetUniformLocation(program->progid, "uUseDiscard");
    
    glUniform1i(program->uTexture, 0);
    glUniform1i(program->uShadowmap, 1);

    return program;
}
