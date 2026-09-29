#pragma once
#include <entt/entt.hpp>
#include "../Components/Components.h"
class VulkanEngine;
class MeshRenderer;

class BeltRenderSystem {
public:
    static void SyncItemEntities(entt::registry& registry, VulkanEngine* engine, MeshRenderer* meshRenderer, float gridSize);
    static void SyncLane(entt::registry& registry, BeltLane& lane, glm::vec3 beltPosition, glm::vec3 direction,
        float lateralOffset, VulkanEngine* engine, MeshRenderer* meshRenderer, float gridSize);
};