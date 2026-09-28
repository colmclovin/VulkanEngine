#version 450

layout(set = 0, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec4 fragPosLightSpace;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(lighting.sunDirection.xyz);
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = lighting.sunColor.a;

    // Shadow sampling omitted for the first pass — terrain rarely self-shadows meaningfully,
    // and this keeps the first GPU-driven test simpler. Can add shadow map sampling back later,
    // same technique as mesh.frag, once the core indirect-draw path is confirmed working.

    vec3 litColor = fragColor * lighting.sunColor.rgb * (ambient + diffuse * 0.8);
    outColor = vec4(litColor, 1.0);
}