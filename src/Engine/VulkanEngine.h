#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <optional>
#include <string>

class VulkanEngine {
public:
    VulkanEngine(); //constructor
    ~VulkanEngine(); //destructor

    void Init(const char *appName = "Vulkan App", uint32_t width = 1280, uint32_t height = 720);
    void Shutdown();

    bool BeginFrame(); // Returns false if should skip frame
    void EndFrame();
    bool ShouldClose() const;
    void PollEvents() const;

    // Getters for device and resources
    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    VkCommandPool GetCommandPool() const { return m_CommandPool; }
    VkRenderPass GetRenderPass() const { return m_RenderPass; }
    VkExtent2D GetSwapChainExtent() const { return m_SwapChainExtent; }
    VkFormat GetSwapChainFormat() const { return m_SwapChainImageFormat; }
    GLFWwindow *GetWindow() const { return m_Window; }
    uint32_t GetGraphicsQueueFamily() { return FindQueueFamilies(m_PhysicalDevice).graphicsFamily.value(); }  // NEW
    VkCommandBuffer GetCurrentCommandBuffer() const;
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }
    uint32_t GetSwapChainImageCount() const;
    VkFormat GetDepthFormat() const { return m_DepthFormat; }
    VkImageView GetDepthImageView() const { return m_DepthImageView; }

    // Utility functions for creating resources
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer &buffer, VkDeviceMemory &bufferMemory);

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkShaderModule CreateShaderModule(const std::vector<char> &code);
    std::vector<char> ReadFile(const std::string &filename);

private:
    //GLFW variables
    GLFWwindow *m_Window = nullptr;
    uint32_t m_WindowWidth = 1280;
    uint32_t m_WindowHeight = 720;
    bool m_FramebufferResized = false;
    //Vulkan variables
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;
    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;

    // Command buffers
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    // Render pass
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;

    // Synchronization
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    uint32_t m_CurrentFrame = 0;
    uint32_t m_CurrentImageIndex = 0;

    VkImage m_DepthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
    VkImageView m_DepthImageView = VK_NULL_HANDLE;
    VkFormat m_DepthFormat = VK_FORMAT_D32_SFLOAT;




    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    // Validation layers
    const std::vector<const char *> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char *> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool m_EnableValidationLayers = false;
#else
    const bool m_EnableValidationLayers = true;
#endif

    // Init functions
    void CreateWindow(const char *title);
    void CreateInstance(const char *appName);
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void RecreateSwapChain();
    void CleanupSwapChain();
    void CreateImageViews();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CreateDepthResources();



    //void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    // Helper structures
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    // Helper functions
    bool CheckValidationLayerSupport();
    std::vector<const char *> GetRequiredExtensions();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *pUserData);

    // Static helper for creating debug messenger
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkDebugUtilsMessengerEXT *pDebugMessenger);

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                              VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator);

 
};