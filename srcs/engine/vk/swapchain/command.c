#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "engine/vk/swapchain/command.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

int vk_command_init(int graphics_family)
{
    if (graphics_family < 0) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_command_init: invalid graphics family %d", graphics_family);
        return 0;
    }

    VkCommandPoolCreateInfo cpci        = {0};
    cpci.sType                          = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.flags                          = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex               = (uint32_t)graphics_family;
    if (vkCreateCommandPool(g_device, &cpci, NULL, &g_cmdpool) != VK_SUCCESS) return 0;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_command_init: created command pool for family %d", graphics_family);

    uint32_t frames         = (g_frames_in_flight > 0) ? g_frames_in_flight : 1;
    g_cmdbuf                = malloc(sizeof(VkCommandBuffer) * frames);
    if (!g_cmdbuf) { ftpp_logf(FTPP_LOG_ERROR, "vk_command_init: failed to allocate command buffer array"); return 0; }

    VkCommandBufferAllocateInfo cbai    = {0};
    cbai.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool                    = g_cmdpool;
    cbai.level                          = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount             = frames;
    if (vkAllocateCommandBuffers(g_device, &cbai, g_cmdbuf) != VK_SUCCESS) return 0;
    ftpp_logf(FTPP_LOG_DEBUG, "vk_command_init: allocated %u per-frame command buffers", frames);

    return 1;
}

void vk_command_cleanup(void)
{
    if (g_cmdbuf) {
        uint32_t frames = (g_frames_in_flight > 0) ? g_frames_in_flight : 1;
        vkFreeCommandBuffers(g_device, g_cmdpool, frames, g_cmdbuf);
        free(g_cmdbuf);
        g_cmdbuf = NULL;
    }
    if (g_cmdpool != VK_NULL_HANDLE) { vkDestroyCommandPool(g_device, g_cmdpool, NULL); g_cmdpool = VK_NULL_HANDLE; }

    ftpp_logf(FTPP_LOG_DEBUG, "vk_command_cleanup: command pool and buffers destroyed");
}

VkCommandBuffer vk_command_allocate_one_time(void)
{
    if (g_device == VK_NULL_HANDLE || g_cmdpool == VK_NULL_HANDLE)
        return VK_NULL_HANDLE;

    VkCommandBufferAllocateInfo cbai    = {0};
    cbai.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool                    = g_cmdpool;
    cbai.level                          = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount             = 1;

    VkCommandBuffer commandbuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(g_device, &cbai, &commandbuffer) != VK_SUCCESS) return VK_NULL_HANDLE;
    return commandbuffer;
}

void vk_command_free_one_time(VkCommandBuffer cb)
{
    if (g_device == VK_NULL_HANDLE || g_cmdpool == VK_NULL_HANDLE)  return;
    if (cb == VK_NULL_HANDLE)                                       return;

    vkFreeCommandBuffers(g_device, g_cmdpool, 1, &cb);
}
