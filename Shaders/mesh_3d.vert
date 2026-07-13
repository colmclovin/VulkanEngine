#version 450

// Vertex shader for 3D mesh rendering

layout(binding = 0) uniform CameraUBO {
	mat4 view;
	mat4 projection;
} camera;

layout(push_constant) uniform PushConstants {
	mat4 model;
	vec4 color;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;

void main() {
	vec4 worldPos = push.model * vec4(inPosition, 1.0);
	fragWorldPos = worldPos.xyz;

	gl_Position = camera.projection * camera.view * worldPos;

	// Mix vertex color with material color
	fragColor = inColor * push.color.rgb;

	// Transform normal to world space (assumes uniform scale)
	fragNormal = mat3(push.model) * inNormal;
}
