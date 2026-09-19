// skinned.vert
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 baseColor;
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

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec4 fragBaseColor;

void main() {
    mat4 skinMatrix =
        bones.boneMatrices[inBoneIndices.x] * inBoneWeights.x +
        bones.boneMatrices[inBoneIndices.y] * inBoneWeights.y +
        bones.boneMatrices[inBoneIndices.z] * inBoneWeights.z +
        bones.boneMatrices[inBoneIndices.w] * inBoneWeights.w;

    vec4 skinnedPos = skinMatrix * vec4(inPosition, 1.0);
    gl_Position = pc.mvp * skinnedPos;

    fragNormal = mat3(skinMatrix) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragBaseColor = pc.baseColor;
}