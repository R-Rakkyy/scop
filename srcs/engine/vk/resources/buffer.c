#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>
#include "engine/vk/resources/buffer.h"
#include "engine/vk/context.h"
#include "engine/vk/swapchain/command.h"
#include "iostream/logger_bridge.h"

uint32_t vk_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps;

    vkGetPhysicalDeviceMemoryProperties(g_physical, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) // check if this memory type is suitable
            return i;
    }

    return UINT32_MAX; // no suitable memory type found
}

int vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *out_buf, VkDeviceMemory *out_mem)
{
    if (!out_buf || !out_mem) return 0;
    if (size == 0) { ftpp_logf(FTPP_LOG_ERROR, "vk_create_buffer: asked to create buffer with size 0"); return 0; }
    // query info
    VkBufferCreateInfo bci  = {0};
    bci.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size                = size;
    bci.usage               = usage;
    bci.sharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(g_device, &bci, NULL, out_buf) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_buffer: vkCreateBuffer failed");
        return 0;
    }
    // get memory requirements & allocate memory
    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(g_device, *out_buf, &mr);
    ftpp_logf(FTPP_LOG_DEBUG, "vk_create_buffer: mem req size=%llu alignment=%llu typeBits=0x%x", (unsigned long long)mr.size, (unsigned long long)mr.alignment, mr.memoryTypeBits);
    uint32_t memType    = vk_find_memory_type(mr.memoryTypeBits, properties);
    if (memType == UINT32_MAX) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_buffer: no suitable memory type found (typeBits=0x%x)", mr.memoryTypeBits);
        vkDestroyBuffer(g_device, *out_buf, NULL);
        return 0;
    }
    VkMemoryAllocateInfo mai    = {0};
    mai.sType                   = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize          = mr.size;
    mai.memoryTypeIndex         = memType;
    if (vkAllocateMemory(g_device, &mai, NULL, out_mem) != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_buffer: vkAllocateMemory failed (size=%llu)", (unsigned long long)mai.allocationSize);
        vkDestroyBuffer(g_device, *out_buf, NULL);
        return 0;
    }

    // bind buffer to memory
    VkResult r = vkBindBufferMemory(g_device, *out_buf, *out_mem, 0);
    if (r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_buffer: vkBindBufferMemory failed: %d", r);
        vkDestroyBuffer(g_device, *out_buf, NULL);
        vkFreeMemory(g_device, *out_mem, NULL);
        return 0;
    }
    ftpp_logf(FTPP_LOG_DEBUG, "vk_create_buffer: created buffer %p mem %p (size=%llu)", (void*)*out_buf, (void*)*out_mem, (unsigned long long)size);
    
    return 1;
}

int vk_copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    if (size == 0) { ftpp_logf(FTPP_LOG_ERROR, "vk_copy_buffer: asked to copy size 0"); return 0; }
    
    VkCommandBuffer cb = vk_command_allocate_one_time();    // allocate a command buffer for one-time use
    if (cb == VK_NULL_HANDLE) return 0;
    
    VkCommandBufferBeginInfo bi     = {0};
    bi.sType                        = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags                        = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cb, &bi) != VK_SUCCESS) { vk_command_free_one_time(cb); return 0; }
    
    VkBufferCopy copy   = {0};
    copy.srcOffset      = 0;
    copy.dstOffset      = 0;
    copy.size           = size;
    vkCmdCopyBuffer(cb, src, dst, 1, &copy);
    if (vkEndCommandBuffer(cb) != VK_SUCCESS) { vk_command_free_one_time(cb); return 0; } // end recording the command buffer
    
    // if it worked, submit the command buffer to the graphics queue and wait for it to finish
    VkSubmitInfo si         = {0};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &cb;

    VkResult _r = vkQueueSubmit(g_graphics, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(g_graphics);
    vk_command_free_one_time(cb);
    if (_r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_copy_buffer: vkQueueSubmit failed: %d", _r);
        return 0;
    }
    ftpp_logf(FTPP_LOG_DEBUG, "vk_copy_buffer: copied %llu bytes from %p to %p", (unsigned long long)size, (void*)src, (void*)dst);
    return 1;
}

void vk_destroy_buffer(VkBuffer buf, VkDeviceMemory mem)
{
    if (buf != VK_NULL_HANDLE) {
        vkDestroyBuffer(g_device, buf, NULL);
        ftpp_logf(FTPP_LOG_DEBUG, "vk_destroy_buffer: destroyed buffer %p", (void*)buf);
    }

    if (mem != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, mem, NULL);
        ftpp_logf(FTPP_LOG_DEBUG, "vk_destroy_buffer: freed memory %p", (void*)mem);
    }
}
