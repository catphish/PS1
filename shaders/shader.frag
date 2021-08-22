#version 130

in vec3 frag_color;
in vec2 frag_texture_uv;
flat in int frag_texture_mode;

out vec3 out_color;

void main()

{
  switch(frag_texture_mode) {
    case 0: // 4-bit lookup
      out_color = vec3(1,0,0);
      break;
    case 1: // 8-bit lookup
      out_color = vec3(0,1,0);
      break;
    case 2: // 15-bit raw
      out_color = vec3(0,0,1);
      break;
    case 3: // No Texture
      out_color = frag_color;
      break;
  }
}