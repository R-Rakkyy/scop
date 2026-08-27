#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "engine/vk/resources/texture_mipmap.h"
#include "engine/vk/context.h"
#include "engine/vk/swapchain/command.h"
#include "config.h"

int vk_texture_format_supports_mipmaps(void)
{
    if (SCOP_TEXTURE_FORMAT == VK_FORMAT_UNDEFINED) return 0;
    VkFormatProperties props;

    vkGetPhysicalDeviceFormatProperties(g_physical, SCOP_TEXTURE_FORMAT, &props);
    return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
}

int vk_texture_generate_mipmaps(VkImage image, uint32_t w, uint32_t h, uint32_t mip_levels)
{
    VkCommandBuffer cb = vk_command_allocate_one_time();
    if (cb == VK_NULL_HANDLE) return 0;

    VkCommandBufferBeginInfo bi     = {0};
    bi.sType                        = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags                        = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS) { vk_command_free_one_time(cb); return 0; }

    VkImageMemoryBarrier barrier    = {0}; // Memory barrier used to transition the image layout for each mip level
    barrier.sType                   = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex     = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex     = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                   = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mip_w = (int32_t)w;
    int32_t mip_h = (int32_t)h;

    for (uint32_t i = 1; i < mip_levels; ++i) { // i is the mip level being generated, i-1 is the source level
        barrier.subresourceRange.baseMipLevel   = i - 1;
        barrier.oldLayout                       = (i == 1) ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                   = (i == 1) ? VK_ACCESS_TRANSFER_READ_BIT : VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;
        if (i > 1) { // transition previous level to TRANSFER_SRC_OPTIMAL before blitting
            vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        }

        VkImageBlit blit        = {0};
        blit.srcOffsets[1].x    = mip_w;
        blit.srcOffsets[1].y    = mip_h;
        blit.srcOffsets[1].z    = 1;
        blit.srcSubresource.aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel    = i - 1;
        blit.srcSubresource.layerCount  = 1;
        blit.dstOffsets[1].x    = (mip_w > 1) ? mip_w / 2 : 1;
        blit.dstOffsets[1].y    = (mip_h > 1) ? mip_h / 2 : 1;
        blit.dstOffsets[1].z    = 1;
        blit.dstSubresource.aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel    = i;
        blit.dstSubresource.layerCount  = 1;
        vkCmdBlitImage(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout               = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;     // currently, the source level is in TRANSFER_SRC_OPTIMAL
        barrier.newLayout               = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // after blitting, we want it to be in SHADER_READ_ONLY_OPTIMAL for sampling
        barrier.srcAccessMask           = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask           = VK_ACCESS_SHADER_READ_BIT;
        // transition the source level to SHADER_READ_ONLY_OPTIMAL after blitting
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        
        // 16x16 -> 8x8 -> 4x4 -> 2x2 -> 1x1
        if (mip_w > 1) mip_w /= 2; 
        if (mip_h > 1) mip_h /= 2;
    }
    //  i (the miplevel) has reached max, compute the last barrier to transition the last level to SHADER_READ_ONLY_OPTIMAL
    barrier.subresourceRange.baseMipLevel   = mip_levels - 1;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

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
