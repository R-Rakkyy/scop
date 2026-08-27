#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <stdint.h>
#include "engine/vk/pipeline/shader.h"
#include "engine/vk/context.h"
#include "utils/file_management.h"
#include "iostream/logger_bridge.h"

int vk_create_shader_module_from_file(const char *path, VkShaderModule *out_module)
{
    if (!path || !out_module) return 0;
    void *shaderfile_buf    = NULL;
    size_t size             = 0;
    if (!fm_read_file(path, &shaderfile_buf, &size)) {
        ftpp_logf(FTPP_LOG_ERROR, "Failed to read shader file: %s", path);
        return 0;
    }

    if (size % 4 != 0 || !shaderfile_buf || size == 0) { // SPIR-V code must be a multiple of 4 bytes
        ftpp_logf(FTPP_LOG_ERROR, "Shader file corrupted or invalid size: %s", path);
        free(shaderfile_buf);
        return 0;
    }

    VkShaderModuleCreateInfo smci = {0};
    smci.sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize   = size;
    smci.pCode      = (const uint32_t*)shaderfile_buf;

    VkResult _r = vkCreateShaderModule(g_device, &smci, NULL, out_module);
    free(shaderfile_buf);
    if (_r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "vkCreateShaderModule failed for %s", path);
        return 0;
    }
    ftpp_logf(FTPP_LOG_DEBUG, "Loaded shader module from %s", path);
    return 1;
}

void vk_destroy_shader_module(VkShaderModule module)
{
    if (module != VK_NULL_HANDLE) vkDestroyShaderModule(g_device, module, NULL);
}
