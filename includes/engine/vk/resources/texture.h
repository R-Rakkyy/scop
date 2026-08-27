#ifndef SCOP_VK_TEXTURE_H
#define SCOP_VK_TEXTURE_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include "config.h" /* VK_TEXTURE_MAX */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    VkImage         image;
    VkDeviceMemory  mem;
    VkImageView     view;
} TextureSlot;

int         vk_texture_create(const uint8_t *rgba, uint32_t width, uint32_t height);
VkImageView vk_texture_get_view(int handle);
VkSampler   vk_texture_get_sampler(void);
VkSampler   vk_texture_get_ui_sampler(void);
void        vk_texture_destroy_all(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_TEXTURE_H */
