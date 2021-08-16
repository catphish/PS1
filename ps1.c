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
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &idx);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(glGetAttribLocation(program, "point"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glVertexAttribPointer(glGetAttribLocation(program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

  glEnable(GL_DEPTH_TEST);
  
  glUseProgram(program);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glGenTextures(1, &tex);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUniform1i(glGetUniformLocation(program, "tex"), 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_BGR, GL_FLOAT, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glGenerateMipmap(GL_TEXTURE_2D);
}

int main() {
  uint32_t running = 1;
  uint32_t WindowFlags = SDL_WINDOW_OPENGL;
  SDL_Window *Window = SDL_CreateWindow("OpenGL Test", 0, 0, 1024, 768, WindowFlags);
  SDL_GL_CreateContext(Window);

  glewExperimental = GL_TRUE;
  glewInit();
  gl_init();

  rom_load_bios();
  cpu_reset();
  dma_reset();
  gpu_reset();
  while(running) {
    cpu_fetch_execute();
    SDL_Event Event;
    while (SDL_PollEvent(&Event))
      if (Event.type == SDL_QUIT) running = 0;

  // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // glUseProgram(program);
  // glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void *)0);

  //   SDL_GL_SwapWindow(Window);
  }
  return(0);
}
