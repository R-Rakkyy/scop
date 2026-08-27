#ifndef SCOP_VK_DEPTH_H
#define SCOP_VK_DEPTH_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

int  vk_depth_create(VkExtent2D extent);
void vk_depth_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_DEPTH_H */
