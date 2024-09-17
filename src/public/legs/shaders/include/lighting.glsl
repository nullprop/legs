vec3 BlinnPhong(vec3 pos, vec3 normal, vec3 eyePos, float specularity)
{
    vec3 viewDir = normalize(eyePos - pos);
    vec3 halfDir = normalize(ubo.sunDir + viewDir);

    float sunMult = smoothstep(-0.6, 0.1, ubo.sunDir.z);
    vec3 sunColor = ubo.sunColor * sunMult;

    float diff = max(dot(normal, ubo.sunDir), 0.0);
    vec3 diffuse = diff * sunColor;

    float spec = pow(max(dot(normal, halfDir), 0.0), 16.0);
    vec3 specular = specularity * spec * sunColor;

    vec3 ambient = vec3(0.01);
    float horizonScatter = 1.0 - abs(ubo.sunDir.z);
    ambient += 0.08 * horizonScatter;
    ambient.r *= (1.0 + horizonScatter);

    return ambient + diffuse + specular;
}
