#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "memory.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

struct __attribute__((packed)) {
  union {
    struct {
      uint32_t tex_page_x_base : 4;
      uint32_t tex_page_y_base : 1;
      uint32_t semi_transparency : 2;
      uint32_t tex_page_colors : 2;
      uint32_t dither_24_15 : 1;
      uint32_t draw_display_area : 1;
      uint32_t set_mask_bit : 1;
      uint32_t draw_pixels : 1;
      uint32_t interlace : 1;
      uint32_t reverseflag : 1;
      uint32_t tex_disable : 1;
      uint32_t horz_res_2 : 1;
      uint32_t horz_res_1 : 2;
      uint32_t vert_res : 1;
      uint32_t video_mode : 1;
      uint32_t color_depth : 1;
      uint32_t vert_interlace : 1;
      uint32_t display_disable : 1;
      uint32_t irq : 1;
      uint32_t dma_req : 1;
      uint32_t ready_cmd : 1;
      uint32_t ready_vram : 1;
      uint32_t ready_dma : 1;
      uint32_t dma_direction : 2;
      uint32_t odd_even : 1;
    };
    uint32_t gpustat_32;
  };
  uint8_t tex_rect_x_flip;
  uint8_t tex_rect_y_flip;
  uint8_t tex_window_mask_x;
  uint8_t tex_window_mask_y;
  uint8_t tex_window_offset_x;
  uint8_t tex_window_offset_y;
  uint16_t draw_area_left;
  uint16_t draw_area_top;
  uint16_t draw_area_right;
  uint16_t draw_area_bottom;
  uint16_t draw_offset_x;
  uint16_t draw_offset_y;

  uint16_t start_display_x;
  uint16_t start_display_y;
  uint16_t h_display_range_1;
  uint16_t h_display_range_2;
  uint16_t v_display_range_1;
  uint16_t v_display_range_2;
} gpu;

void gpu_reset() {
  // Hardocded ready status
  gpu.ready_cmd           = 1;
  gpu.ready_dma           = 1;
  gpu.ready_vram          = 1;
  // GP1.2
  gpu.irq                 = 0;
  // GP1.3
  gpu.display_disable     = 1;
  // GP1.4
  gpu.dma_direction       = 0;
  // GP1.5
  gpu.start_display_x     = 0;
  gpu.start_display_y     = 0;
  // GP1.6
  gpu.h_display_range_1   = 0x200;
  gpu.h_display_range_2   = 0xc00;
  // GP1.7
  gpu.v_display_range_1   = 0x10;
  gpu.v_display_range_2   = 0x100;
  // GP1.8
  gpu.horz_res_1          = 0;
  gpu.vert_res            = 0;
  gpu.video_mode          = 0;
  gpu.color_depth         = 0;
  gpu.vert_interlace      = 0;
  gpu.horz_res_2          = 0;
  gpu.reverseflag         = 0;
  // GP0.e1
  gpu.tex_page_x_base     = 0;
  gpu.tex_page_y_base     = 0;
  gpu.semi_transparency   = 0;
  gpu.tex_page_colors     = 0;
  gpu.dither_24_15        = 0;
  gpu.draw_display_area   = 0;
  gpu.tex_disable         = 0;
  gpu.tex_rect_x_flip     = 0;
  gpu.tex_rect_y_flip     = 0;
  // GP0.e2
  gpu.tex_window_mask_x   = 0;
  gpu.tex_window_mask_y   = 0;
  gpu.tex_window_offset_x = 0;
  gpu.tex_window_offset_y = 0;
  // GP0.e3
  gpu.draw_area_left      = 0;
  gpu.draw_area_top       = 0;
  // GP0.e4
  gpu.draw_area_right     = 0;
  gpu.draw_area_bottom    = 0;
  // GP0.e5
  gpu.draw_offset_x       = 0;
  gpu.draw_offset_y       = 0;
  // GP0.e6
  gpu.set_mask_bit        = 0;
  gpu.draw_pixels         = 0;
}

