#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/vertex_pc.glsl"
#include "include/ubo.glsl"

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
