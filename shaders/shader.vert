#version 130

in vec2 position;
in vec3 color;
in vec2 texture_uv;
in int texture_mode;

out vec3 frag_color;
out vec2 frag_texture_uv;
flat out int frag_texture_mode;

void main()
{
  gl_Position = vec4(position.x / 320 - 1.0, 1.0 - position.y / 240, 0.0, 1.0);
  frag_color = color;
  frag_texture_uv = texture_uv;
  frag_texture_mode = texture_mode;
}
