#version 130

in vec3 frag_color;
in vec2 frag_texture_uv;
flat in uint frag_texture_enable;
flat in uint frag_texture_mode;
flat in uvec2 frag_texture_base;
flat in uint frag_clut;
uniform usampler2D vram_texture;

out vec3 out_color;

 uint vram_read(uint x, uint y) {
     return texelFetch(vram_texture, ivec2(int(x), int(y)), 0).r;
 }

void main() {
  if(frag_texture_enable == 1u) {
    // Textures enabled
    uint tex_x = frag_texture_base.x + uint(frag_texture_uv.x)/4u;
    uint tex_y = frag_texture_base.y + uint(frag_texture_uv.y);
    uint texel = vram_read(tex_x, tex_y);
    uint clut_index;
    switch(int(frag_texture_base.x + uint(frag_texture_uv.x)) % 4) {
      case 0: clut_index = (texel >> 0)  & 0xfu; break;
      case 1: clut_index = (texel >> 4)  & 0xfu; break;
      case 2: clut_index = (texel >> 8)  & 0xfu; break;
      case 3: clut_index = (texel >> 12) & 0xfu; break;
    }
    uint frag_clut_x = frag_clut & 0x3fu;
    uint frag_clut_y = frag_clut >> 6;
    uint pixel_data = vram_read(frag_clut_x*16u + clut_index, frag_clut_y);
    out_color = vec3(float((pixel_data>>0) & 0x1fu)/31, float((pixel_data>>5) & 0x1fu)/31, float((pixel_data>>10) & 0x1fu)/31);
    if(out_color == vec3(0,0,0)) discard;
  } else {
    // Textures disabled
    out_color = frag_color;
  }
}
