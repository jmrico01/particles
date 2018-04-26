#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;

out vec3 normalCamSpace;
out vec3 vertColor;

void main()
{
    gl_Position = mvp * vec4(position, 1.0);
    normalCamSpace = (view * model * vec4(normal, 0.0)).xyz;
    vertColor = color;
}