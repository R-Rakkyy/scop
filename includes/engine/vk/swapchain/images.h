#ifndef SCOP_VK_IMAGES_H
#define SCOP_VK_IMAGES_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif
    
int     vk_images_create_from_swapchain(void);
void    vk_images_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // SCOP_VK_IMAGES_H
