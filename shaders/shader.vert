#version 130

in vec2 position;
in vec3 color;
in vec2 texture_uv;
in uint texpage;
in uvec2 clut;

out vec3 frag_color;
out vec2 frag_texture_uv;
flat out uint frag_texture_enable;
flat out uint frag_texture_mode;
flat out uvec2 frag_texture_base;
flat out uvec2 frag_clut;

void main()
{
  gl_Position = vec4(position.x / 320 - 1.0, 1.0 - position.y / 240, 0.0, 1.0);
  frag_color = color;
  frag_texture_uv = texture_uv;
  frag_texture_enable = (texpage & 0x8000u) >> 15;
  frag_texture_mode = (texpage >> 7) & 0x3u;
  frag_texture_base = uvec2((texpage & 0xfu) * 64u, ((texpage>>4) & 0x1u) * 256u);
  frag_clut = clut;
}
