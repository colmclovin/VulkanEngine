#version 450

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
} pc;

layout(std430, set = 0, binding = 0) readonly buffer InstanceBuffer {
    vec4 instances[];
};

layout(location = 0) in vec3 inPosition;

void main() {
    vec4 instance = instances[gl_InstanceIndex];
    vec3 worldPos = inPosition + instance.xyz;   // fixed scale of 1.0, matching tree_instanced.vert
    gl_Position = pc.lightSpaceMatrix * vec4(worldPos, 1.0);
}