#include "ImGuiVulkanUtil.h"
#include "../Engine/VulkanEngine.h"
#include <vulkan/vulkan.h>
#include <stdexcept>


ImGuiVulkanUtil::ImGuiVulkanUtil(VulkanEngine* engine) {
    m_Engine = engine;
}
ImGuiVulkanUtil::~ImGuiVulkanUtil() {

}

// Core functionality methods for ImGui integration
void ImGuiVulkanUtil::Init(GLFWwindow* window, VkInstance instance, VkFormat colorFormat, uint32_t imageCount) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);

    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;
    pipelineRenderingInfo.depthAttachmentFormat = m_Engine->GetDepthFormat();

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_3;
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = m_Engine->GetPhysicalDevice();
    initInfo.Device = m_Engine->GetDevice();
    initInfo.QueueFamily = m_Engine->GetGraphicsQueueFamily();
    initInfo.Queue = m_Engine->GetGraphicsQueue();
    initInfo.DescriptorPoolSize = 8;        // backend creates+owns its own correctly-sized pool
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = imageCount;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingInfo;

    ImGui_ImplVulkan_Init(&initInfo);

    m_Initialized = true;
}

void ImGuiVulkanUtil::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiVulkanUtil::RenderDrawData(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}
void ImGuiVulkanUtil::Shutdown() {
    if (!m_Initialized) return;
    vkDeviceWaitIdle(m_Engine->GetDevice());

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_Initialized = false;
}
