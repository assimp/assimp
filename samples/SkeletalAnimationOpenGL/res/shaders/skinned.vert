#version 330 core

layout(std140) uniform UBO
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

layout(std140) uniform BoneUBO
{
    mat4 bones[MAX_JOINT_COUNT]; // Our app.cpp adds MAX_JOINT_COUNT to the shader source when compiling
};

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in uvec4 in_bone_ids;
layout(location = 3) in vec4 in_bone_weights;

out vec2 v_texcoord;

void main()
{
    mat4 bone_matrix = in_bone_weights.x * bones[in_bone_ids.x]
                        + in_bone_weights.y * bones[in_bone_ids.y]
                        + in_bone_weights.z * bones[in_bone_ids.z]
                        + in_bone_weights.w * bones[in_bone_ids.w];

    vec4 pos = bone_matrix * vec4(in_pos, 1.0);

    gl_Position = projection * view * world * pos;
    v_texcoord = in_texcoord;
}
