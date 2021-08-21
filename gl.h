const char *vertexSource = "#version 130\n\
in vec2 location;\n\
in vec3 color;\n\
in vec2 uv;\n\
in float zpos;\n\
out vec3 vcolor;\n\
out vec2 vuv;\n\
void main()\n\
{\n\
  gl_Position = vec4(location.x / 320 - 1.0, 1.0 - location.y / 240, zpos, 1.0);\n\
  vcolor = color;\n\
  vuv = vec2(uv.x / 2048 - uv.y / 512);\n\
}";

const char *shadedFragmentSource = "#version 130\n\
in vec3 vcolor;\n\
out vec3 fragColor;\n\
void main()\n\
{\n\
  fragColor = vcolor;\n\
}";
const char *texturedFragmentSource = "#version 130\n\
in vec2 vuv;\n\
out vec4 fragColor;\n\
uniform sampler2D ourTexture;\n\
void main()\n\
{\n\
  fragColor = texture(ourTexture, vuv);\n\
}";


void printStatus(const char *step, GLuint context, GLuint status)
{
  GLint result = GL_FALSE;
  glGetShaderiv(context, status, &result);
  if (result == GL_FALSE) {
    char buffer[1024];
    if (status == GL_COMPILE_STATUS)
      glGetShaderInfoLog(context, 1024, NULL, buffer);
    else
      glGetProgramInfoLog(context, 1024, NULL, buffer);
    if (buffer[0])
      fprintf(stderr, "%s: %s\n", step, buffer);
  };
}

void printCompileStatus(const char *step, GLuint context)
{
  printStatus(step, context, GL_COMPILE_STATUS);
}

void printLinkStatus(const char *step, GLuint context)
{
  printStatus(step, context, GL_LINK_STATUS);
}
