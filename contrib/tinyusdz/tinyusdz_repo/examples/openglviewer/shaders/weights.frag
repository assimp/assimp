
in float selected;
in vec4 interpolated_weights;

out vec4 output_color;

uniform vec4 highlight_color;

void main()
{
  output_color = interpolated_weights;
  if(selected > 0.999f)
  {
  output_color = mix(output_color, highlight_color, 0.5f);
  }
}