struct __attribute__((packed)) vertex {
  uint32_t position;
  uint32_t color;
  uint32_t texture_uv;
  uint16_t texpage;
  uint16_t clut;
};

struct vertex vertices[1024 * 1024];
uint32_t vertices_count;

uint8_t vram[1024*1024];
char vertex_shader_source[1024*1024];
char fragment_shader_source[1024*1024];

GLuint vao;
GLuint vbo;
GLuint tex;
GLuint program;

SDL_Window *Window;

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

void gpu_init() {
  // for(int n=0; n<1024*1024; n++)
  //   vram[n] = rand();

  uint32_t WindowFlags = SDL_WINDOW_OPENGL;
  Window = SDL_CreateWindow("OpenGL Test", 0, 0, 1280, 960, WindowFlags);
  SDL_GL_CreateContext(Window);

  glewExperimental = GL_TRUE;
  glewInit();

  int f, n;

  f = open("shaders/shader.vert", O_RDONLY);
  n = read(f, vertex_shader_source, 1024*1024);
  if(n < 0) { printf("Failed to load vertex shader source!\n"); exit(1); }
  vertex_shader_source[n] = 0;
  close(f);

  f = open("shaders/shader.frag", O_RDONLY);
  n = read(f, fragment_shader_source, 1024*1024);
  if(n < 0) { printf("Failed to load vertex shader source!\n"); exit(1); }
  fragment_shader_source[n] = 0;
  close(f);

  char *vertex_shader_sources = vertex_shader_source;
  char *fragment_shader_sources = fragment_shader_source;

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, (const GLchar**)&vertex_shader_sources, NULL);
  glCompileShader(vertex_shader);
  printCompileStatus("Vertex shader", vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, (const GLchar**)&fragment_shader_sources, NULL);
  glCompileShader(fragment_shader);
  printCompileStatus("Fragment shader", fragment_shader);

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  printLinkStatus("Shader program", program);
  glUseProgram(program);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glVertexAttribPointer(glGetAttribLocation(program, "position"),  2, GL_UNSIGNED_SHORT, GL_FALSE, 16, (void *)0);
  glVertexAttribPointer(glGetAttribLocation(program, "color"), 3, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void *)4);
  glVertexAttribPointer(glGetAttribLocation(program, "texture_uv"),  2, GL_UNSIGNED_BYTE, GL_FALSE, 16, (void *)8);
  glVertexAttribIPointer(glGetAttribLocation(program, "texpage"),  1, GL_UNSIGNED_SHORT, 16, (void *)12);
  glVertexAttribIPointer(glGetAttribLocation(program, "clut"),  1, GL_UNSIGNED_SHORT, 16, (void *)14);
  glEnableVertexAttribArray(glGetAttribLocation(program, "color"));
  glEnableVertexAttribArray(glGetAttribLocation(program, "position"));
  glEnableVertexAttribArray(glGetAttribLocation(program, "texture_uv"));
  glEnableVertexAttribArray(glGetAttribLocation(program, "texpage"));
  glEnableVertexAttribArray(glGetAttribLocation(program, "clut"));

//  glEnable(GL_DEPTH_TEST);
//  glDepthFunc(GL_LEQUAL);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  GLuint texture;
  glGenTextures(1, &texture); 
  glBindTexture(GL_TEXTURE_2D, texture); 
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  gpu_reset();
}

uint32_t gp0_buffer[12];
uint32_t gp0_data_offset;
uint8_t gp0_command;
uint8_t gp0_offset;

extern SDL_Window *Window;

