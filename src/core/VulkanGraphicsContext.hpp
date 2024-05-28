#ifndef CORE_VULKAN_GRAPHICS_CONTEXT_H
#define CORE_VULKAN_GRAPHICS_CONTEXT_H

// libs
#include <vulkan/vulkan.hpp>

// std
#include <span>

struct SDL_Window;
struct VmaAllocator_T;

namespace core
{

/*!
 * @brief RAII Class that manages common resources in a Vulkan Graphics application.
 *
 * This class automatically loads the necessary Vulkan entrypoints using the DispatchLoaderDynamic class provided by
 * Vulkan.hpp. It ensures that all required entrypoints, including device-specific extensions, are available for Vulkan
 * function calls.
 *
 * NOTE: you MUST use the vulkan.hpp default dispatcher function ptrs to have access to those entrypoints.
 * By default, all vulkan.hpp functions uses the default dispatcher.
 *
 * Managed Resources:
 * - A VkInstance handle, including optional debug messenger and validation layer setup.
 * - A VkSurfaceKHR handle, which abstract native platform surface or window objects for use with Vulkan.
 * - A VkSwapchainKHR handle, to present to the VkSurfaceKHR.
 * - A single VkDevice handle, that supports the VK_KHR_swapchain extension, with a single graphics queue and present
 * queue.
 * - A VMA (Vulkan Memory Allocator) handle, for memory allocation management.
 *
 * For more details about a managed resource, check the documentation of the private methods
 * @ref createInstanceAndDebug, @ref searchPhysicalDevice, @ref createLogicalDevice, @ref createAllocator and
 * @ref createSwapchain.
 */
class VulkanGraphicsContext
{
public:
    /*!
     * Default constructor. Does not acquire any resource.
     */
    VulkanGraphicsContext() noexcept = default;

    /*!
     * @brief Constructor that acquires the resources.
     *
     * @param vulkanApiVersion 0 or the highest version of Vulkan that the application is designed to use.
     * - This MUST be encoded with the vulkan version numbers, which can be achieved
     *   by using vk::makeApiVersion(v, M, m, p).
     * - The Khronos validation layers will treat apiVersion as the highest API version of the application targets,
     *   and will validate API usage against the minimum of that version and the implementation version.
     * - If vulkanApiVersion is not 0, then it MUST be greater than or equal to VK_API_VERSION_1_0.
     * - The variant version of the instance MUST match that requested in vulkanApiVersion.
     *
     * @param requiredInstanceExtensions array-view of null-terminated UTF-8 strings containing the names of extensions
     * to enable for the created instance.
     * - This MUST include "VK_KHR_surface" and the specific platform surface extension, such as "VK_KHR_win32_surface"
     *   or "VK_KHR_xcb_surface".
     *
     * @param enableValidationLayersIfSupported a flag to enable validation layers, if supported by the Vulkan instance.
     * - If this flag is true and the validation layers are not supported, the instance creation will proceed without
     *   requesting the validation layers.
     *
     * @param enableDebugMessengerIfSupported a flag to create a VkDebugUtilsMessengerEXT instance, if supported by the
     * Vulkan instance.
     * - If this flag is true and the debug utils messenger extension is not supported, the instance creation will
     *   proceed without requesting the extension.
     * - If "VK_EXT_debug_utils" is within "requiredInstanceExtensions", this flag will effectively have no effect, and
     *   the instance creation will fail if it does not support the Debug Utils Messenger extension.
     *
     * @param window A valid SDL_window, which MUST have been created with the "SDL_WINDOW_VULKAN" flag.
     *
     * @throws a vk::SystemError containing the error information.
     * This can be because:
     * - Vulkan is not supported by the machine,
     * - the required extensions are not supported,
     * - no physical devices are suitable for the application needs,
     * - internal issues with the host/device Vulkan calls, such as being out of memory.
     *
     * If the creation fails, all acquired resources are released.
     */
    VulkanGraphicsContext(uint32_t vulkanApiVersion,
                          std::span<const char* const> requiredInstanceExtensions,
                          bool enableValidationLayersIfSupported,
                          bool enableDebugMessengerIfSupported,
                          SDL_Window* window);

    VulkanGraphicsContext(const VulkanGraphicsContext&) = delete;
    VulkanGraphicsContext& operator=(const VulkanGraphicsContext&) = delete;

    VulkanGraphicsContext(VulkanGraphicsContext&&) noexcept;
    VulkanGraphicsContext& operator=(VulkanGraphicsContext&&) noexcept;

    ~VulkanGraphicsContext() noexcept;

    /*!
     * Sets the present mode to be used in the next @ref recreateSwapchain call.
     * NOTE: if the present mode is not supported, @ref recreateSwapchain will fail.
     */
    void setSwapchainPresentMode(vk::PresentModeKHR presentModeKHR) noexcept
    {
        m_currentSwapchainPresentMode = presentModeKHR;
    }

    /*!
     * Sets the surface format to be used in the next @ref recreateSwapchain call.
     * NOTE: if the surface format is not supported, @ref recreateSwapchain will fail.
     */
    void setSwapchainSurfaceFormatKHR(vk::SurfaceFormatKHR surfaceFormatKHR) noexcept
    {
        m_currentSwapchainSurfaceFormat = surfaceFormatKHR;
    }

    /*!
     * @brief Re-Creates the swapchain.
     *
     * Re-creates the swapchain, querying the current drawable extent from the window and using the
     * set values for the swapchain present mode and surfaceFormatKHR.
     *
     * @param window the SDL_window bound to the VkSurface, to query for the drawable extent.
     *
     * @throws a vk::SystemError if the swapchain recreation failed.
     */
    void recreateSwapchain(SDL_Window* window);

private:
    struct QueueFamiliesIndices {
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;
    };

