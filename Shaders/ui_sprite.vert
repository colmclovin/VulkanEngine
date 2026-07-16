#version 450

// Vertex shader for UI sprite rendering

layout(push_constant) uniform PushConstants {
	mat4 transform;
	vec4 color;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	gl_Position = push.transform * vec4(inPosition, 1.0);
	fragColor = push.color;
	fragTexCoord = inTexCoord;
}