void gpu_gp0(uint32_t command) {
  //printf("GP0: Command %08x!\n", command);
  gp0_buffer[gp0_offset] = command;
  switch(gp0_buffer[0] & 0xff000000) {
    case 0x0: // Nop
      break;
    case 0x01000000: // Cache clear, meh
      break;
    case 0x28000000:
      // Solid rectangle
      switch(gp0_offset) {
        case 0:
          // All vertices same color
          vertices[vertices_count+0].color = command;
          vertices[vertices_count+1].color = command;
          vertices[vertices_count+2].color = command;
          vertices[vertices_count+3].color = command;
          vertices[vertices_count+4].color = command;
          vertices[vertices_count+5].color = command;
          // No textures
          vertices[vertices_count+0].texpage = 0;
          vertices[vertices_count+1].texpage = 0;
          vertices[vertices_count+2].texpage = 0;
          vertices[vertices_count+3].texpage = 0;
          vertices[vertices_count+4].texpage = 0;
          vertices[vertices_count+5].texpage = 0;
          break;
        case 1:
          vertices[vertices_count+0].position = command;
          break;
        case 2:
          vertices[vertices_count+1].position = command;
          vertices[vertices_count+3].position = command;
          break;
        case 3:
          vertices[vertices_count+2].position = command;
          vertices[vertices_count+4].position = command;
          break;
        case 4:
          vertices[vertices_count+5].position = command;

          gp0_offset = -1;
          vertices_count += 6;
          break;
      }
      gp0_offset++;
      break;
    case 0x2c000000:
      // Textured rectangle
      switch(gp0_offset) {
        case 0:
          //printf("Drawing textured rectangle.\n");
          // Set base color
          vertices[vertices_count+0].color = command;
          vertices[vertices_count+1].color = command;
          vertices[vertices_count+2].color = command;
          vertices[vertices_count+3].color = command;
          vertices[vertices_count+4].color = command;
          vertices[vertices_count+5].color = command;
          break;
        case 1:
          //printf("  %08x", command);
          vertices[vertices_count+0].position = command;
          break;
        case 2:
          //printf("  %04x\n", command >> 16);
          // CLUT comes from here
          vertices[vertices_count+0].clut = (command >> 16);
          vertices[vertices_count+1].clut = (command >> 16);
          vertices[vertices_count+2].clut = (command >> 16);
          vertices[vertices_count+3].clut = (command >> 16);
          vertices[vertices_count+4].clut = (command >> 16);
          vertices[vertices_count+5].clut = (command >> 16);

          vertices[vertices_count+0].texture_uv = command;
          break;
        case 3:
          //printf("  %08x", command);
          vertices[vertices_count+1].position = command;
          vertices[vertices_count+3].position = command;
          break;
        case 4:
          //printf("  %04x (%04x)\n", command & 0xffff, command>>16);
          // Texture mode comes from texpage here
          vertices[vertices_count+0].texpage = (1<<15) | (command >> 16);
          vertices[vertices_count+1].texpage = (1<<15) | (command >> 16);
          vertices[vertices_count+2].texpage = (1<<15) | (command >> 16);
          vertices[vertices_count+3].texpage = (1<<15) | (command >> 16);
          vertices[vertices_count+4].texpage = (1<<15) | (command >> 16);
          vertices[vertices_count+5].texpage = (1<<15) | (command >> 16);

          vertices[vertices_count+1].texture_uv = command;
          vertices[vertices_count+3].texture_uv = command;
          break;
        case 5:
          //printf("  %08x", command);
          vertices[vertices_count+2].position = command;
          vertices[vertices_count+4].position = command;
          break;
        case 6:
          //printf("  %04x\n", command & 0xffff);
          vertices[vertices_count+2].texture_uv = command;
          vertices[vertices_count+4].texture_uv = command;
          break;
        case 7:
          //printf("  %08x", command);
          vertices[vertices_count+5].position = command;
          break;
        case 8:
          //printf("  %04x\n", command & 0xffff);
          vertices[vertices_count+5].texture_uv = command;
          gp0_offset = -1;
          vertices_count += 6;
          break;
      }
      gp0_offset++;
      break;
    case 0x30000000:
      // Shaded triangle
      switch(gp0_offset) {
        case 0:
          vertices[vertices_count+0].color = command;
          // No textures
          vertices[vertices_count+0].texpage = 0;
          vertices[vertices_count+1].texpage = 0;
          vertices[vertices_count+2].texpage = 0;
          vertices[vertices_count+3].texpage = 0;
          vertices[vertices_count+4].texpage = 0;
          vertices[vertices_count+5].texpage = 0;
          break;
        case 1:
          vertices[vertices_count+0].position = command;
          break;
        case 2:
          vertices[vertices_count+1].color = command;
          break;
        case 3:
          vertices[vertices_count+1].position = command;
          break;
        case 4:
          vertices[vertices_count+2].color = command;
          break;
        case 5:
          vertices[vertices_count+2].position = command;

          gp0_offset = -1;
          vertices_count += 3;
          break;
      }
      gp0_offset++;
      break;
    case 0x38000000:
      // Shaded rectangle
      switch(gp0_offset) {
        case 0:
          vertices[vertices_count+0].color = command;
          // No textures
          vertices[vertices_count+0].texpage = 0;
          vertices[vertices_count+1].texpage = 0;
          vertices[vertices_count+2].texpage = 0;
          vertices[vertices_count+3].texpage = 0;
          vertices[vertices_count+4].texpage = 0;
          vertices[vertices_count+5].texpage = 0;
          break;
        case 1:
          vertices[vertices_count+0].position = command;
          break;
        case 2:
          vertices[vertices_count+1].color = command;
          vertices[vertices_count+3].color = command;
          break;
        case 3:
          vertices[vertices_count+1].position = command;
          vertices[vertices_count+3].position = command;
          break;
        case 4:
          vertices[vertices_count+2].color = command;
          vertices[vertices_count+4].color = command;
          break;
        case 5:
          vertices[vertices_count+2].position = command;
          vertices[vertices_count+4].position = command;
        case 6:
          vertices[vertices_count+5].color = command;
          break;
        case 7:
          vertices[vertices_count+5].position = command;

          gp0_offset = -1;
          vertices_count += 6;
          break;
      }
      gp0_offset++;
      break;
    case 0xa0000000:
      if(gp0_offset == 2) {
        //printf("load data.\n");
        //printf("destination %08x dimensions %08x\n", gp0_buffer[1], gp0_buffer[2]);
        gp0_offset++;
        gp0_data_offset = 0;
      } else if(gp0_offset == 3) {
        uint32_t pixels = gp0_data_offset * 2;
        uint32_t coord = pixels / (gp0_buffer[2] & 0xffff) * 1024 + pixels % (gp0_buffer[2] & 0xffff) + (gp0_buffer[1] >> 16) * 1024 + (gp0_buffer[1] & 0xffff);
        //printf("%08x = %04x\n", coord, command >> 16);
        //printf("%08x = %04x\n", coord+1, command & 0xffff);
        ((uint16_t*)vram)[coord] = command & 0xffff;
        ((uint16_t*)vram)[coord+1] = command >> 16;
        gp0_data_offset++;
        if(gp0_data_offset == ((gp0_buffer[2] >> 16) * (gp0_buffer[2] & 0xffff) + 1 ) / 2) {
          //printf("load data end\n");
          glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, 1024, 512, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, vram);
          glGenerateMipmap(GL_TEXTURE_2D);
          gp0_offset = 0;
        }
      } else {
        gp0_offset++;
      }
      break;
    case 0xc0000000:
      if(gp0_offset == 2) {
        gp0_offset = 0;
      } else {
        gp0_offset++;
      }
      break;
    case 0xe1000000:
      gpu.tex_page_x_base   = (command >> 0)  & 0xf;
      gpu.tex_page_y_base   = (command >> 4)  & 0x1;
      gpu.semi_transparency = (command >> 5)  & 0x3;
      gpu.tex_page_colors   = (command >> 7)  & 0x3;
      gpu.dither_24_15      = (command >> 9)  & 0x1;
      gpu.draw_display_area = (command >> 10) & 0x1;
      gpu.tex_disable       = (command >> 11) & 0x1;
      gpu.tex_rect_x_flip   = (command >> 12) & 0x1;
      gpu.tex_rect_y_flip   = (command >> 13) & 0x1;
      break;
    case 0xe2000000:
      gpu.tex_window_mask_x   = (command >> 0)  & 0x1f;
      gpu.tex_window_mask_y   = (command >> 5)  & 0x1f;
      gpu.tex_window_offset_x = (command >> 10) & 0x1f;
      gpu.tex_window_offset_y = (command >> 15) & 0x1f;
      break;
    case 0xe3000000:
      gpu.draw_area_left   = (command >> 0)   & 0x3ff;
      gpu.draw_area_top    = (command >> 10)  & 0x3ff;
      break;
    case 0xe4000000:
      gpu.draw_area_right  = (command >> 0)   & 0x3ff;
      gpu.draw_area_bottom = (command >> 10)  & 0x3ff;
      break;
    case 0xe5000000:
      gpu.draw_offset_x    = (command >> 0)   & 0x7ff;
      gpu.draw_offset_y    = (command >> 11)  & 0x7ff;
      SDL_Event Event;
      while (SDL_PollEvent(&Event))
        if (Event.type == SDL_QUIT) exit(0);
      // DRAW!
      //printf("FRAME!\n");
      glClear(GL_COLOR_BUFFER_BIT);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, vertices_count);
      vertices_count = 0;
      SDL_GL_SwapWindow(Window);
      break;
    case 0xe6000000:
      gpu.set_mask_bit     = (command >> 0)   & 0x1;
      gpu.draw_pixels      = (command >> 1)   & 0x1;
      break;
    default:
      printf("GP0: Unknown command %08x!\n", command);
      exit(1);
  }
}

