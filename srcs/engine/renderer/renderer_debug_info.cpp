#include "engine/renderer/renderer.hpp"
#include "config.h"

#include "engine/vk/context.h"
#include "engine/vk/swapchain/swapchain.h"
#include "engine/vk/swapchain/depth.h"
#include "engine/vk/swapchain/frames.h"
#include "engine/vk/resources/texture_mipmap.h"

#include <string>

namespace {

const char *present_mode_name(VkPresentModeKHR m)
{
    switch (m) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:         return "IMMEDIATE";
        case VK_PRESENT_MODE_MAILBOX_KHR:           return "MAILBOX";
        case VK_PRESENT_MODE_FIFO_KHR:              return "FIFO";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:      return "FIFO_RELAXED";
        default:                                    return "UNKNOWN";
    }
}

const char *format_name(VkFormat f)
{
    switch (f) {
        case VK_FORMAT_B8G8R8A8_UNORM:          return "B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_SRGB:           return "B8G8R8A8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM:          return "R8G8B8A8_UNORM";
        case VK_FORMAT_D32_SFLOAT:              return "D32_SFLOAT";
        default:                                return "OTHER";
    }
}

const char *device_type_name(VkPhysicalDeviceType t)
{
    switch (t) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:      return "discrete";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:    return "integrated";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:       return "virtual";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:               return "cpu";
        default:                                        return "other";
    }
}

std::string vulkan_api_version(uint32_t v)
{
    return std::to_string(VK_API_VERSION_MAJOR(v)) + "." + std::to_string(VK_API_VERSION_MINOR(v)) + "." + std::to_string(VK_API_VERSION_PATCH(v));
}

std::string instance_extensions_line()
{
    std::string exts                = "Extensions: VK_KHR_swapchain, dynamicRendering(1.3)";
    unsigned int glfw_ext_count     = 0;
    const char **glfw_exts          = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    for (unsigned int i = 0; i < glfw_ext_count; ++i) 
        exts                        += std::string(", ") + glfw_exts[i];
    
    if (g_debug_messenger != VK_NULL_HANDLE)
        exts                        += ", VK_EXT_debug_utils (validation on)";
    return exts;
}

} // namespace

/* Built once at startup (nothing here changes at runtime) and reused every frame the panel is shown. */
void Renderer::_build_device_info_lines(int graphics_family, int present_family)
{
    _device_info_lines.clear();

    VkPhysicalDeviceProperties          props{};
    if (g_physical != VK_NULL_HANDLE)   vkGetPhysicalDeviceProperties(g_physical, &props);

    // GPU name
    _device_info_lines.push_back(std::string("GPU: ") + props.deviceName + "  (" + device_type_name(props.deviceType) + ")");
    
    // Driver and Vulkan API version, graphics/present queue families
    _device_info_lines.push_back("Driver: " + std::to_string(props.driverVersion) \
                                + "  Vulkan: " + vulkan_api_version(props.apiVersion) \
                                + "  Queues: graphics=" + std::to_string(graphics_family) \
                                + " present=" + std::to_string(present_family));
    
    // Instance extensions (GLFW + debug utils if validation is on)
    _device_info_lines.push_back(instance_extensions_line());
    
    // Swapchain format, image count, present mode, extent
    _device_info_lines.push_back("Swapchain: " + std::string(format_name(g_swap_image_format)) + ", " \
                                + std::to_string(g_image_count) + " images, " \
                                + present_mode_name(g_swap_present_mode) + ", " \
                                + std::to_string(g_swap_extent.width) + "x" \
                                + std::to_string(g_swap_extent.height)); \
    
    // Depth format, frames in flight
    _device_info_lines.push_back("Depth: " + std::string(format_name(g_depth_format)) +
        "  Frames in flight: " + std::to_string(g_frames_in_flight));

    // Mipmaps (levels are computed per-texture from its actual w/h; only the cap is fixed)
    _device_info_lines.push_back(!vk_texture_format_supports_mipmaps() ? "Mipmaps: unsupported"
        : (VK_TEXTURE_MIP_LEVELS_CAP == 1) ? "Mipmaps: disabled (VK_TEXTURE_MIP_LEVELS_CAP=1)"
        : (VK_TEXTURE_MIP_LEVELS_CAP > 0) ? "Mipmaps: supported, capped at " + std::to_string(VK_TEXTURE_MIP_LEVELS_CAP) + " levels"
        : "Mipmaps: supported, uncapped (levels computed per texture)");
}
