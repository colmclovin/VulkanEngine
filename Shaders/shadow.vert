#version 450

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMVP;   // light's viewProj * model, computed per-entity on CPU
} pc;

layout(location = 0) in vec3 inPosition;
// no other attributes needed — we only care about depth

void main() {
    gl_Position = pc.lightSpaceMVP * vec4(inPosition, 1.0);
}