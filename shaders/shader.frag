#version 130

in vec3 frag_color;
in vec2 frag_texture_uv;
flat in uint frag_texture_enable;
flat in uint frag_texture_mode;
flat in uint frag_texture_base_x;
flat in uint frag_texture_base_y;
in flat uvec2 frag_clut;
uniform sampler2D vram_texture;

// Some code below is placeholder and not my work.

out vec3 out_color;

vec4 vram_read(int x, int y) {
    return texelFetch(vram_texture, ivec2(int(x), int(y)), 0);
}

uint unpack_texel(vec4 texel) {
    uint i0 = uint(texel.a);

    texel *= ivec4(0x1f);
    uint i1 = uint(texel.b + 0.5);
    uint i2 = uint(texel.g + 0.5);
    uint i3 = uint(texel.r + 0.5);

    return (i0 << 15) | (i1 << 10) | (i2 << 5) | i3;
}

void main() {
  if(frag_texture_enable == 1u) {
    // Textures enabled
    int tc_x = int(frag_texture_base_x) + int(frag_texture_uv.x / 4);
    int tc_y = int(frag_texture_base_y) + int(frag_texture_uv.y);
    uint texel = unpack_texel(vram_read(tc_x, tc_y));

        uint clut_index = ((texel >> ((uint(tc_x) % 4u) * 4u)) & 0xfu);
        out_color = vram_read(frag_clut.x + int(clut_index), frag_clut.y);

    //out_color = vec3(1,0,0);
  } else {
    // Textures disabled
    out_color = frag_color;
  }
}
