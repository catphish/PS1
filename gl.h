const char *vertexSource = "#version 130\n\
in mediump vec2 xy;\n\
in mediump vec3 rgb;\n\
out mediump vec3 vcolor;\n\
void main()\n\
{\n\
  gl_Position = vec4(2.0 * xy.x / 1024, 2.0 * xy.y / 1024, 0.0, 1.0);\n\
  vcolor = vec3(1.0 * rgb.r / 255, 1.0 * rgb.g / 255, 1.0 * rgb.b / 255);\n\
}";

const char *fragmentSource = "#version 130\n\
in mediump vec3 vcolor;\n\
out mediump vec3 fragColor;\n\
void main()\n\
{\n\
  fragColor = vcolor;\n\
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
