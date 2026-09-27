// shadow_skinned.vert
#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 lightSpaceMatrix;
} pc;

layout(binding = 0) uniform BoneMatrices {
    mat4 boneMatrices[64];
} bones;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in ivec4 inBoneIndices;
layout(location = 5) in vec4 inBoneWeights;

void main() {
    mat4 skinMatrix =
        bones.boneMatrices[inBoneIndices.x] * inBoneWeights.x +
        bones.boneMatrices[inBoneIndices.y] * inBoneWeights.y +
        bones.boneMatrices[inBoneIndices.z] * inBoneWeights.z +
        bones.boneMatrices[inBoneIndices.w] * inBoneWeights.w;

    vec4 skinnedPos = skinMatrix * vec4(inPosition, 1.0);
    vec4 worldPos = pc.model * skinnedPos;

    gl_Position = pc.lightSpaceMatrix * worldPos;
}