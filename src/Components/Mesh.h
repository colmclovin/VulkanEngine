#pragma once

#include "Vertex.h"
#include <vector>
#include "SubMesh.h"
#include "Material.h"
class VulkanEngine;

class Mesh {

public:
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    std::vector<SubMesh> SubMeshes;      // NEW
    std::vector<Material> Materials;     // NEW

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    static Mesh CreateTriangle();
    void UploadToGPU(VulkanEngine* engine);
    void DestroyGPUResources(VkDevice device);   // NEW — see note below
    bool IsUploaded() const { return vertexBuffer != VK_NULL_HANDLE; }

};