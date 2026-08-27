#ifndef SCOP_VK_QUEUES_H
#define SCOP_VK_QUEUES_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

// Finds queue family indices for the given physical device and surface.
// Returns 1 on success (both graphics and present found), 0 otherwise.
int vk_find_queue_families(VkPhysicalDevice physical, VkSurfaceKHR surface, int *graphics_family, int *present_family);

// After a logical device (`g_device`) has been created, acquire the device queues
// and store them into the global context (g_graphics, g_present).
void vk_acquire_queues(int graphics_family, int present_family);

#ifdef __cplusplus
}
#endif

#endif // SCOP_VK_QUEUES_H
