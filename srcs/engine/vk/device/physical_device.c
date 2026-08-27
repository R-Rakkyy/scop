#define GLFW_INCLUDE_VULKAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/vk/device/physical_device.h"
#include "engine/vk/device/utils_device.h"
#include "engine/vk/context.h"
#include "engine/vk/device/queues.h"
#include "iostream/logger_bridge.h"

static const char *device_type_name(VkPhysicalDeviceType t)
{
    switch (t) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:      return "discrete";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:    return "integrated";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:       return "virtual";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:               return "cpu";
        default:                                        return "other";
    }
}

/* care, only safe because at startup right now (main and only thread. might want to change it later) */
static uint32_t prompt_for_gpu_choice(uint32_t n)
{
    char line[32];
    ftpp_logf(FTPP_LOG_ERROR, "/!\\ Pressing Ctrl+C/Ctrl+D will abort the program and LEAK. /!\\");
    ftpp_logf(FTPP_LOG_ERROR, "Multiple GPUs found, pick one [0-%u] (default 0): ", n - 1);
    fflush(stdout);
    
    if (!fgets(line, sizeof(line), stdin)) return 0;

    char *endptr    = NULL;
    long choice     = strtol(line, &endptr, 10);
    
    if (endptr == line || choice < 0 || (uint32_t)choice >= n) {
        ftpp_logf(FTPP_LOG_WARNING, "prompt_for_gpu_choice: no/invalid selection, defaulting to device 0");
        return 0;
    }

    return (uint32_t)choice;
}

int vk_pick_physical_and_queues(int *graphics_family, int *present_family)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(g_instance, &count, NULL);
    if (count == 0) return 0;
    
    VkPhysicalDevice    *phys_devices   = malloc(sizeof(VkPhysicalDevice) * count);
    if (!phys_devices) return 0;
    t_gpu_candidate     *candidates     = malloc(sizeof(t_gpu_candidate) * count);
    if (!candidates) { free(phys_devices); return 0; }
    vkEnumeratePhysicalDevices(g_instance, &count, phys_devices);

    uint32_t candidate_count = 0;
    for (uint32_t d = 0; d < count; ++d) {
        VkPhysicalDevice            device = phys_devices[d];
        VkPhysicalDeviceProperties  props;
        vkGetPhysicalDeviceProperties(device, &props);
        ftpp_logf(FTPP_LOG_DEBUG, ">Found physical device %u: %s (API 0x%x):", d, props.deviceName, props.apiVersion);

        int gf = -1, pf = -1; // gf = graphics family, pf = present family
        int has_swapchain = vk_has_device_extension(device, "VK_KHR_swapchain");
        vk_find_queue_families(device, g_surface, &gf, &pf);
        ftpp_logf(FTPP_LOG_DEBUG, "   - supports VK_KHR_swapchain: %s", has_swapchain ? "yes" : "no");
        ftpp_logf(FTPP_LOG_DEBUG, "   - queue families: graphics=%d present=%d", gf, pf);
        if (!has_swapchain || gf < 0 || pf < 0) continue;

        candidates[candidate_count].index           = d;
        candidates[candidate_count].graphics_family = gf;
        candidates[candidate_count].present_family  = pf;
        candidate_count++;
    }

    if (candidate_count == 0) {
        ftpp_logf(FTPP_LOG_ERROR, "No suitable physical device found with swapchain and required queue families");
        free(phys_devices);
        free(candidates);
        *graphics_family = -1;
        *present_family = -1;
        return 0;
    }

    if (candidate_count > 1) {
        ftpp_logf(FTPP_LOG_ERROR, "Multiple suitable physical devices found (%u), you must specify one", candidate_count);
        for (uint32_t i = 0; i < candidate_count; ++i) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(phys_devices[candidates[i].index], &props);
            ftpp_logf(FTPP_LOG_ERROR, "[%u] %s (%s)", i, props.deviceName, device_type_name(props.deviceType));
        }
    }

    uint32_t chosen_candidate   = (candidate_count > 1) ? prompt_for_gpu_choice(candidate_count) : 0;
    t_gpu_candidate chosen      = candidates[chosen_candidate];

    g_physical          = phys_devices[chosen.index];
    *graphics_family    = chosen.graphics_family;
    *present_family     = chosen.present_family;
    ftpp_logf(FTPP_LOG_DEBUG, "Selected physical device index %u", chosen.index);
    free(phys_devices);
    free(candidates);

    return (*graphics_family >= 0 && *present_family >= 0); // return 1 if both families are valid, 0 otherwise
}
