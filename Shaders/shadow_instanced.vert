#version 450

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
} pc;

layout(location = 0) in vec3 inPosition;

layout(location = 1) in vec4 instanceModel0;
layout(location = 2) in vec4 instanceModel1;
layout(location = 3) in vec4 instanceModel2;
layout(location = 4) in vec4 instanceModel3;

void main() {
    mat4 model = mat4(instanceModel0, instanceModel1, instanceModel2, instanceModel3);
    gl_Position = pc.lightSpaceMatrix * model * vec4(inPosition, 1.0);
}