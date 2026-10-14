#version 450

layout(push_constant) uniform PC {
    mat4 viewProj;
    vec4 layer;      // x = height, y = unused, z = layer index, w = unused
} pc;

layout(set = 0, binding = 1) uniform FogParams { vec4 fog; } fogUbo;   // xy = origin, z = invSize, w = time

layout(location = 0) out vec3 worldPos;

void main() {
    // four corners of a triangle strip: (0,0) (1,0) (0,1) (1,1)
    vec2 c = vec2(float(gl_VertexIndex & 1), float(gl_VertexIndex >> 1));
    float worldSize = 1.0 / fogUbo.fog.z;
    vec2 xz = fogUbo.fog.xy + c * worldSize;
    worldPos = vec3(xz.x, pc.layer.x, xz.y);
    gl_Position = pc.viewProj * vec4(worldPos, 1.0);
}