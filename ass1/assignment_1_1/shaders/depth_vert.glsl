#version 410 core
layout (location = 0) in vec3 pos;

uniform mat4 lightMVP;

void main()
{
    gl_Position = lightMVP * vec4(pos, 1.0);
}
