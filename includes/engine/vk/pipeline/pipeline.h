#ifndef SCOP_VK_PIPELINE_H
#define SCOP_VK_PIPELINE_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

int     vk_pipeline_create_from_shaders(VkShaderModule vert, VkShaderModule frag, int graphics_family);
int     vk_pipeline_create_palette(VkShaderModule vert, VkShaderModule frag);
void    vk_pipeline_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // SCOP_VK_PIPELINE_H
