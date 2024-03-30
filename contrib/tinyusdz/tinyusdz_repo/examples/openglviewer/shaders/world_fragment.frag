in vec3 fragment_world_position;
out vec4 output_color;

void main()
{
	output_color = vec4(fragment_world_position, 1);
}
