#define GLFW_INCLUDE_VULKAN
#include <stdlib.h>
#include <GLFW/glfw3.h>
#include "engine/vk/device/queues.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

int vk_find_queue_families(VkPhysicalDevice physical, VkSurfaceKHR surface, int *graphics_family, int *present_family)
{
    if (!physical) return 0;
    
    uint32_t qcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &qcount, NULL);
    if (qcount == 0) return 0;
    
    VkQueueFamilyProperties *qprops = malloc(sizeof(VkQueueFamilyProperties) * qcount);
    if (!qprops) return 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &qcount, qprops);
    
    int gf = -1, pf = -1; // gf = graphics family, pf = present family
    for (uint32_t i = 0; i < qcount; ++i) {
        if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)   gf = (int)i; // graphics family found
        
        if (surface != VK_NULL_HANDLE) {
            VkBool32 present_flag = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical, i, surface, &present_flag);
            if (present_flag)                               pf = (int)i; // present family found
        }
    }
    free(qprops);
    if (graphics_family)    *graphics_family = gf;
    if (present_family)     *present_family = pf;

    ftpp_logf(FTPP_LOG_DEBUG, "vk_find_queue_families: found graphics_family=%d present_family=%d for physical device 0x%p", gf, pf, (void*)physical);
    return (gf >= 0 && pf >= 0);
}

void vk_acquire_queues(int graphics_family, int present_family)
{
    if (g_device == VK_NULL_HANDLE) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_acquire_queues: g_device is not created");
        return;
    }
    
    if (graphics_family >= 0)   vkGetDeviceQueue(g_device, (uint32_t)graphics_family, 0, &g_graphics);
    if (present_family >= 0)    vkGetDeviceQueue(g_device, (uint32_t)present_family, 0, &g_present);

    ftpp_logf(FTPP_LOG_DEBUG, "vk_acquire_queues: acquired g_graphics=0x%p g_present=0x%p", (void*)g_graphics, (void*)g_present);
}
