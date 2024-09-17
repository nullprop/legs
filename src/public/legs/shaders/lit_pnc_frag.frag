#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/ubo.glsl"
#include "include/lighting.glsl"

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor, 1.0);
}
