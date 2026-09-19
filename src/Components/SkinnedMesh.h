#pragma once
#include "AnimationClip.h"
#include "Material.h"
#include "Skeleton.h"
#include "SkinnedVertex.h"
#include "SubMesh.h"
#include <vector>

class SkinnedMesh {
public:
    std::vector<SkinnedVertex> Vertices;
    std::vector<uint32_t> Indices;
    std::vector<SubMesh> SubMeshes;
    std::vector<Material> Materials;

    Skeleton skeleton;
    std::vector<AnimationClip> animations;

    // GPU resources — filled in during Stage B, unused for now
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    bool IsUploaded() const { return vertexBuffer != VK_NULL_HANDLE; }
    void UploadToGPU(VulkanEngine *engine);
    void DestroyGPUResources(VkDevice device);

    int FindClipIndex(const std::string &name) const {
        for (size_t i = 0; i < animations.size(); i++) {
            if (animations[i].name == name) return static_cast<int>(i);
        }
        return -1;
    }


};