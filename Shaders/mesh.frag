#version 450

layout(set = 0, binding = 0) uniform sampler2D texSampler;   // existing texture binding, now explicit about set=0

layout(set = 1, binding = 0) uniform LightingData {
    vec4 sunDirection;
    vec4 sunColor;
} lighting;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec4 fragBaseColor;

layout(location = 0) out vec4 outColor;



void main() {
     vec3 lightDir = normalize(lighting.sunDirection.xyz);
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = lighting.sunColor.a;

    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 litColor = fragBaseColor.rgb * fragColor * texColor.rgb * (ambient + diffuse * 0.8);
    outColor = vec4(litColor, fragBaseColor.a * texColor.a);
}