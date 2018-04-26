#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

out vec2 fragUV;

uniform mat4 pixelToClip;
uniform vec3 posBottomLeft;
uniform vec2 size;

void main()
{
    fragUV = uv;

    vec3 pixelPos = vec3(
        position.xy * size + posBottomLeft.xy,
        position.z + posBottomLeft.z);
    gl_Position = pixelToClip * vec4(pixelPos, 1.0);
}