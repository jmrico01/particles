#version 330 core

#define PLANE_SIZE 50.0f

layout(location = 0) in vec3 position;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(position * PLANE_SIZE, 1.0);
}