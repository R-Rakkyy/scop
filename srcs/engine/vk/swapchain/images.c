#define GLFW_INCLUDE_VULKAN
#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>
#include "engine/vk/swapchain/images.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

int vk_images_create_from_swapchain(void)
{
    if (g_device == VK_NULL_HANDLE || g_swap == VK_NULL_HANDLE)         { ftpp_logf(FTPP_LOG_ERROR, "vk_images_create_from_swapchain: device or swapchain not ready"); return 0; }

    // Query swapchain images
    VkResult _r = VK_SUCCESS;
    if (g_dldi.vkGetSwapchainImagesKHR)     _r = g_dldi.vkGetSwapchainImagesKHR(g_device, g_swap, &g_image_count, NULL);
    else                                    _r = vkGetSwapchainImagesKHR(g_device, g_swap, &g_image_count, NULL);
    if (_r != VK_SUCCESS) { ftpp_logf(FTPP_LOG_ERROR, "vk_images_create_from_swapchain: vkGetSwapchainImagesKHR failed (VkResult=%d)", (int)_r); return 0; }
    
    if (g_image_count == 0) return 0; // no images in swapchain, nothing to do
    ftpp_logf(FTPP_LOG_DEBUG, "vk_images_create_from_swapchain: image count=%u", g_image_count);

    // Allocate array for swapchain images and fetch them
    VkImage *imgs = malloc(sizeof(VkImage) * g_image_count);
    if (!imgs) { ftpp_logf(FTPP_LOG_ERROR, "vk_images_create_from_swapchain: allocation failure for images"); return 0; }
    if (g_dldi.vkGetSwapchainImagesKHR)     _r = g_dldi.vkGetSwapchainImagesKHR(g_device, g_swap, &g_image_count, imgs);
    else                                    _r = vkGetSwapchainImagesKHR(g_device, g_swap, &g_image_count, imgs);
    if (_r != VK_SUCCESS) { free(imgs); ftpp_logf(FTPP_LOG_ERROR, "vk_images_create_from_swapchain: vkGetSwapchainImagesKHR(fetch) failed (VkResult=%d)", (int)_r); return 0; }
    g_images = imgs;

    // Create image views using stored swapchain image format
    g_image_views = malloc(sizeof(VkImageView) * g_image_count);
    if (!g_image_views) { free(imgs); ftpp_logf(FTPP_LOG_ERROR, "vk_images_create_from_swapchain: allocation failure for image_views"); return 0; }
    for (uint32_t i = 0; i < g_image_count; ++i) {
        VkImageViewCreateInfo iv    = {0};
        iv.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image                    = imgs[i];
        iv.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
        iv.format                   = g_swap_image_format;
        iv.components.r         = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.components.g         = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.components.b         = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.components.a         = VK_COMPONENT_SWIZZLE_IDENTITY;
        iv.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.baseMipLevel    = 0;
        iv.subresourceRange.levelCount      = 1;
        iv.subresourceRange.baseArrayLayer  = 0;
        iv.subresourceRange.layerCount      = 1;
        VkResult _r2 = vkCreateImageView(g_device, &iv, NULL, &g_image_views[i]);
        if (_r2 != VK_SUCCESS) {
            ftpp_logf(FTPP_LOG_ERROR, "vk_images_create_from_swapchain: vkCreateImageView failed for index %u (VkResult=%d)", i, (int)_r2);
            for (uint32_t j = 0; j < i; ++j) vkDestroyImageView(g_device, g_image_views[j], NULL);
            free(g_image_views); g_image_views = NULL;
            free(imgs);
            return 0;
        }
        ftpp_logf(FTPP_LOG_DEBUG, "vk_images_create_from_swapchain: created image view[%u]", i);
        
        if (g_dldi.vkSetDebugUtilsObjectNameEXT) { // set debug name for image view
            VkDebugUtilsObjectNameInfoEXT on        = {0};
            on.sType                                = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            on.objectType                           = VK_OBJECT_TYPE_IMAGE_VIEW;
            on.objectHandle                         = (uint64_t)g_image_views[i];
            
            char _nm[64]; snprintf(_nm, sizeof(_nm), "swapchain-imageview-%u", i);
            on.pObjectName = _nm;
            
            g_dldi.vkSetDebugUtilsObjectNameEXT(g_device, &on);
        }
    }

    return 1;
}

void vk_images_destroy(void)
{
    if (g_image_views) {
        for (uint32_t i = 0; i < g_image_count; ++i) vkDestroyImageView(g_device, g_image_views[i], NULL);
        free(g_image_views); g_image_views = NULL;
    }
    if (g_images) { free(g_images); g_images = NULL; }
}
