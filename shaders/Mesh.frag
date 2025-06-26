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

    vec3 headlightPos;
    vec3 headlightDir;
    float headlightCutoff;
    float headlightOuterCutoff;
    float headlightIntensity;
    vec3 headlightColor;


} gubo;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    float amb;
    float gamma;
    vec3 sColor;
    mat4 mvpMat;
    mat4 mMat;
    mat4 nMat;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D tex;

void main() {
    vec3 N = normalize(fragNorm);
    vec3 V = normalize(gubo.eyePos - fragPos);
    vec3 L = normalize(gubo.DlightDir);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 albedo = texture(tex, fragUV).rgb;
    vec3 MD = albedo;
    vec3 MS = ubo.sColor;
    vec3 MA = albedo * ubo.amb;
    vec3 LA = gubo.AmbLightColor;

    vec3 diffuse = MD * NdotL;
    vec3 specular = MS * pow(NdotH, ubo.gamma);
    vec3 ambient = LA * MA;

    vec3 color = clamp(diffuse + specular + ambient, 0.0, 1.0);
    outColor = vec4(color, 1.0);

}