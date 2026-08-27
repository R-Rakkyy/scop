#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "config.h"
#include "engine/vk/swapchain/depth.h"
#include "engine/vk/context.h"
#include "engine/vk/resources/buffer.h"
#include "iostream/logger_bridge.h"

int vk_depth_create(VkExtent2D extent)
{
    g_depth_format          = VK_DEPTH_FORMAT_DEFAULT;

    // Depth needs an Image. (-> g_depth_image)
    VkImageCreateInfo ici   = {0};
    ici.sType               = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType           = VK_IMAGE_TYPE_2D;
    ici.extent.width        = extent.width;
    ici.extent.height       = extent.height;
    ici.extent.depth        = 1;
    ici.mipLevels           = 1;
    ici.arrayLayers         = 1;
    ici.format              = g_depth_format;                               // depth image must match the chosen depth format
    ici.tiling              = VK_IMAGE_TILING_OPTIMAL;                      // depth images must be optimal-tiled for use as depth/stencil attachments
    ici.initialLayout       = VK_IMAGE_LAYOUT_UNDEFINED;                    // initial layout doesn't matter, will be transitioned to DEPTH_STENCIL_ATTACHMENT_OPTIMAL before first render pass
    ici.usage               = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;  // depth images must be usable as depth/stencil attachments
    ici.samples             = g_msaa_samples;                               // depth image must match the MSAA sample count of the color image
    ici.sharingMode         = VK_SHARING_MODE_EXCLUSIVE;                    // depth images are not shared between queues
    if (vkCreateImage(g_device, &ici, NULL, &g_depth_image) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_depth_create: vkCreateImage failed");
        return 0;
    }

    // Depth needs memory for the Image. First query the requirements, then allocate and bind. (-> g_depth_image_mem)
    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(g_device, g_depth_image, &mr);
    uint32_t memType    = vk_find_memory_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memType == UINT32_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_depth_create: no suitable memory type for depth image");
        vk_depth_destroy();
        return 0;
    }
    VkMemoryAllocateInfo mai    = {0};
    mai.sType                   = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize          = mr.size;
    mai.memoryTypeIndex         = memType;
    if (vkAllocateMemory(g_device, &mai, NULL, &g_depth_image_mem) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_depth_create: vkAllocateMemory failed");
        vk_depth_destroy();
        return 0;
    }
    vkBindImageMemory(g_device, g_depth_image, g_depth_image_mem, 0);

    // Depth needs an ImageView for the Image. (-> g_depth_image_view)
    VkImageViewCreateInfo ivci          = {0};
    ivci.sType                          = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image                          = g_depth_image;
    ivci.viewType                       = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format                         = g_depth_format;
    ivci.subresourceRange.aspectMask    = VK_IMAGE_ASPECT_DEPTH_BIT;
    ivci.subresourceRange.baseMipLevel  = 0;
    ivci.subresourceRange.levelCount    = 1;
    ivci.subresourceRange.baseArrayLayer= 0;
    ivci.subresourceRange.layerCount    = 1;
    if (vkCreateImageView(g_device, &ivci, NULL, &g_depth_image_view) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_depth_create: vkCreateImageView failed");
        vk_depth_destroy();
        return 0;
    }

    ftpp_logf(FTPP_LOG_DEBUG, "vk_depth_create: created %ux%u depth image (format=%d)", extent.width, extent.height, (int)g_depth_format);
    return 1;
}

void vk_depth_destroy(void)
{
    if (g_depth_image_view != VK_NULL_HANDLE) {
        vkDestroyImageView(g_device, g_depth_image_view, NULL);
        g_depth_image_view = VK_NULL_HANDLE;
    }
    if (g_depth_image != VK_NULL_HANDLE) {
        vkDestroyImage(g_device, g_depth_image, NULL);
        g_depth_image = VK_NULL_HANDLE;
    }
    if (g_depth_image_mem != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, g_depth_image_mem, NULL);
        g_depth_image_mem = VK_NULL_HANDLE;
    }
}
