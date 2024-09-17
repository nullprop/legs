#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/ubo.glsl"
#include "include/util.glsl"

layout(location = 0) in vec3 fragDir;

layout(location = 0) out vec4 outColor;

void main()
{
    float sunDot = dot(fragDir, ubo.sunDir);

    vec3 darkColor = vec3(0.05, 0.05, 0.05);
    vec3 baseColor = vec3(0.1, 0.2, 0.7);
    vec3 brightColor = vec3(1.0, 1.0, 1.0);

    float dayAmount = 0.01 + smoothstep(-1.0, 1.0, ubo.sunDir.z);
    darkColor.rgb *= dayAmount;
    baseColor.rgb *= dayAmount;
    brightColor.rgb *= dayAmount;

    float horizonAmount = 3.0 * smoothstep(0.9, -0.4, ubo.sunDir.z);
    vec3 horizonColor = 1.0 + vec3(horizonAmount, 0.0, -0.4 * horizonAmount);
    darkColor.rgb *= horizonColor;
    baseColor.rgb *= horizonColor;
    brightColor.rgb *= horizonColor;

    float darkMix = smoothstep(0.0, -1.0, sunDot);

    float brightMix = sign(sunDot) * pow(sunDot, 4);
    brightMix = smoothstep(0.0, 1.0, brightMix);

    vec3 gradient = mix(baseColor, darkColor, darkMix)
            + mix(baseColor, brightColor, brightMix);

    outColor = vec4(gradient, 1.0);
}
