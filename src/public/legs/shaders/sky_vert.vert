#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/vertex_p.glsl"
#include "include/ubo.glsl"

layout(location = 0) out vec3 fragDir;

void main()
{
    fragDir = normalize((transpose(ubo.view) * vec4(inPosition, 1.0)).xyz);
    gl_Position = ubo.proj * vec4(inPosition, 1.0);
}
