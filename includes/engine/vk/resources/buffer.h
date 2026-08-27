#ifndef SCOP_VK_BUFFER_H
#define SCOP_VK_BUFFER_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t        vk_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
int             vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *out_buf, VkDeviceMemory *out_mem);
int             vk_copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
void            vk_destroy_buffer(VkBuffer buf, VkDeviceMemory mem);

#ifdef __cplusplus
}
#endif

#endif
