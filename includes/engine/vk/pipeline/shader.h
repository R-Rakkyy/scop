#ifndef SCOP_VK_SHADER_H
#define SCOP_VK_SHADER_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

// Load a SPIR-V shader from disk and create a VkShaderModule.
int     vk_create_shader_module_from_file(const char *path, VkShaderModule *out_module);
void    vk_destroy_shader_module(VkShaderModule module);

#ifdef __cplusplus
}
#endif

#endif // SCOP_VK_SHADER_H
