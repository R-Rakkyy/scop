#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "engine/vk/swapchain/swapchain.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"
#include "glfw/window.h"

static VkExtent2D choose_swap_extent(uint32_t width, uint32_t height, const VkSurfaceCapabilitiesKHR *caps)
{
    if (caps->currentExtent.width != (uint32_t)-1) {
        return caps->currentExtent;
    } else {
        int fbw     = (int)width;
        int fbh     = (int)height;
        window_get_framebuffer_size(&fbw, &fbh);

        uint32_t use_w      = (fbw > 0) ? (uint32_t)fbw : (uint32_t)width;
        uint32_t use_h      = (fbh > 0) ? (uint32_t)fbh : (uint32_t)height;
        VkExtent2D actual   = { use_w, use_h };

        if (actual.width < caps->minImageExtent.width)          actual.width = caps->minImageExtent.width;

        else if (actual.width > caps->maxImageExtent.width)     actual.width = caps->maxImageExtent.width;
        
        if (actual.height < caps->minImageExtent.height)        actual.height = caps->minImageExtent.height;
        
        else if (actual.height > caps->maxImageExtent.height)   actual.height = caps->maxImageExtent.height;
        
        return actual;
    }
}

/* note: 
 * FIFO is always guaranteed available (Vulkan spec) and is inherently
 * tearing-free, so it's the fallback in both branches below. */
static VkPresentModeKHR choose_present_mode(const VkSurfaceCapabilitiesKHR *caps)
{
    (void)caps;
    uint32_t mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical, g_surface, &mode_count, NULL);
    if (mode_count == 0) return VK_PRESENT_MODE_FIFO_KHR;

    VkPresentModeKHR *modes = malloc(sizeof(VkPresentModeKHR) * mode_count);
    if (!modes) return VK_PRESENT_MODE_FIFO_KHR;
    vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical, g_surface, &mode_count, modes);

    VkPresentModeKHR chosen = VK_PRESENT_MODE_FIFO_KHR;
    if (VK_PRESENT_MODE_FORCE) { // USER OVERRIDE
        for (uint32_t i = 0; i < mode_count; ++i) {
            if (modes[i] == VK_PRESENT_MODE_CHOICE) {
                chosen = VK_PRESENT_MODE_CHOICE;
                break;
            }
        }
        if (chosen != VK_PRESENT_MODE_CHOICE)
            ftpp_logf(FTPP_LOG_WARNING, "choose_present_mode: user override %d not available, falling back to FIFO", (int)VK_PRESENT_MODE_CHOICE);
    } else if (!g_vsync_enabled) {
        /* If V-Sync is disabled, try to find IMMEDIATE first (lowest latency, tearing allowed), then MAILBOX (low latency, no tearing). */
        int found = 0;
        for (uint32_t i = 0; i < mode_count && !found; ++i) {
            if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                chosen = VK_PRESENT_MODE_IMMEDIATE_KHR;
                found = 1;
            }
        }
        if (!found) {
            for (uint32_t i = 0; i < mode_count; ++i) {
                if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                    chosen = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }
            }
        }
    }

    {   // log scope
        char list[128] = {0};
        for (uint32_t i = 0; i < mode_count && i < 8; ++i) {
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "%s%d", i ? "," : "", (int)modes[i]);
            strncat(list, tmp, sizeof(list) - strlen(list) - 1);
        }
        ftpp_logf(FTPP_LOG_DEBUG, "choose_present_mode: available=[%s] vsync=%d chosen=%d", list, g_vsync_enabled, (int)chosen);
    }
    free(modes);
    return chosen;
}

static VkSurfaceFormatKHR choose_surface_format(VkPhysicalDevice dev, VkSurfaceKHR surf, const VkSurfaceCapabilitiesKHR *caps)
{
    (void)caps;
    uint32_t fmtc = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &fmtc, NULL);
    if (fmtc == 0)  return (VkSurfaceFormatKHR){ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }; // no formats available, should never happen, return a safe default

    VkSurfaceFormatKHR *fmts = malloc(sizeof(VkSurfaceFormatKHR) * fmtc);
    if (!fmts)      return (VkSurfaceFormatKHR){ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &fmtc, fmts);

    VkSurfaceFormatKHR chosen = fmts[0]; // default to first available if the desired one isn't found
    for (uint32_t i = 0; i < fmtc; ++i) {
        if (fmts[i].format == VK_DESIRED_FORMAT && fmts[i].colorSpace == VK_DESIRED_COLOR_SPACE) {
            chosen = fmts[i];
            break;
        }
    }

    free(fmts);
    ftpp_logf(FTPP_LOG_DEBUG, "choose_surface_format: chosen format=%d colorSpace=%d", (int)chosen.format, (int)chosen.colorSpace);
    return chosen;
}

