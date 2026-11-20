#version 330 core

uniform sampler2D diffuse_texture;

in vec2 v_texcoord;

out vec4 out_color;

void main()
{
    out_color = texture(diffuse_texture, v_texcoord);
}
