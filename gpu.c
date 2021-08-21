#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "memory.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "gl.h"

struct vertex {
  uint32_t color;
  uint32_t position;
  uint32_t uv;
  uint32_t zpos;
};
struct vertex shaded_vertices[1024 * 1024];
struct vertex textured_vertices[1024 * 1024];
uint32_t shaded_vertices_count, textured_vertices_count;
uint32_t zpos;

uint8_t vram[1024*1024];

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

GLuint vao;
GLuint vbo;
GLuint idx;
GLuint tex;
GLuint shaded_program;
GLuint textured_program;

SDL_Window *Window;

void gpu_init() {
  memset(vram, 0xff, 1024*1024);
  uint32_t WindowFlags = SDL_WINDOW_OPENGL;
  Window = SDL_CreateWindow("OpenGL Test", 0, 0, 1280, 960, WindowFlags);
  SDL_GL_CreateContext(Window);

  glewExperimental = GL_TRUE;
  glewInit();
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);
  printCompileStatus("Vertex shader", vertexShader);

  GLuint shadedFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(shadedFragmentShader, 1, &shadedFragmentSource, NULL);
  glCompileShader(shadedFragmentShader);
  printCompileStatus("Textured fragment shader", shadedFragmentShader);

  GLuint texturedFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(texturedFragmentShader, 1, &texturedFragmentSource, NULL);
  glCompileShader(texturedFragmentShader);
  printCompileStatus("Textured fragment shader", texturedFragmentShader);

  shaded_program = glCreateProgram();
  glAttachShader(shaded_program, vertexShader);
  glAttachShader(shaded_program, shadedFragmentShader);
  glLinkProgram(shaded_program);
  printLinkStatus("Shaded shader program", shaded_program);

  textured_program = glCreateProgram();
  glAttachShader(textured_program, vertexShader);
  glAttachShader(textured_program, texturedFragmentShader);
  glLinkProgram(textured_program);
  printLinkStatus("Shaded shader program", textured_program);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glVertexAttribPointer(glGetAttribLocation(shaded_program, "color"), 3, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void *)0);
  glVertexAttribPointer(glGetAttribLocation(shaded_program, "location"),  2, GL_UNSIGNED_SHORT, GL_FALSE, 16, (void *)4);
  glVertexAttribPointer(glGetAttribLocation(shaded_program, "uv"),  2, GL_UNSIGNED_SHORT, GL_FALSE, 16, (void *)8);
  glVertexAttribPointer(glGetAttribLocation(shaded_program, "zpos"),  2, GL_UNSIGNED_INT, GL_TRUE, 16, (void *)8);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glEnable(GL_DEPTH_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

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
          shaded_vertices[shaded_vertices_count+0].color = command;
          shaded_vertices[shaded_vertices_count+1].color = command;
          shaded_vertices[shaded_vertices_count+2].color = command;
          shaded_vertices[shaded_vertices_count+3].color = command;
          shaded_vertices[shaded_vertices_count+4].color = command;
          shaded_vertices[shaded_vertices_count+5].color = command;
          break;
        case 1:
          shaded_vertices[shaded_vertices_count+0].position = command;
          shaded_vertices[shaded_vertices_count+0].zpos = zpos;
          break;
        case 2:
          shaded_vertices[shaded_vertices_count+1].position = command;
          shaded_vertices[shaded_vertices_count+1].zpos = zpos;
          shaded_vertices[shaded_vertices_count+3].position = command;
          shaded_vertices[shaded_vertices_count+3].zpos = zpos;
          break;
        case 3:
          shaded_vertices[shaded_vertices_count+2].position = command;
          shaded_vertices[shaded_vertices_count+2].zpos = zpos;
          shaded_vertices[shaded_vertices_count+4].position = command;
          shaded_vertices[shaded_vertices_count+4].zpos = zpos;
          break;
        case 4:
          shaded_vertices[shaded_vertices_count+5].position = command;
          shaded_vertices[shaded_vertices_count+5].zpos = zpos;

          gp0_offset = -1;
          shaded_vertices_count += 6;
          zpos++;
          break;
      }
      gp0_offset++;
      break;
    case 0x2c000000:
      // Textured rectangle
      switch(gp0_offset) {
        case 0:
          // Ignore color
          break;
        case 1:
          shaded_vertices[shaded_vertices_count+0].position = command;
          shaded_vertices[shaded_vertices_count+0].zpos = zpos;
          break;
        case 2:
          shaded_vertices[shaded_vertices_count+0].uv = command;
          break;
        case 3:
          shaded_vertices[shaded_vertices_count+1].position = command;
          shaded_vertices[shaded_vertices_count+1].zpos = zpos;
          shaded_vertices[shaded_vertices_count+3].position = command;
          shaded_vertices[shaded_vertices_count+3].zpos = zpos;
          break;
        case 4:
          shaded_vertices[shaded_vertices_count+1].uv = command;
          shaded_vertices[shaded_vertices_count+3].uv = command;
          break;
        case 5:
          shaded_vertices[shaded_vertices_count+2].position = command;
          shaded_vertices[shaded_vertices_count+2].zpos = zpos;
          shaded_vertices[shaded_vertices_count+4].position = command;
          shaded_vertices[shaded_vertices_count+4].zpos = zpos;
        case 6:
          shaded_vertices[shaded_vertices_count+2].uv = command;
          shaded_vertices[shaded_vertices_count+4].uv = command;
          break;
        case 7:
          shaded_vertices[shaded_vertices_count+5].position = command;
          shaded_vertices[shaded_vertices_count+5].zpos = zpos;
          break;
        case 8:
          shaded_vertices[shaded_vertices_count+5].uv = command;
          gp0_offset = -1;
          shaded_vertices_count += 6;
          zpos++;
          break;
      }
      gp0_offset++;
      break;
    case 0x30000000:
      // Shaded triangle
      switch(gp0_offset) {
        case 0:
          shaded_vertices[shaded_vertices_count+0].color = command;
          break;
        case 1:
          shaded_vertices[shaded_vertices_count+0].position = command;
          shaded_vertices[shaded_vertices_count+0].zpos = zpos;
          break;
        case 2:
          shaded_vertices[shaded_vertices_count+1].color = command;
          break;
        case 3:
          shaded_vertices[shaded_vertices_count+1].position = command;
          shaded_vertices[shaded_vertices_count+1].zpos = zpos;
          break;
        case 4:
          shaded_vertices[shaded_vertices_count+2].color = command;
          break;
        case 5:
          shaded_vertices[shaded_vertices_count+2].position = command;
          shaded_vertices[shaded_vertices_count+2].zpos = zpos;

          gp0_offset = -1;
          shaded_vertices_count += 3;
          zpos++;
          break;
      }
      gp0_offset++;
      break;
    case 0x38000000:
      // Shaded rectangle
      switch(gp0_offset) {
        case 0:
          shaded_vertices[shaded_vertices_count+0].color = command;
          break;
        case 1:
          shaded_vertices[shaded_vertices_count+0].position = command;
          shaded_vertices[shaded_vertices_count+0].zpos = zpos;
          break;
        case 2:
          shaded_vertices[shaded_vertices_count+1].color = command;
          shaded_vertices[shaded_vertices_count+3].color = command;
          break;
        case 3:
          shaded_vertices[shaded_vertices_count+1].position = command;
          shaded_vertices[shaded_vertices_count+1].zpos = zpos;
          shaded_vertices[shaded_vertices_count+3].position = command;
          shaded_vertices[shaded_vertices_count+3].zpos = zpos;
          break;
        case 4:
          shaded_vertices[shaded_vertices_count+2].color = command;
          shaded_vertices[shaded_vertices_count+4].color = command;
          break;
        case 5:
          shaded_vertices[shaded_vertices_count+2].position = command;
          shaded_vertices[shaded_vertices_count+2].zpos = zpos;
          shaded_vertices[shaded_vertices_count+4].position = command;
          shaded_vertices[shaded_vertices_count+4].zpos = zpos;
        case 6:
          shaded_vertices[shaded_vertices_count+5].color = command;
          break;
        case 7:
          shaded_vertices[shaded_vertices_count+5].position = command;
          shaded_vertices[shaded_vertices_count+5].zpos = zpos;

          gp0_offset = -1;
          shaded_vertices_count += 6;
          zpos++;
          break;
      }
      gp0_offset++;
      break;
    case 0xa0000000:
      if(gp0_offset == 2) {
        //printf("load data start\n");
        gp0_offset++;
        gp0_data_offset = 0;
      } else if(gp0_offset == 3) {
        uint32_t coord = gp0_data_offset * 2 / (gp0_buffer[2] >> 16) * 1024 + (gp0_data_offset * 2) % (gp0_buffer[2] >> 16) + (gp0_buffer[2] >> 16) * 1024 + (gp0_buffer[2] & 0xffff);
        ((uint16_t*)vram)[coord] = command >> 16;
        ((uint16_t*)vram)[coord+1] = command & 0xffff;
        gp0_data_offset++;
        if(gp0_data_offset == ((gp0_buffer[2] >> 16) * (gp0_buffer[2] & 0xffff) + 1 ) / 2) {
          //printf("load data end\n");
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2048, 512, 0, GL_RGB, GL_UNSIGNED_INT_8_8_8_8, vram);
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
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glUseProgram(shaded_program);
      glBufferData(GL_ARRAY_BUFFER, sizeof(shaded_vertices), shaded_vertices, GL_DYNAMIC_DRAW);
      glDrawArrays(GL_TRIANGLES, 0, shaded_vertices_count);
      zpos = 0;
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
