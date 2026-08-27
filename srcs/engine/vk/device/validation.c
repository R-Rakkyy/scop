#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>
#include "engine/vk/device/validation.h"
#include "iostream/logger_bridge.h"

const char* s_validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

const char** vk_validation_get_layers(uint32_t *count)
{
    if (!count) return NULL;

    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    if (layer_count == 0) { *count = 0; return NULL; }
    
    VkLayerProperties *props = malloc(sizeof(VkLayerProperties) * layer_count);
    if (!props) { *count = 0; return NULL; }
    vkEnumerateInstanceLayerProperties(&layer_count, props);
    int found = 0;
    for (uint32_t i = 0; i < layer_count; ++i) { // try to find the validation layer in the list of available layers
        if (strcmp(props[i].layerName, s_validation_layers[0]) == 0) { found = 1; break; }
    }

    free(props);
    
    if (found) {
        *count = 1;
        return s_validation_layers;
    }
    *count = 0;
    return NULL;
}
