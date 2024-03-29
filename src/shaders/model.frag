#version 330 core

in vec3 normalCamSpace;

out vec4 outColor;

uniform vec4 color;

void main()
{
    vec3 ambientColor = vec3(0.1, 0.1, 0.1);
    vec3 lightColor = vec3(0.7, 0.7, 0.7);

    vec3 lightDirCamSpace = vec3(0.0, 0.0, -1.0);
    vec3 normal = normalize(normalCamSpace);
    float cosTheta = dot(normalCamSpace, -lightDirCamSpace);

    vec3 lightingTotal = ambientColor + lightColor * cosTheta;
    outColor = vec4(lightingTotal, 1.0) * color;
}