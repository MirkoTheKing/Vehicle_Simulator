#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNorm;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniformBufferObject {
    vec3 DlightDir;
    vec3 DlightColor;
    vec3 AmbLightColor;
    vec3 eyePos;
} gubo;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    float amb;
    float gamma;
    float metalic;
    float roughness;
    vec3 baseColor;
    vec3 sColor;
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D tex;


vec3 fresnel_term(float cosTheta, vec3 F0 ){
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distribution(float NdotH, float roughness){
    float a = roughness;
    float a2 = a * a;
    float NdotH_clamped = clamp(NdotH, 0.0, 1.0);
    float denom = (NdotH_clamped * NdotH_clamped) * (a2 - 1.0) + 1.0;
    return a2 / (3.141592 * denom * denom);
}

float G_Smith(float NdotV, float NdotL, float NdotH, float VdotH) {
    float term1 = (2.0 * NdotH * NdotV) / VdotH;
    float term2 = (2.0 * NdotH * NdotL) / VdotH;
    return min(1.0, min(term1, term2));
}


void main() {
    vec3 N = normalize(fragNorm);
    vec3 V = normalize(gubo.eyePos - fragPos);
    vec3 L = normalize(gubo.DlightDir);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    vec3 albedo = texture(tex, fragUV).rgb;
    vec3 baseColor = albedo;
    float metalic = ubo.metalic;
    float roughness = ubo.roughness;

    vec3 F0 = mix(vec3(0.04), baseColor,metalic);

    vec3 F = fresnel_term(VdotH, F0);

    float D = distribution(NdotH, roughness);

    float G = G_Smith(NdotV, NdotL, NdotH, VdotH);

    vec3 specular = (D*F*G) / max(4.0 * NdotV *NdotL,0.001);

    vec3 kd = (1.0 - F) * (1.0 - metalic);
    vec3 diffuse = kd * baseColor / 3.141592;

    vec3 lightColor = gubo.DlightColor;
    vec3 radiance = lightColor * NdotL;

    vec3 color = (diffuse + specular) * radiance;

    // Ambient
    vec3 ambient = gubo.AmbLightColor * baseColor * ubo.amb;
    color += ambient;

    // Gamma correction
    color = pow(color, vec3(1.0 / ubo.gamma));

    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);

}