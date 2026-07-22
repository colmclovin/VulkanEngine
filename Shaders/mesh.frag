#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple fixed-direction lighting so you can actually see shape/depth,
    // not just a flat silhouette. Swap for real lighting later.
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = normalize(fragNormal);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = 0.2;

    vec3 litColor = fragColor * (ambient + diffuse * 0.8);
    outColor = vec4(litColor, 1.0);
}