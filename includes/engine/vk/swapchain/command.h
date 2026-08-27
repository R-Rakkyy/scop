#ifndef SCOP_VK_COMMAND_H
#define SCOP_VK_COMMAND_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

int             vk_command_init(int graphics_family);
void            vk_command_cleanup(void);

VkCommandBuffer vk_command_allocate_one_time(void);
void            vk_command_free_one_time(VkCommandBuffer cb);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_COMMAND_H */
