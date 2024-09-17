#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/vertex_pnc.glsl"
#include "include/ubo.glsl"
#include "include/lighting.glsl"

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    
    vec3 position = (ubo.model * vec4(inPosition, 1.0)).xyz;
    vec3 normal = (ubo.model * vec4(inNormal, 1.0)).xyz;
    vec3 light = BlinnPhong(gl_Position.xyz, normal, ubo.eye, 1.0);
    fragColor = inColor * light;
}
