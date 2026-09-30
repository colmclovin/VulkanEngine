// Shaders/tree_instanced.frag
#version 450

layout(set = 0, binding = 0) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;
layout(set = 2, binding = 0) uniform sampler2D treeTexture;   // NEW — set 2, changes per submesh draw

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragPosLightSpace;

layout(location = 0) out vec4 outColor;

float CalculateShadow(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 1.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    return (projCoords.z - 0.005) > closestDepth ? 0.0 : 1.0;
}

void main() {
    vec4 texColor = texture(treeTexture, fragTexCoord);
    if (texColor.a < 0.1) discard;   // handle leaf-alpha-cutout textures, common for foliage

    vec3 lightDir = normalize(lighting.sunDirection.xyz);
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = lighting.sunColor.a;

    float shadow = CalculateShadow(fragPosLightSpace);
    diffuse *= shadow;

    vec3 litColor = texColor.rgb * lighting.sunColor.rgb * (ambient + diffuse * 0.8);
    outColor = vec4(litColor, texColor.a);
}