void gpu_gp1(uint32_t command) {
  switch (command & 0xff000000)
  {
  case 0x0:
    gpu_reset();
    break;
  case 0x01000000: // Reset command buffer, meh
    break;
  case 0x02000000:
    gpu.irq = 0;
    break;
  case 0x03000000:
    gpu.display_disable = command & 0x1;
    break;
  case 0x04000000:
    gpu.dma_direction = command & 0x3;
    break;
  case 0x05000000:
    gpu.start_display_x   = (command >> 0)  & 0x3ff;
    gpu.start_display_y   = (command >> 10) & 0x3ff;
    break;
  case 0x06000000:
    gpu.h_display_range_1 = (command >> 0)  & 0xfff;
    gpu.h_display_range_2 = (command >> 12) & 0xfff;
    break;
  case 0x07000000:
    gpu.v_display_range_1 = (command >> 0)  & 0xfff;
    gpu.v_display_range_2 = (command >> 12) & 0xfff;
    break;
  case 0x08000000:
    gpu.horz_res_1     = (command >> 0)  & 0x3;
    gpu.vert_res       = (command >> 2)  & 0x1;
    gpu.video_mode     = (command >> 3)  & 0x1;
    gpu.color_depth    = (command >> 4)  & 0x1;
    gpu.vert_interlace = (command >> 5)  & 0x1;
    gpu.horz_res_2     = (command >> 6)  & 0x1;
    gpu.reverseflag    = (command >> 7)  & 0x1;
    break;
  default:
    printf("GP1: Unknown command %08x!\n", command);
    exit(1);
  }
}

uint32_t gpu_load_32(uint32_t address) {
  switch(address) {
    case 0x1f801814:
    // Temporary hack to trick bios into continuing during early development
      gpu.vert_res = 0;
      return(gpu.gpustat_32);
    case 0x1f801810:
      return(0);
    default:
      printf("Unknown GPU register: 0x%08x\n", address);
      exit(1);
  }
}

void gpu_store_32(uint32_t address, uint32_t value) {
  switch(address) {
    case 0x1f801810:
      gpu_gp0(value);
      return;
    case 0x1f801814:
      gpu_gp1(value);
      return;
    default:
      printf("Unknown GPU register: 0x%08x\n", address);
      exit(1);
  }
}

 memory_accessor_t gpu_accessor = {
  .load_32 = gpu_load_32,
  .load_16 = memory_dummy_load_16,
  .load_8 = memory_dummy_load_8,
  .store_32 = gpu_store_32,
  .store_16 = memory_dummy_store_16,
  .store_8 = memory_dummy_store_8,
};
