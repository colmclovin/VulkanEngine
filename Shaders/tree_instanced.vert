// Shaders/tree_instanced.vert
#version 450

layout(set = 0, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(std430, set = 1, binding = 0) readonly buffer InstanceBuffer {
    vec4 instances[];   // xyz = position, w = scale
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragPosLightSpace;

void main() {
vec4 instance = instances[gl_InstanceIndex];
vec3 worldPos = inPosition * 1.0 + instance.xyz;   // CHANGED — fixed scale of 1.0, since w is now the grid index

    gl_Position = lighting.viewProj * vec4(worldPos, 1.0);
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
    fragPosLightSpace = lighting.lightSpaceMatrix * vec4(worldPos, 1.0);
}