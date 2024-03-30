in vec3 interpolated_normal;
out vec4 output_color;

void main()
{
  output_color = vec4(normalize(interpolated_normal), 1);
}
