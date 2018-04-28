#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

out vec2 fragUV;

uniform vec3 posBottomLeft;
uniform vec2 size;
uniform vec2 uvOrigin;
uniform vec2 uvSize;

void main()
{
    fragUV = uvOrigin + uv * uvSize;
    gl_Position = vec4(
        position.xy * size + posBottomLeft.xy,
        posBottomLeft.z,
        1.0);
}