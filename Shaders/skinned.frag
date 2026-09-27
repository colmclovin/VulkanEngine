#version 450

layout(binding = 1) uniform sampler2D texSampler;

struct PointLight {
    vec4 position;   // xyz = position, w = range
    vec4 color;      // rgb = color, w = intensity
};

layout(binding = 2) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
    PointLight pointLights[16];   // ADD — must match MAX_POINT_LIGHTS in your C++ LightingUBO
} lighting;

layout(binding = 3) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec4 fragBaseColor;
layout(location = 4) in vec4 fragPosLightSpace;
layout(location = 5) in vec3 fragWorldPos;   // ADD — needed for point light calculations

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

    // ADD — sum contributions from all active point lights
    vec3 pointLightContribution = vec3(0.0);
    for (int i = 0; i < lighting.numPointLights; i++) {
        vec3 lightPos = lighting.pointLights[i].position.xyz;
        float range = lighting.pointLights[i].position.w;
        vec3 lightColor = lighting.pointLights[i].color.rgb;
        float intensity = lighting.pointLights[i].color.w;

        vec3 toLight = lightPos - fragWorldPos;
        float dist = length(toLight);
        float attenuation = clamp(1.0 - (dist / range), 0.0, 1.0);
        attenuation *= attenuation;

        float pointDiffuse = max(dot(normal, normalize(toLight)), 0.0);
        pointLightContribution += lightColor * intensity * pointDiffuse * attenuation;
    }

    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 litColor = fragBaseColor.rgb * fragColor * texColor.rgb *
                    (lighting.sunColor.rgb * (ambient + diffuse * 0.8) + pointLightContribution);   // CHANGED
    outColor = vec4(litColor, fragBaseColor.a * texColor.a);
}