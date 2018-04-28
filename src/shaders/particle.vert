#version 330 core

layout(location = 0) in vec3 squareVerts;
layout(location = 1) in vec2 uvs;
layout(location = 2) in vec3 center;

out vec2 fragUV;

uniform vec2 size;
uniform vec3 camRight;
uniform vec3 camUp;
uniform mat4 vp;

void main()
{
    vec3 worldPos = center
        + camRight * squareVerts.x * size.x
        + camUp * squareVerts.y * size.y;
    gl_Position = vp * vec4(worldPos, 1.0);

    fragUV = uvs;
}