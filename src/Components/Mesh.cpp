#include "Mesh.h"
#include "../Engine/VulkanEngine.h"
#include <cstring>
#include <iostream>
Mesh Mesh::CreateTriangle() {
    Mesh mesh;
    mesh.Vertices = {
        { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 1.0f } },
        { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.5f, 1.5f } }
    };
    mesh.Indices = { 0, 1, 2 };

    return mesh;
}

void Mesh::UploadToGPU(VulkanEngine* engine) {
    if (IsUploaded()) return;

    VkDevice device = engine->GetDevice();

    // --- Vertex buffer ---
    VkDeviceSize vertexBufferSize = sizeof(Vertices[0]) * Vertices.size();
    VkBuffer vertexStagingBuffer; VkDeviceMemory vertexStagingMemory;
    engine->CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexStagingBuffer, vertexStagingMemory);

    void* data;
    vkMapMemory(device, vertexStagingMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, Vertices.data(), vertexBufferSize);
    vkUnmapMemory(device, vertexStagingMemory);

    engine->CreateBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    engine->CopyBuffer(vertexStagingBuffer, vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
    vkFreeMemory(device, vertexStagingMemory, nullptr);

    // --- Index buffer ---
    VkDeviceSize indexBufferSize = sizeof(Indices[0]) * Indices.size();
    VkBuffer indexStagingBuffer; VkDeviceMemory indexStagingMemory;
    engine->CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indexStagingBuffer, indexStagingMemory);

    vkMapMemory(device, indexStagingMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, Indices.data(), indexBufferSize);
    vkUnmapMemory(device, indexStagingMemory);

    engine->CreateBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    engine->CopyBuffer(indexStagingBuffer, indexBuffer, indexBufferSize);

    vkDestroyBuffer(device, indexStagingBuffer, nullptr);
    vkFreeMemory(device, indexStagingMemory, nullptr);

    std::cout << "Mesh uploaded to GPU" << std::endl;
}

void Mesh::DestroyGPUResources(VkDevice device) {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
}
