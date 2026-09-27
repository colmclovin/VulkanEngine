#version 450

layout(push_constant) uniform PushConstants {
    vec4 baseColor;
} pc;

layout(set = 0, binding = 0) uniform sampler2D texSampler;

layout(set = 1, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(set = 1, binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec4 fragBaseColor;   // still comes from the vertex shader's instanceTint — now effectively unused, or combine both
layout(location = 4) in vec4 fragPosLightSpace;
layout(location = 5) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

float CalculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}

void main() {
    vec3 lightDir = normalize(lighting.sunDirection.xyz);
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = lighting.sunColor.a;

    float shadow = CalculateShadow(fragPosLightSpace);
    diffuse *= shadow;

    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 litColor = pc.baseColor.rgb * fragColor * texColor.rgb * lighting.sunColor.rgb * (ambient + diffuse * 0.8);   // CHANGED — pc.baseColor instead of fragBaseColor
    outColor = vec4(litColor, pc.baseColor.a * texColor.a);   // CHANGED
}