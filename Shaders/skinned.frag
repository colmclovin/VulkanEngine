// skinned.frag
#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec4 fragBaseColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = 0.2;

    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 litColor = fragBaseColor.rgb * fragColor * texColor.rgb * (ambient + diffuse * 0.8);
    outColor = vec4(litColor, fragBaseColor.a * texColor.a);
}