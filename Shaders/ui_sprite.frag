#version 450

// Fragment shader for UI sprite rendering

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	// Simple solid color rendering (can add texture sampling later)
	outColor = fragColor;
}
