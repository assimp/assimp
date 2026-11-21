#version 330 core

layout(std140) uniform UBO
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in uvec4 in_bone_ids;
layout(location = 3) in vec4 in_bone_weights;

out vec2 v_texcoord;

void main()
{
    gl_Position = projection * view * world * vec4(in_pos, 1.0);
    v_texcoord = in_texcoord;
}
