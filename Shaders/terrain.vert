#version 450

layout(set = 0, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
    // padding + point lights array follow, matching your existing LightingUBO layout exactly
} lighting;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec4 fragPosLightSpace;
layout(location = 3) out vec3 fragWorldPos;

void main() {
    vec4 worldPos = vec4(inPosition, 1.0);   // already absolute world position — no model matrix needed
    gl_Position = lighting.viewProj * worldPos;

    fragNormal = inNormal;
    fragColor = inColor;
    fragPosLightSpace = lighting.lightSpaceMatrix * worldPos;
    fragWorldPos = worldPos.xyz;
}