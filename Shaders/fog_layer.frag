#version 450
#extension GL_GOOGLE_include_directive : require
#include "Include/noise.glsl"

layout(set = 0, binding = 0) uniform sampler2D fogMap;
layout(set = 0, binding = 1) uniform FogParams { vec4 fog; } fogUbo;

layout(push_constant) uniform PC {
    mat4 viewProj;
    vec4 layer;
} pc;

layout(location = 0) in vec3 worldPos;
layout(location = 0) out vec4 outColor;

void main() {
    float f = texture(fogMap, (worldPos.xz - fogUbo.fog.xy) * fogUbo.fog.z).r;
    float unexplored = 1.0 - smoothstep(0.0, 0.5, f);

    float t = fogUbo.fog.w + pc.layer.z * 17.0;
    vec2 p = worldPos.xz * (0.035 + pc.layer.z * 0.012) + vec2(t * 0.05, -t * 0.03);
    float n = fbm(p, 4, 1.0, 2.0, 0.5) * 0.5 + 0.5;

    float edge = 4.0 * unexplored * (1.0 - unexplored);
    float density = smoothstep(0.35, 0.8, unexplored + (n - 0.5) * 0.9 * edge);

    vec3 col = mix(vec3(0.62, 0.68, 0.76), vec3(0.95, 0.97, 1.0), n);
    outColor = vec4(col, density * 0.55);
}