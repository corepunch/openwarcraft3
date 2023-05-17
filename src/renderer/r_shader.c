#include "r_local.h"

LPCSTR vertex_shader =
    "#version 140\n"
    "in vec3 i_position;\n"
    "in vec4 i_color;\n"
    "in vec2 i_texcoord;\n"
    "in vec2 i_texcoord2;\n"
    "out vec4 v_color;\n"
    "out vec3 v_position;\n"
    "out vec2 v_texcoord;\n"
    "out vec2 v_texcoord2;\n"
    "uniform mat4 u_projection_matrix;\n"
    "uniform mat4 u_model_matrix;\n"
    "void main() {\n"
    "    v_color = i_color;\n"
    "    v_position = i_position;\n"
    "    v_texcoord = i_texcoord;\n"
    "    v_texcoord2 = i_texcoord2;\n"
    "    gl_Position = u_projection_matrix * u_model_matrix * vec4(i_position, 1.0);\n"
    "}\n";

LPCSTR vertex_shader_skin =
    "#version 140\n"
    "in vec3 i_position;\n"
    "in vec4 i_color;\n"
    "in vec2 i_texcoord;\n"
    "in vec2 i_texcoord2;\n"
    "in vec4 i_skin;\n"
    "out vec4 v_color;\n"
    "out vec3 v_position;\n"
    "out vec2 v_texcoord;\n"
    "out vec2 v_texcoord2;\n"
    "uniform mat4 u_nodes_matrices[64];\n"
    "uniform mat4 u_projection_matrix;\n"
    "uniform mat4 u_model_matrix;\n"
    "void main() {\n"
    "    int count = 1;\n"
    "    vec4 sum = u_nodes_matrices[int(i_skin[0])] * vec4(i_position, 1.0);\n"
    "    if (i_skin[1] > 0.0) {\n"
    "        sum += u_nodes_matrices[int(i_skin[1])] * vec4(i_position, 1.0);\n"
    "        count += 1;\n"
    "    }\n"
    "    if (i_skin[2] > 0.0) {\n"
    "        sum += u_nodes_matrices[int(i_skin[2])] * vec4(i_position, 1.0);\n"
    "        count += 1;\n"
    "    }\n"
    "    if (i_skin[3] > 0.0) {\n"
    "        sum += u_nodes_matrices[int(i_skin[3])] * vec4(i_position, 1.0);\n"
    "        count += 1;\n"
    "    }\n"
    "    sum.xyz /= float(count);\n"
    "    sum.w = 1.0;\n"
    "    v_color = i_color;\n"
    "    v_position = sum.xyz;\n"
    "    v_texcoord = i_texcoord;\n"
    "    v_texcoord2 = i_texcoord2;\n"
    "    gl_Position = u_projection_matrix * u_model_matrix * sum;\n"
    "}\n";

LPCSTR fragment_shader =
    "#version 140\n"
    "in vec4 v_color;\n"
    "in vec3 v_position;\n"
    "in vec2 v_texcoord;\n"
    "in vec2 v_texcoord2;\n"
    "out vec4 o_color;\n"
    "uniform sampler2D u_texture;\n"
    "uniform sampler2D u_shadowmap;\n"
    "void main() {\n"
    "    vec4 shadow = mix(vec4(1), texture(u_shadowmap, v_texcoord2), 0.35);\n"
    "    o_color = /*vec4(1,1,1,1);*/ shadow * texture(u_texture, v_texcoord) * v_color;\n"
    "    /*if (o_color.a < 0.05) discard;*/\n"
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
    glBindAttribLocation(program->progid, attrib_skin, "i_skin");
    glLinkProgram(program->progid);
    glUseProgram(program->progid);
    
    program->u_projection_matrix = glGetUniformLocation(program->progid, "u_projection_matrix");
    program->u_model_matrix = glGetUniformLocation(program->progid, "u_model_matrix");
    program->u_texture = glGetUniformLocation(program->progid, "u_texture");
    program->u_shadowmap = glGetUniformLocation(program->progid, "u_shadowmap");
    program->u_nodes_matrices = glGetUniformLocation(program->progid, "u_nodes_matrices");
    
    glUniform1i(program->u_texture, 0);
    glUniform1i(program->u_shadowmap, 1);

    return program;
}
