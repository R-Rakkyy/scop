#ifndef SCOP_VK_SWAPCHAIN_H
#define SCOP_VK_SWAPCHAIN_H

#ifdef __cplusplus
extern "C" {
#endif

int     vk_create_swapchain(int graphics_family, int present_family);
void    vk_destroy_swapchain(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_SWAPCHAIN_H */
