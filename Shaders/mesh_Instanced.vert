// mesh_instanced.vert
#version 450

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

// Per-instance attributes
layout(location = 6) in vec4 instanceModel0;
layout(location = 7) in vec4 instanceModel1;
layout(location = 8) in vec4 instanceModel2;
layout(location = 9) in vec4 instanceModel3;
layout(location = 10) in vec4 instanceTint;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec4 fragBaseColor;
layout(location = 4) out vec4 fragPosLightSpace;
layout(location = 5) out vec3 fragWorldPos;

void main() {
    mat4 model = mat4(instanceModel0, instanceModel1, instanceModel2, instanceModel3);

    vec4 worldPos = model * vec4(inPosition, 1.0);
    gl_Position = lighting.viewProj * worldPos;

    fragNormal = mat3(model) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragBaseColor = instanceTint;
    fragPosLightSpace = lighting.lightSpaceMatrix * worldPos;
    fragWorldPos = worldPos.xyz;
}