#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "engine/vk/resources/texture.h"
#include "engine/vk/resources/texture_mipmap.h"
#include "engine/vk/context.h"
#include "engine/vk/resources/buffer.h"
#include "engine/vk/swapchain/command.h"
#include "iostream/logger_bridge.h"

static TextureSlot  g_slots[VK_TEXTURE_MAX];
static int          g_slot_count    = 0;
static VkSampler    g_sampler       = VK_NULL_HANDLE;
static VkSampler    g_sampler_ui    = VK_NULL_HANDLE;

static int ensure_sampler(void)
{
    if (g_sampler != VK_NULL_HANDLE) return 1;
    VkSamplerCreateInfo sci         = {0};
    sci.sType                       = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter                   = VK_FILTER_LINEAR;
    sci.minFilter                   = VK_FILTER_LINEAR;
    sci.addressModeU                = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeV                = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.addressModeW                = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.borderColor                 = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sci.mipmapMode                  = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.maxLod                      = VK_LOD_CLAMP_NONE; /* Vulkan clamps to whatever levels a given image actually has. */
    if (g_max_sampler_anisotropy > 0.0f) {
        sci.anisotropyEnable        = VK_TRUE;
        sci.maxAnisotropy           = g_max_sampler_anisotropy;
    }
    if (vkCreateSampler(g_device, &sci, NULL, &g_sampler) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture: vkCreateSampler failed");
        return 0;
    }
    return 1;
}

static int ensure_sampler_ui(void)
{
    if (g_sampler_ui != VK_NULL_HANDLE) return 1;
    VkSamplerCreateInfo sci     = {0};
    sci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    /* Linear for glyph smoothing to not bleed into neighboring glyphs. */
    sci.magFilter               = VK_FILTER_LINEAR;
    sci.minFilter               = VK_FILTER_LINEAR;
    sci.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sci.maxLod                  = 1.0f;
    if (vkCreateSampler(g_device, &sci, NULL, &g_sampler_ui) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture: vkCreateSampler(nearest) failed");
        return 0;
    }
    return 1;
}

/* 
 * Transitions the image to TRANSFER_DST_OPTIMAL and copies the staging buffer into it. */
static int transition_and_copy(VkBuffer staging, VkImage image, uint32_t w, uint32_t h, uint32_t mip_levels)
{
    VkCommandBuffer cb = vk_command_allocate_one_time();
    if (cb == VK_NULL_HANDLE) return 0;
    VkCommandBufferBeginInfo bi = {0};
    bi.sType        = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags        = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS) { vk_command_free_one_time(cb); return 0; }

    /* Covers every level, even though only level 0 gets real pixel data below */
    VkImageMemoryBarrier to_dst = {0};
    to_dst.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_dst.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
    to_dst.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_dst.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    to_dst.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    to_dst.image                = image;
    to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_dst.subresourceRange.levelCount = mip_levels;
    to_dst.subresourceRange.layerCount = 1;
    to_dst.srcAccessMask        = 0;
    to_dst.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &to_dst);

    VkBufferImageCopy region            = {0};
    region.imageSubresource.aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount  = 1;
    region.imageExtent.width            = w;
    region.imageExtent.height           = h;
    region.imageExtent.depth            = 1;
    vkCmdCopyBufferToImage(cb, staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier to_next        = to_dst;
    to_next.subresourceRange.levelCount = 1; /* only level 0 was just written */
    to_next.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_next.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
    if (mip_levels > 1) {
        /* Leave level 0 as a blit source */
        to_next.newLayout               = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        to_next.dstAccessMask           = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &to_next);
    } else {
        to_next.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_next.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &to_next);
    }

    // end recording and submit
    if (vkEndCommandBuffer(cb) != VK_SUCCESS) { vk_command_free_one_time(cb); return 0; }
    VkSubmitInfo si     = {0};
    si.sType            = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers  = &cb;
    
    VkResult _r = vkQueueSubmit(g_graphics, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_graphics);
    vk_command_free_one_time(cb);
    
    return _r == VK_SUCCESS;
}