static int build_swapchain_create_info(int graphics_family, int present_family, const VkSurfaceCapabilitiesKHR *caps, VkSurfaceFormatKHR fmt, VkSwapchainCreateInfoKHR *out_sci, uint32_t *out_imgc)
{
    if (!caps || !out_sci || !out_imgc) return 0;
    VkExtent2D extent       = choose_swap_extent(WINDOW_DEFAULT_WIDTH, WINDOW_DEFAULT_HEIGHT, caps);
    uint32_t imgc           = (VK_MIN_IMAGE_COUNT > 0) ? (uint32_t)VK_MIN_IMAGE_COUNT : caps->minImageCount + 1;
    if (imgc < caps->minImageCount)                             imgc = caps->minImageCount;
    if (caps->maxImageCount > 0 && imgc > caps->maxImageCount)  imgc = caps->maxImageCount;
    
    memset(out_sci, 0, sizeof(*out_sci));
    out_sci->sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    out_sci->surface            = g_surface;
    out_sci->minImageCount      = imgc;
    out_sci->imageFormat        = fmt.format;
    out_sci->imageColorSpace    = fmt.colorSpace;
    out_sci->imageExtent        = extent;
    out_sci->imageArrayLayers   = 1;
    out_sci->imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_EXTRA;
    // If the graphics and present families are different, we need to use CONCURRENT mode and provide both family indices. Otherwise, EXCLUSIVE mode is preferred for performance.
    uint32_t indices[2] = {(uint32_t)graphics_family, (uint32_t)present_family};
    if (graphics_family != present_family) {
        out_sci->imageSharingMode       = VK_SHARING_MODE_CONCURRENT;
        out_sci->queueFamilyIndexCount  = 2;
        out_sci->pQueueFamilyIndices    = indices;
    } else {
        out_sci->imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    }
    out_sci->preTransform       = caps->currentTransform;               // use the current transform, no rotation or mirroring
    out_sci->compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;    // ignore alpha channel, no blending with other windows
    out_sci->presentMode        = choose_present_mode(caps);
    out_sci->clipped            = VK_TRUE;                              // allow the implementation to discard pixels that are obscured, for performance
                                                                        // note: never had issue with it, but if you do, set to VK_FALSE and see if it helps, idk
    *out_imgc = imgc;
    return 1;
}

int vk_create_swapchain(int graphics_family, int present_family)
{
    VkSurfaceCapabilitiesKHR    caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physical, g_surface, &caps);
    VkSurfaceFormatKHR          fmt = choose_surface_format(g_physical, g_surface, &caps);
    VkSwapchainCreateInfoKHR    sci = {0};
    uint32_t                    imgc = 0;
    if (!build_swapchain_create_info(graphics_family, present_family, &caps, fmt, &sci, &imgc)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_swapchain: failed to build swapchain create info");
        return 0;
    }

    // Create the swapchain
    VkResult _r = VK_SUCCESS;
    if (g_dldi.vkCreateSwapchainKHR)    _r = g_dldi.vkCreateSwapchainKHR(g_device, &sci, NULL, &g_swap);
    else                                _r = vkCreateSwapchainKHR(g_device, &sci, NULL, &g_swap);
    if (_r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_swapchain: vkCreateSwapchainKHR failed (VkResult=%d)", (int)_r);
        return 0;
    }
    g_swap_image_format     = fmt.format;
    g_swap_color_space      = fmt.colorSpace;
    g_swap_extent           = sci.imageExtent;
    g_swap_present_mode     = sci.presentMode;
    g_image_count           = imgc;

    /* Cap frames-in-flight to a sensible max (1..3), avoid too much memory usage and concurrency issues */
    if      (g_image_count < 1) g_frames_in_flight = 1;
    else if (g_image_count > 3) g_frames_in_flight = 3;
    else                        g_frames_in_flight = g_image_count;

    ftpp_logf(FTPP_LOG_DEBUG, "vk_create_swapchain: format=%d colorSpace=%d extent=%dx%d images=%u presentMode=%d", (int)g_swap_image_format, (int)g_swap_color_space, g_swap_extent.width, g_swap_extent.height, g_image_count, (int)g_swap_present_mode);
    ftpp_logf(FTPP_LOG_DEBUG, "vk_create_swapchain: frames_in_flight=%u (capped 1..3)", g_frames_in_flight);

    return 1;
}

void vk_destroy_swapchain(void)
{
    if (g_swap != VK_NULL_HANDLE) {
        if (g_dldi.vkDestroySwapchainKHR) g_dldi.vkDestroySwapchainKHR(g_device, g_swap, NULL);
        else vkDestroySwapchainKHR(g_device, g_swap, NULL);
    }
    g_swap = VK_NULL_HANDLE;
}

