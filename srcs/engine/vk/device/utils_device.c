#define GLFW_INCLUDE_VULKAN
#include <stdlib.h>
#include <string.h>
#include "engine/vk/device/utils_device.h"
#include "iostream/logger_bridge.h"

int vk_enumerate_device_extensions(VkPhysicalDevice dev, VkExtensionProperties **out_exts, uint32_t *out_count)
{
    if (!out_exts || !out_count) return 0;
    uint32_t count = 0;

    if (vkEnumerateDeviceExtensionProperties(dev, NULL, &count, NULL) != VK_SUCCESS) return 0;
    *out_count = count;
    if (count == 0) { *out_exts = NULL; return 1; }
    
    VkExtensionProperties *exts = malloc(sizeof(VkExtensionProperties) * count);
    if (!exts) return 0;
    
    if (vkEnumerateDeviceExtensionProperties(dev, NULL, &count, exts) != VK_SUCCESS) { free(exts); return 0; }
    
    *out_exts = exts;
    return 1;
}

int vk_has_device_extension(VkPhysicalDevice dev, const char *ext_name)
{
    if (!ext_name) return 0;
    VkExtensionProperties *exts = NULL;
    uint32_t count              = 0;
    
    if (!vk_enumerate_device_extensions(dev, &exts, &count)) return 0;
    
    int found = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (strcmp(exts[i].extensionName, ext_name) == 0) { found = 1; break; }
    }
    
    if (exts) free(exts);
    
    return found;
}

int vk_check_device_extensions(VkPhysicalDevice dev, const char **required, uint32_t required_count)
{
    if (!required || required_count == 0) return 1;
    VkExtensionProperties *exts = NULL;
    uint32_t count              = 0;
    
    if (!vk_enumerate_device_extensions(dev, &exts, &count)) return 0;
    for (uint32_t r = 0; r < required_count; ++r) {
        int found = 0;
        for (uint32_t e = 0; e < count; ++e) {
            if (strcmp(required[r], exts[e].extensionName) == 0) { found = 1; break; }
        }

        if (!found) {
            if (exts) free(exts);
            ftpp_logf(FTPP_LOG_DEBUG, "vk_check_device_extensions: missing device extension '%s'", required[r]);
            return 0;
        }
    }

    if (exts) free(exts);
    return 1;
}
