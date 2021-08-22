#version 130

in vec3 frag_color;
in vec2 frag_texture_uv;
flat in uint frag_texture_enable;
flat in uint frag_texture_mode;
flat in uvec2 frag_texture_base;
flat in uvec2 frag_clut;
uniform sampler2D vram_texture;

out vec3 out_color;

 vec4 vram_read(uint x, uint y) {
     return texelFetch(vram_texture, ivec2(int(x), int(y)), 0);
 }

void main() {
  if(frag_texture_enable == 1u) {
    // Textures enabled
    uint tex_x = frag_texture_base.x + uint(frag_texture_uv.x)/4u;
    uint tex_y = frag_texture_base.y + uint(frag_texture_uv.y);
    vec4 texel = vram_read(tex_x, tex_y);
    switch(int(frag_texture_base.x + uint(frag_texture_uv.x)) % 4) {
      case 3: out_color = vec3(texel.r,0,0);break;
      case 2: out_color = vec3(texel.g,0,0);break;
      case 1: out_color = vec3(texel.b,0,0);break;
      case 0: out_color = vec3(texel.a,0,0);break;
    }
    if(out_color == vec3(0,0,0))discard;
  } else {
    // Textures disabled
    out_color = frag_color;
  }
}
