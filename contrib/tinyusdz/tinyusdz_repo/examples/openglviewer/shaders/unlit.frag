in vec2 interpolated_uv;
in vec3 interpolated_normal;
in vec4 interpolated_colors;
out vec4 output_color;

in float selected;

uniform sampler2D base_color_texture;
uniform sampler2D emissive_texture;

uniform vec3 emissive_factor;
uniform vec4 base_color_factor;
uniform int alpha_mode;
uniform float alpha_cutoff;
uniform vec4 highlight_color;

#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2


void main()
{
  vec4 color = base_color_factor * texture(base_color_texture, interpolated_uv);
  if(interpolated_colors != vec4(0))
  color *= interpolated_colors;

  if(alpha_mode == ALPHA_MASK)
  {
	if(color.a < alpha_cutoff)
	{
		discard;
	}
  }

  //slight blend optimization
  if(alpha_mode == ALPHA_BLEND)
  {
	if(color.a < 0.01)
	{
		discard;
	}
  }

  vec4 emissive = vec4(emissive_factor, 1) * texture(emissive_texture, interpolated_uv);

  color = color + emissive;
  output_color = color;

  	if(selected >= 0.999)
	{
		output_color = highlight_color;
	}
}
