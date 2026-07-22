#pragma once

#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <glm/glm.hpp>


class VulkanEngine;

class ImGuiVulkanUtil {
public:
    ImGuiVulkanUtil(VulkanEngine* engine);
    ~ImGuiVulkanUtil();

    void Init(GLFWwindow* window, VkInstance instance, VkFormat colorFormat, uint32_t imageCount);
    void NewFrame();                                  // start ImGui frame
    void RenderDrawData(VkCommandBuffer commandBuffer); // draw into the active dynamic-rendering pass
    void Shutdown();

private:
    VulkanEngine* m_Engine = nullptr;
    //VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
    bool m_Initialized = false;
};

