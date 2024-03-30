in vec2 interpolated_uv;
uniform sampler2D occlusion_texture;

out vec4 output_color;

void main()
{
  vec4 sampled_pixel = texture(occlusion_texture, interpolated_uv).rrra;
  output_color = vec4(sampled_pixel.rgb, 1.0);
}
