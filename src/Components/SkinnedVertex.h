#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <glm/glm.hpp>

struct SkinnedVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color;
    glm::ivec4 boneIndices = glm::ivec4(0);
    glm::vec4 boneWeights = glm::vec4(0.0f);

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(SkinnedVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

        attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, position) };
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, normal) };
        attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SkinnedVertex, texCoord) };
        attributeDescriptions[3] = { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(SkinnedVertex, color) };
        attributeDescriptions[4] = { 4, 0, VK_FORMAT_R32G32B32A32_SINT, offsetof(SkinnedVertex, boneIndices) };
        attributeDescriptions[5] = { 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(SkinnedVertex, boneWeights) };

        return attributeDescriptions;
    }
};