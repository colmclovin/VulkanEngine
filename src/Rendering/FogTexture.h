#pragma once
#include "../Engine/VulkanEngine.h"
#include "../Game/FogOfWar.h"
#include <glm/vec3.hpp>


class FogTexture {
public:
    static constexpr int SIZE = 512;
    void Init(VulkanEngine *engine);
    void Update(const FogOfWar &fog, glm::vec3 center); // call at ~10 Hz
    VkImageView GetView() const { return m_View; }
    VkSampler GetSampler() const { return m_Sampler; }
    glm::vec2 GetOrigin() const { return m_Origin; }
    float GetInvWorldSize() const { return 1.0f / (SIZE * FogOfWar::CELL); }
    bool WindowNeedsMove(glm::vec3 c) const {
        glm::vec2 center = m_Origin + glm::vec2(SIZE * FogOfWar::CELL * 0.5f);
        return glm::length(glm::vec2(c.x, c.z) - center) > 400.0f;
    }
    VkBuffer GetParamsBuffer() const { return m_ParamsBuf; }
    float GetTime() const { return m_Time; }
    void SetTime(float t);
    void Shutdown();

private:
    VulkanEngine *m_Engine = nullptr;
    VkImage m_Image{};
    VkDeviceMemory m_Mem{};
    VkImageView m_View{};
    VkSampler m_Sampler{};
    VkBuffer m_Staging{};
    VkDeviceMemory m_StagingMem{};
    uint8_t *m_Mapped = nullptr;
    glm::vec2 m_Origin{};
    VkBuffer m_ParamsBuf{};
    VkDeviceMemory m_ParamsMem{};
    glm::vec4 *m_ParamsMapped = nullptr;
    float m_Time = 0;

};