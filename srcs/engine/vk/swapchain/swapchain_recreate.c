#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "config.h"
#include "engine/vk/swapchain/swapchain_recreate.h"
#include "engine/vk/swapchain/swapchain.h"
#include "engine/vk/swapchain/images.h"
#include "engine/vk/pipeline/pipeline.h"
#include "engine/vk/swapchain/command.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"
#include "glfw/window.h"
#include "engine/vk/swapchain/frames.h"
#include "engine/vk/swapchain/depth.h"
#include "engine/vk/swapchain/msaa.h"
#include "engine/vk/pipeline/grid_pipeline.h"

// i separate vk_teardown_swapchain_core and vk_teardown_swapchain_extent_resources 
// because if we dont change the extent, we dont need to destroy and recreate depth/msaa/pipeline/grid, which is a lot of work for no reason

static void vk_teardown_swapchain_core(void)
{
    if (g_device == VK_NULL_HANDLE) return;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_teardown_swapchain_core: begin, waiting for device idle");
    vkDeviceWaitIdle(g_device);
    ftpp_logf(FTPP_LOG_DEBUG, "vk_teardown_swapchain_core: device idle, tearing down");

    vk_command_cleanup();
    vk_frames_teardown_all();
    vk_images_destroy();
    if (g_swap != VK_NULL_HANDLE) { vkDestroySwapchainKHR(g_device, g_swap, NULL); g_swap = VK_NULL_HANDLE; }
    ftpp_logf(FTPP_LOG_DEBUG, "vk_teardown_swapchain_core: complete");
}

static void vk_teardown_swapchain_extent_resources(void)
{
    vk_pipeline_destroy();
    vk_depth_destroy();
    vk_msaa_destroy();
}

int vk_recreate_swapchain(void)
{
    if (g_device == VK_NULL_HANDLE) return 0;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_recreate_swapchain: begin, waiting for device idle");
    vkDeviceWaitIdle(g_device); /* Wait until device is idle before tearing down */
    ftpp_logf(FTPP_LOG_DEBUG, "vk_recreate_swapchain: device idle");

    const VkExtent2D old_extent = g_swap_extent;
    /* Teardown the part every recreate needs */
    vk_teardown_swapchain_core();
    /* Recreate swapchain and images */
    if (!vk_create_swapchain(g_graphics_family, g_present_family)) return 0;

    int fbw = 0, fbh = 0;
    window_get_framebuffer_size(&fbw, &fbh);
    if (fbw > 0 && fbh > 0 && ((uint32_t)fbw != g_swap_extent.width || (uint32_t)fbh != g_swap_extent.height)) { // Framebuffer size changed since swapchain creation, need to recreate again
        ftpp_logf(FTPP_LOG_WARNING, "vk_recreate_swapchain: extent %ux%u stale vs framebuffer %dx%d, retrying", g_swap_extent.width, g_swap_extent.height, fbw, fbh);
        vk_destroy_swapchain();
        if (!vk_create_swapchain(g_graphics_family, g_present_family)) return 0;
    }

    if (!vk_images_create_from_swapchain()) return 0;
    if (!vk_command_init(g_graphics_family)) return 0;
    if (!vk_frames_create_sync_objects()) return 0;

    int extent_changed = (g_swap_extent.width != old_extent.width || g_swap_extent.height != old_extent.height);
    if (extent_changed || g_depth_image == VK_NULL_HANDLE) { // Depth and MSAA images are tied to the swapchain extent.
        vk_teardown_swapchain_extent_resources();

        if (!vk_depth_create(g_swap_extent))    return 0;
        if (!vk_msaa_create(g_swap_extent))     return 0;

        if (g_vert_shader != VK_NULL_HANDLE && g_frag_shader != VK_NULL_HANDLE) {
            if (!vk_pipeline_create_from_shaders(g_vert_shader, g_frag_shader, g_graphics_family))  return 0;
        }
        if (g_vert_shader != VK_NULL_HANDLE && g_frag_shader_palette != VK_NULL_HANDLE) {
            if (!vk_pipeline_create_palette(g_vert_shader, g_frag_shader_palette))                  return 0;
        }

        // Grid pipeline too
        vk_grid_destroy();
        if (!vk_grid_init()) return 0;
    }

    ftpp_logf(FTPP_LOG_DEBUG, "vk_recreate_swapchain: complete (%ux%u)", g_swap_extent.width, g_swap_extent.height);
    return 1;
}
