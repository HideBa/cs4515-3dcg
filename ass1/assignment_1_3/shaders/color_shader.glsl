#version 410

// Output for on-screen color
layout(location = 0) out vec4 outColor;

//Textures
uniform sampler2D accumulator_texture;
uniform ivec2 screen_dimensions;

//Set gl_FragCoord to pixel center
layout(pixel_center_integer) in vec4 gl_FragCoord;

void main()
{
  vec2 texel_coord = gl_FragCoord.xy / vec2(screen_dimensions);
  vec4 accumulator_color = texture(accumulator_texture, texel_coord);
  if(accumulator_color.a == 0) {
    outColor = vec4(0, 0, 0, 1);
  } else {
    outColor = vec4(accumulator_color.rgb / accumulator_color.a, 1);
  }
}