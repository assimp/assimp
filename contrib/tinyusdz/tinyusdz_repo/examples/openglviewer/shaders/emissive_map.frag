in vec2 interpolated_uv;
uniform sampler2D emissive_texture;
uniform vec3 emissive_factor;
out vec4 output_color;

void main()
{
  vec4 sampled_pixel = texture(emissive_texture, interpolated_uv);
  output_color = vec4(emissive_factor * sampled_pixel.rgb, 1.0);
}
