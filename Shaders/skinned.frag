#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform LightingData {
    mat4 viewProj;
    vec4 sunDirection;
    vec4 sunColor;
    mat4 lightSpaceMatrix;
    int numPointLights;
} lighting;

layout(binding = 3) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec4 fragBaseColor;
layout(location = 4) in vec4 fragPosLightSpace;

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
    vec3 litColor = fragBaseColor.rgb * fragColor * texColor.rgb * lighting.sunColor.rgb * (ambient + diffuse * 0.8);
    outColor = vec4(litColor, fragBaseColor.a * texColor.a);
    //outColor = vec4(1.0, 0.0, 1.0, 1.0);   // solid magenta — impossible to miss if ANYTHING draws
}