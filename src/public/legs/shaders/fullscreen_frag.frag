#version 450

#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 outUV;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
}
