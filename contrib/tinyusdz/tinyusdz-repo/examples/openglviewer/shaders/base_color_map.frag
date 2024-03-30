in vec2 interpolated_uv;
uniform sampler2D base_color_texture;
uniform vec4  base_color_factor;

out vec4 output_color;

void main()
{
  vec4 sampled_pixel = texture(base_color_texture, interpolated_uv);
  output_color = base_color_factor * sampled_pixel;
}
