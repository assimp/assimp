in vec2 interpolated_uv;
uniform sampler2D metallic_roughness_texture;
uniform float metallic_factor;
uniform float roughness_factor;
out vec4 output_color;

void main()
{
	vec3 sampled_pixel = texture(metallic_roughness_texture, interpolated_uv).rgb;
	output_color = vec4(0.0f, roughness_factor * sampled_pixel.g, metallic_factor * sampled_pixel.b, 1.0f);
}
