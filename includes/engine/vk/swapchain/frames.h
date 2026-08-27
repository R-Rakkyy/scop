#ifndef SCOP_VK_FRAMES_H
#define SCOP_VK_FRAMES_H

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

int     vk_frames_create_sync_objects(void);
void    vk_frames_destroy_sync_objects(void);

/* teardown sync objects used for a swapchain */
void    vk_frames_teardown_all(void);

#endif /* SCOP_VK_FRAMES_H */
