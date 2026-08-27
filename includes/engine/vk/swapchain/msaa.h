#ifndef SCOP_VK_MSAA_H
#define SCOP_VK_MSAA_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Multisampled color image every pipeline renders into.
 * Must be called after vk_create_swapchain() (needs g_swap_image_format/g_swap_extent)
 * and recreated alongside the depth image on resize. */
int  vk_msaa_create(VkExtent2D extent);
void vk_msaa_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_MSAA_H */
