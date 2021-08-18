#include <stdio.h>
#include <stdint.h>
#include "cpu.h"
#include "memory.h"
#include "dma.h"
#include "rom.h"
#include "gpu.h"

#include <assert.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "gl.h"

GLuint vao;
GLuint vbo;
GLuint idx;
GLuint tex;
GLuint program;

uint16_t cycle_count;

void gl_init() {
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);
  printCompileStatus("Vertex shader", vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);
  printCompileStatus("Fragment shader", fragmentShader);

  program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
  printLinkStatus("Shader program", program);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glVertexAttribPointer(glGetAttribLocation(program, "rgb"), 3, GL_UNSIGNED_BYTE, GL_FALSE, 8, (void *)0);
  glVertexAttribPointer(glGetAttribLocation(program, "xy"),  2, GL_UNSIGNED_SHORT, GL_FALSE, 8, (void *)4);

  glEnable(GL_DEPTH_TEST);
  
  glUseProgram(program);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
}

SDL_Window *Window;
int main() {
  uint32_t running = 1;
  uint32_t WindowFlags = SDL_WINDOW_OPENGL;
  Window = SDL_CreateWindow("OpenGL Test", 0, 0, 1024, 768, WindowFlags);
  SDL_GL_CreateContext(Window);

  glewExperimental = GL_TRUE;
  glewInit();
  gl_init();

  rom_load_bios();
  cpu_reset();
  dma_reset();
  gpu_reset();
  glUseProgram(program);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  while(running) {
    cycle_count++;
    if(!cycle_count) {
      SDL_Event Event;
      while (SDL_PollEvent(&Event))
        if (Event.type == SDL_QUIT) running = 0;
    }
    cpu_fetch_execute();

  }
  return(0);
}
