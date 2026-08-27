#ifndef SCOP_VK_TEXTURE_MIPMAP_H
#define SCOP_VK_TEXTURE_MIPMAP_H

#include <vulkan/vulkan.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int vk_texture_format_supports_mipmaps(void);

/* Blits image level 0 down into levels 1..mip_levels-1 and leaves every
 * level in SHADER_READ_ONLY_OPTIMAL. Caller must have already copied pixel
 * data into level 0 and left it in TRANSFER_SRC_OPTIMAL, with every other
 * level in TRANSFER_DST_OPTIMAL (see vk_texture_create), and must only call
 * this after vk_texture_format_supports_mipmaps() returned true. */
int vk_texture_generate_mipmaps(VkImage image, uint32_t w, uint32_t h, uint32_t mip_levels);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_TEXTURE_MIPMAP_H */
