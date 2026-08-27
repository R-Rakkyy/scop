#ifndef SCOP_VK_DEBUG_MESSENGER_H
#define SCOP_VK_DEBUG_MESSENGER_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

int     vk_load_dispatcher(void);
int     vk_create_debug_messenger(void);
void    vk_destroy_debug_messenger(void);

#ifdef __cplusplus
}
#endif

#endif /* SCOP_VK_DEBUG_MESSENGER_H */
