vec4 FragClipPos()
{
    vec4 ndc;
    ndc.xy = (2.0 * gl_FragCoord.xy / ubo.viewport.xy) - 1.0;
    ndc.z = (2.0 * gl_FragCoord.z) - 1.0;
    ndc.w = 1.0;
    vec4 clip = ndc / gl_FragCoord.w;
    return clip;
}

vec3 FragWorldPos()
{
    return (ubo.clipToWorld * FragClipPos()).xyz;
}
