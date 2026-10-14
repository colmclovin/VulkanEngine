#version 450
#extension GL_GOOGLE_include_directive : require
#include "Include/fogfx.glsl"

layout(set = 0, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;
layout(set = 0, binding = 2) uniform sampler2D fogMap;
layout(set = 0, binding = 3) uniform FogParams { vec4 fog; } fogUbo;   // xy = origin, z = invSize, w = time
layout(set = 2, binding = 0) uniform sampler2D treeTexture;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosLightSpace;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

float CalculateShadow(vec4 posLS) {
    vec3 projCoords = posLS.xyz / posLS.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 1.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    return (projCoords.z - 0.005) > closestDepth ? 0.0 : 1.0;
}

void main() {
    vec4 texColor = texture(treeTexture, fragTexCoord);
    if (texColor.a < 0.1) discard;

    vec3 lightDir = normalize(lighting.sunDirection.xyz);
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = lighting.sunColor.a;
    diffuse *= CalculateShadow(fragPosLightSpace);

    vec3 litColor = texColor.rgb * lighting.sunColor.rgb * (ambient + diffuse * 0.8);

    float f = texture(fogMap, (fragWorldPos.xz - fogUbo.fog.xy) * fogUbo.fog.z).r;
    litColor = applyFog(litColor, fragWorldPos, f, fogUbo.fog.w);

    outColor = vec4(litColor, texColor.a);
}