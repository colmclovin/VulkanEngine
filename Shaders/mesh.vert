#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
} pc;

layout(set = 1, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec4 fragBaseColor;
layout(location = 4) out vec4 fragPosLightSpace;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = lighting.viewProj * worldPos;

    fragNormal = mat3(pc.model) * inNormal;   // also fixes normal transform — previously missing entirely
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragBaseColor = pc.baseColor;
    fragPosLightSpace = lighting.lightSpaceMatrix * worldPos;
}