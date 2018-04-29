#version 330 core

layout(location = 0) in vec3 position;

uniform vec3 min;
uniform vec3 max;
uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(position * (max - min) + min, 1.0);
}