    /*!
     * @brief Creates a Vulkan instance and optionally sets up a debug messenger.
     *
     * This function initializes a Vulkan instance with specified parameters,
     * and optionaly creates a debug utils messenger ext instance.
     *
     * This call loads the vkGetInstanceProcAddr ptr, and after creating the instance, if successfull, loads all other
     * function pointers.
     *
     * @param vulkanApiVersion 0 or the highest version of Vulkan that the application is designed to use.
     * - This MUST be encoded with the vulkan version numbers, which can be achieved
     *   by using vk::makeApiVersion(v, M, m, p).
     * - The Khronos validation layers will treat apiVersion as the highest API version of the application targets,
     *   and will validate API usage against the minimum of that version and the implementation version.
     * - If vulkanApiVersion is not 0, then it MUST be greater than or equal to VK_API_VERSION_1_0.
     * - The variant version of the instance MUST match that requested in vulkanApiVersion.
     *
     * @param requiredInstanceExtensions array-view of null-terminated UTF-8 strings containing the names of extensions
     * to enable for the created instance.
     * - This MUST include "VK_KHR_surface" and the specific platform surface extension, such as "VK_KHR_win32_surface"
     *   or "VK_KHR_xcb_surface".
     *
     * @param enableValidationLayersIfSupported Flag to enable validation layers, if supported.
     *
     * @param enableDebugMessengerIfSupported Flag to enable debug messenger, if supported.
     * - If "VK_EXT_debug_utils" is within "requiredInstanceExtensions", this flag will effectively have no effect, and
     *   the instance creation will fail if it does not support the Debug Utils Messenger extension.
     * - The Debug Callback is set with severity flags {Verbose, Info, Warning, Error}, and with Message type flags
     *   {General, Validation, Performance}.
     * - The Debug Callback calls the core::Logger implementation, to log verbose with level TRACE, info with level
     *   INFO, warning with level WARN and error with level ERROR.
     * - If this flag is true, the instance creation and destruction events will also be captured by linking the
     *   VkDebugUtilsMessengerCreateInfoEXT in the pNext.
     *
     * @throws a vk::SystemError if instance or debug creation fails.
     */
    void createInstanceAndDebug(uint32_t vulkanApiVersion,
                                std::span<const char* const> requiredInstanceExtensions,
                                bool enableValidationLayersIfSupported,
                                bool enableDebugMessengerIfSupported);

    /*!
     * @brief Searches for a physical device that suits the application needs.
     *
     * Searches for the first physical device that:
     * - has a graphics queue family,
     * - has a queue family that supports presenting to the VkSurface,
     * - supports the VK_KHR_swapchain extension.
     *
     * @throws a vk::SystemError if none devices available matches the criteria.
     */
    void searchPhysicalDevice();

    /*!
     * @brief Creates the VkDevice handle for the logical device.
     *
     * Requests a single queue from the graphics queue amily and a single queue from the present queue family,
     * and enables the  VK_KHR_swapchain extension.
     *
     * After this call, if successfull, the device-specific function entrypoints are loaded.
     *
     * NOTE: A queue family can support both the graphics queue and the present queue. In this case, only one queue will
     * be created from this family, which will be used for both purposes.
     *
     * @throws a vk::SystemError if the device creation failed.
     */
    void createLogicalDevice();

    /*!
     * Creates a VMA allocator instance.
     *
     * @throws a vk::SystemError if the allocator creation failed.
     */
    void createAllocator();

    /*!
     * @brief Creates a VkSwapchainKHR to present to the window surface.
     *
     * The default values set for the present mode and surfaceFormatKHR are:
     * - If supported, the present mode is set to Mailbox. Otherwise, it defaults to FIFO.
     * - If supported, the vKsurfaceFormatKHR is set to {.format = eB8G8R8A8Srgb, .colorSpace = eSrgbNonlinear}.
     *   Otherwise it defaults to the first VkSurfaceFormatKHR enumerated by the physical device.
     *
     * If the device graphics queue family is the same as the present queue family,
     * the image sharing mode is set to eExclusive, otherwise to eConcurrent.
     *
     * @param window the SDL_window bound to the VkSurface, to query for the drawable extent.
     *
     * @throws a vk::SystemError if the swapchain creation failed.
     */
    void createSwapchain(SDL_Window* window);

    /*!
     * Frees the acquired resources.
     */
    void cleanup() noexcept;

private:
    // PhysicalDevice is owned by instance and Queues are owned by the device
    // We shouldn't need to initialize them, and a copy on move should be enough
    // instead of exchange/swap, but just to be safe and catch something off on assertions
    // they are initialized as nullptr

    vk::Instance m_instance = nullptr;
    vk::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
    vk::SurfaceKHR m_surface = nullptr;
    vk::PhysicalDevice m_physicalDevice = nullptr;
    QueueFamiliesIndices m_queueFamiliesIndices;
    vk::Device m_device = nullptr;
    vk::Queue m_graphicsQueue = nullptr;
    vk::Queue m_presentQueue = nullptr;
    VmaAllocator_T* m_allocator = nullptr;
    vk::PresentModeKHR m_currentSwapchainPresentMode;
    vk::SurfaceFormatKHR m_currentSwapchainSurfaceFormat;
    vk::SwapchainKHR m_swapchain = nullptr;
};

}   // namespace core

#endif
