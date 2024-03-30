
layout (location = 0) in vec3 input_position;
layout (location = 1) in vec3 input_normal;
layout (location = 2) in vec2 input_uv0;
layout (location = 3) in vec4 input_colors;
//layout (location = 4) in vec4 input_joints;
//layout (location = 5) in vec4 input_weights;

uniform mat4 modelMatrix;
uniform mat4 mvp;
uniform mat3 normalMatrix;
//uniform int active_joint;

//uniform vec3 active_vertex;

out vec3 interpolated_normal;
out vec3 fragment_world_position;
out vec4 interpolated_colors;

out vec2 interpolated_uv0;
//out vec4 interpolated_weights;

//out float selected;

void main()
{
  gl_Position = mvp * vec4(input_position, 1.0f);
  interpolated_normal = normalMatrix * normalize(input_normal);
  fragment_world_position = vec3(modelMatrix * vec4(input_position, 1.0f));
  
  interpolated_uv0 = input_uv0;
  interpolated_colors = input_colors;

}
