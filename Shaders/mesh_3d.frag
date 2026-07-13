#version 450

// Fragment shader for 3D mesh rendering

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
	// Simple directional lighting
	vec3 lightDir = normalize(vec3(0.5, -1.0, 0.3)); // Light from top-left
	vec3 normal = normalize(fragNormal);

	// Ambient + diffuse lighting
	float ambient = 0.3;
	float diffuse = max(dot(normal, -lightDir), 0.0) * 0.7;
	float lighting = ambient + diffuse;

	// Apply lighting to color
	vec3 finalColor = fragColor * lighting;

	outColor = vec4(finalColor, 1.0);
}
