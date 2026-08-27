#ifndef SCOP_VK_UTILS_DEVICE_H
#define SCOP_VK_UTILS_DEVICE_H

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

/* Enumerate device extensions. Allocates *out_exts which must be freed by caller. */
int vk_enumerate_device_extensions(VkPhysicalDevice dev, VkExtensionProperties **out_exts, uint32_t *out_count);

/* Check if a device supports a specific extension name. */
int vk_has_device_extension(VkPhysicalDevice dev, const char *ext_name);

/* Check if a device supports all extensions in the list. */
int vk_check_device_extensions(VkPhysicalDevice dev, const char **required, uint32_t required_count);

#endif /* SCOP_VK_UTILS_DEVICE_H */
