#version 330 core

in vec2 fragUV;

out vec4 outColor;

uniform sampler2D textureSampler;

void main()
{
    outColor.rgb = texture(textureSampler, fragUV).rgb;
    outColor.a = 1.0;
}