int vk_texture_create(const uint8_t *rgba, uint32_t w, uint32_t h)
{
    if (!rgba || w == 0 || h == 0) return -1;
    if (g_slot_count >= VK_TEXTURE_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: texture registry full (max %d)", VK_TEXTURE_MAX);
        return -1;
    }
    if (!ensure_sampler() || !ensure_sampler_ui()) return -1;

    uint32_t mip_levels = 1;
    if (vk_texture_format_supports_mipmaps()) {
        mip_levels = (uint32_t)floorf(log2f((float)((w > h) ? w : h))) + 1;         // < Generate mip levels down to 1x1
        if (VK_TEXTURE_MIP_LEVELS_CAP > 0 && mip_levels > VK_TEXTURE_MIP_LEVELS_CAP)
            mip_levels = VK_TEXTURE_MIP_LEVELS_CAP;
    }

    // slow and host-visible staging buffer for uploading pixel data to device-local memory
    VkDeviceSize buf_size           = (VkDeviceSize)w * (VkDeviceSize)h * 4;
    VkBuffer staging                = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem      = VK_NULL_HANDLE;
    if (!vk_create_buffer(buf_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging, &staging_mem)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: staging buffer creation failed");
        return -1;
    }

    void *mapped = NULL;
    if (vkMapMemory(g_device, staging_mem, 0, buf_size, 0, &mapped) != VK_SUCCESS || !mapped) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: vkMapMemory failed");
        vk_destroy_buffer(staging, staging_mem);
        return -1;
    }
    memcpy(mapped, rgba, (size_t)buf_size);     // copy the pixel data into the staging buffer
    vkUnmapMemory(g_device, staging_mem);       // unmap the staging buffer memory

    /* **** Creation of TextureSlot **** */
    TextureSlot slot        = {0};
    VkImageCreateInfo ici   = {0};
    ici.sType               = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType           = VK_IMAGE_TYPE_2D;
    ici.extent.width        = w;
    ici.extent.height       = h;
    ici.extent.depth        = 1;
    ici.mipLevels           = mip_levels;
    ici.arrayLayers         = 1;
    ici.format              = SCOP_TEXTURE_FORMAT;
    ici.tiling              = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout       = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage               = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.samples             = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(g_device, &ici, NULL, &slot.image) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: vkCreateImage failed");
        vk_destroy_buffer(staging, staging_mem);
        return -1;
    }

    // mem info + allocate
    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(g_device, slot.image, &mr);
    uint32_t memType = vk_find_memory_type(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memType == UINT32_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: no suitable memory type");
        vkDestroyImage(g_device, slot.image, NULL);
        vk_destroy_buffer(staging, staging_mem);
        return -1;
    }
    VkMemoryAllocateInfo mai    = {0};
    mai.sType                   = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize          = mr.size;
    mai.memoryTypeIndex         = memType;
    if (vkAllocateMemory(g_device, &mai, NULL, &slot.mem) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: vkAllocateMemory failed");
        vkDestroyImage(g_device, slot.image, NULL);
        vk_destroy_buffer(staging, staging_mem);
        return -1;
    }

    // bind and copy
    vkBindImageMemory(g_device, slot.image, slot.mem, 0);
    if (!transition_and_copy(staging, slot.image, w, h, mip_levels)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: upload failed");
        vk_destroy_buffer(staging, staging_mem);
        vkFreeMemory(g_device, slot.mem, NULL);
        vkDestroyImage(g_device, slot.image, NULL);
        return -1;
    }
    vk_destroy_buffer(staging, staging_mem);

    // create mipmaps if supported, otherwise leave level 0 in SHADER_READ_ONLY_OPTIMAL
    if (mip_levels > 1 && !vk_texture_generate_mipmaps(slot.image, w, h, mip_levels)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: mipmap generation failed");
        vkFreeMemory(g_device, slot.mem, NULL);
        vkDestroyImage(g_device, slot.image, NULL);
        return -1;
    }

    VkImageViewCreateInfo ivci      = {0};
    ivci.sType                      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image                      = slot.image;
    ivci.viewType                   = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format                     = SCOP_TEXTURE_FORMAT;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.levelCount = mip_levels;
    ivci.subresourceRange.layerCount = 1;
    if (vkCreateImageView(g_device, &ivci, NULL, &slot.view) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_texture_create: vkCreateImageView failed");
        vkFreeMemory(g_device, slot.mem, NULL);
        vkDestroyImage(g_device, slot.image, NULL);
        return -1;
    }


    int handle              = g_slot_count;
    g_slots[g_slot_count++] = slot;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_texture_create: created %ux%u texture (handle=%d)", w, h, handle);
    
    return handle;
}

VkImageView vk_texture_get_view(int handle)
{
    if (handle < 0 || handle >= g_slot_count) return VK_NULL_HANDLE;
    return g_slots[handle].view;
}

VkSampler vk_texture_get_sampler(void)      { return g_sampler;     }
VkSampler vk_texture_get_ui_sampler(void)   { return g_sampler_ui;  }

void vk_texture_destroy_all(void)
{
    for (int i = 0; i < g_slot_count; ++i) {
        if (g_slots[i].view != VK_NULL_HANDLE)  vkDestroyImageView(g_device, g_slots[i].view, NULL);
        if (g_slots[i].image != VK_NULL_HANDLE) vkDestroyImage(g_device, g_slots[i].image, NULL);
        if (g_slots[i].mem != VK_NULL_HANDLE)   vkFreeMemory(g_device, g_slots[i].mem, NULL);
    }
    g_slot_count = 0;
    if (g_sampler != VK_NULL_HANDLE)    { vkDestroySampler(g_device, g_sampler, NULL); g_sampler = VK_NULL_HANDLE;          }
    if (g_sampler_ui != VK_NULL_HANDLE) { vkDestroySampler(g_device, g_sampler_ui, NULL); g_sampler_ui = VK_NULL_HANDLE;    }
}
