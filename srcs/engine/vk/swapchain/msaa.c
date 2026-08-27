#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "engine/vk/swapchain/msaa.h"
#include "engine/vk/context.h"
#include "engine/vk/resources/buffer.h"
#include "iostream/logger_bridge.h"

int vk_msaa_create(VkExtent2D extent)
{
    if (g_msaa_samples <= VK_SAMPLE_COUNT_1_BIT) return 1; /* MSAA off: nothing to create */

    // MSAA needs an Image. (-> g_msaa_color_image)
    VkImageCreateInfo ici   = {0};
    ici.sType               = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType           = VK_IMAGE_TYPE_2D;
    ici.extent.width        = extent.width;
    ici.extent.height       = extent.height;
    ici.extent.depth        = 1;
    ici.mipLevels           = 1;
    ici.arrayLayers         = 1;
    ici.format              = g_swap_image_format;                  // !! MSAA color image must match the swapchain format
    ici.tiling              = VK_IMAGE_TILING_OPTIMAL;              // color images must be optimal-tiled for use as color attachments
    ici.initialLayout       = VK_IMAGE_LAYOUT_UNDEFINED;            // initial layout doesn't matter, will be transitioned to COLOR_ATTACHMENT_OPTIMAL before first render pass
    ici.usage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  // color images must be usable as color attachments
    ici.samples             = g_msaa_samples;                       // well, obviously.
    ici.sharingMode         = VK_SHARING_MODE_EXCLUSIVE;            // color images are not shared between queues
    if (vkCreateImage(g_device, &ici, NULL, &g_msaa_color_image) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_msaa_create: vkCreateImage failed");
        return 0;
    }

    // MSAA needs memory for the Image. First query the requirements, then allocate and bind. (-> g_msaa_color_mem)
    VkMemoryRequirements    mr;
    vkGetImageMemoryRequirements(g_device, g_msaa_color_image, &mr);
    uint32_t memType    =   vk_find_memory_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memType == UINT32_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_msaa_create: no suitable memory type");
        vk_msaa_destroy();
        return 0;
    }
    VkMemoryAllocateInfo mai    = {0};
    mai.sType                   = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize          = mr.size;
    mai.memoryTypeIndex         = memType;
    if (vkAllocateMemory(g_device, &mai, NULL, &g_msaa_color_mem) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_msaa_create: vkAllocateMemory failed");
        vk_msaa_destroy();
        return 0;
    }
    vkBindImageMemory(g_device, g_msaa_color_image, g_msaa_color_mem, 0);

    // MSAA needs an ImageView for the Image. (-> g_msaa_color_view)
    VkImageViewCreateInfo ivci          = {0};
    ivci.sType                          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image                          = g_msaa_color_image;
    ivci.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format                         = g_swap_image_format;
    ivci.subresourceRange.aspectMask    = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.levelCount    = 1;
    ivci.subresourceRange.layerCount    = 1;
    if (vkCreateImageView(g_device, &ivci, NULL, &g_msaa_color_view) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_msaa_create: vkCreateImageView failed");
        vk_msaa_destroy();
        return 0;
    }

    ftpp_logf(FTPP_LOG_DEBUG, "vk_msaa_create: created %ux%u color image (samples=%d)", extent.width, extent.height, (int)g_msaa_samples);
    return 1;
}

void vk_msaa_destroy(void)
{
    // color view
    if (g_msaa_color_view != VK_NULL_HANDLE) {
        vkDestroyImageView(g_device, g_msaa_color_view, NULL);
        g_msaa_color_view = VK_NULL_HANDLE;
    }
    // color image
    if (g_msaa_color_image != VK_NULL_HANDLE) {
        vkDestroyImage(g_device, g_msaa_color_image, NULL);
        g_msaa_color_image = VK_NULL_HANDLE;
    }
    // color memory
    if (g_msaa_color_mem != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, g_msaa_color_mem, NULL);
        g_msaa_color_mem = VK_NULL_HANDLE;
    }
}
