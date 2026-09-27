// shadow_skinned.frag
#version 450

void main() {
    // Depth-only pass — no color output needed.
    // gl_FragDepth is written implicitly from gl_Position.z via the fixed-function depth test/write.
}