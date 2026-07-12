#version 450

// Input from vertex shader
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in float fragTexIndex;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // For now, just output the vertex color
    // TODO: Add texture sampling based on fragTexIndex
    outColor = fragColor;
}