#define GLFW_INCLUDE_VULKAN
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "engine/vk/device/logical_device.h"
#include "engine/vk/context.h"
#include "engine/vk/device/queues.h"
#include "engine/vk/device/utils_device.h"
#include "iostream/logger_bridge.h"

static const char *OPTIONAL_MEMORY_BUDGET_EXT       = "VK_EXT_memory_budget";
static const char *device_exts[]                    = { "VK_KHR_swapchain" };
static const char *combined_exts[sizeof(device_exts) / sizeof(device_exts[0]) + 1];

static VkSampleCountFlagBits choose_msaa_samples(void)
{
    if (!ENABLE_MSAA) return VK_SAMPLE_COUNT_1_BIT;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_physical, &props);
    VkSampleCountFlags counts = (props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts);

    VkSampleCountFlagBits candidates[] = {
        VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_8_BIT, VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        if (candidates[i] > VK_MSAA_MAX_SAMPLES)    continue;
        if (counts & candidates[i])                 return candidates[i];
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

static int detect_and_fill_supported_features(  VkPhysicalDeviceFeatures *out_features,
                                                VkPhysicalDeviceFeatures2 *out_feat2,
                                                VkPhysicalDeviceVulkan13Features *out_feat13)
{
    if (!out_features || !out_feat2 || !out_feat13) return 0;
    PFN_vkGetPhysicalDeviceFeatures2 fpGetFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(g_instance, "vkGetPhysicalDeviceFeatures2");
    if (!fpGetFeatures2) {
        ftpp_logf(FTPP_LOG_ERROR, "detect_and_fill_supported_features: vkGetPhysicalDeviceFeatures2 unavailable (need Vulkan >= 1.1)");
        return 0;
    }

    memset(out_feat13, 0, sizeof(*out_feat13));
    out_feat13->sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    memset(out_feat2, 0, sizeof(*out_feat2));
    out_feat2->sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    out_feat2->pNext    = out_feat13;
    fpGetFeatures2(g_physical, out_feat2);

    if (!out_feat13->dynamicRendering) {
        ftpp_logf(FTPP_LOG_ERROR, "detect_and_fill_supported_features: device does not support dynamicRendering (Vulkan 1.3)");
        return 0;
    }

    if (ENABLE_ANISOTROPIC_FILTERING && out_feat2->features.samplerAnisotropy) {
        out_features->samplerAnisotropy = VK_TRUE;
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(g_physical, &props);
        g_max_sampler_anisotropy = (props.limits.maxSamplerAnisotropy < 16.0f) // clamp to 16.
                                    ? props.limits.maxSamplerAnisotropy : 16.0f;
    }
    if (out_feat2->features.fillModeNonSolid)   out_features->fillModeNonSolid = VK_TRUE;   // wireframe, point, etc
    if (out_feat2->features.sampleRateShading)  out_features->sampleRateShading = VK_TRUE;  // per-sample shading (for MSAA)
    ftpp_logf(FTPP_LOG_DEBUG, "detect_and_fill_supported_features: samplerAnisotropy=%d sampleRateShading=%d fillModeNonSolid=%d dynamicRendering=%d", (int)out_features->samplerAnisotropy, (int)out_features->sampleRateShading, (int)out_features->fillModeNonSolid, (int)out_feat13->dynamicRendering);

    /* Re-request only what we detected as supported (features2 must be re-populated
     * with the requested subset, not the full supported set, before vkCreateDevice). */
    out_feat2->features             = *out_features;
    memset(out_feat13, 0, sizeof(*out_feat13));
    out_feat13->sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    out_feat13->dynamicRendering    = VK_TRUE;
    out_feat2->pNext                = out_feat13;
    return 1;
}

static int prepare_device_create_info(VkDeviceCreateInfo *di, VkDeviceQueueCreateInfo *qis, uint32_t unique_count,
    VkPhysicalDeviceFeatures *out_features, VkPhysicalDeviceFeatures2 *out_feat2, VkPhysicalDeviceVulkan13Features *out_feat13)
{
    if (!di || !out_features) return 0;
    memset(out_features, 0, sizeof(*out_features));
    if (!detect_and_fill_supported_features(out_features, out_feat2, out_feat13)) return 0;

    memset(di, 0, sizeof(*di));
    di->sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    di->queueCreateInfoCount    = unique_count;
    di->pQueueCreateInfos       = qis;

    uint32_t required_extc = (uint32_t)(sizeof(device_exts) / sizeof(device_exts[0])); // this is the required extension count
    if (!vk_check_device_extensions(g_physical, device_exts, required_extc)) {
        ftpp_logf(FTPP_LOG_ERROR, "prepare_device_create_info: required device extensions not supported");
        return 0;
    }

    g_memory_budget_supported = vk_has_device_extension(g_physical, OPTIONAL_MEMORY_BUDGET_EXT);
    if (g_memory_budget_supported) {
        for (uint32_t i = 0; i < required_extc; ++i)
            combined_exts[i]            = device_exts[i];
        combined_exts[required_extc]    = OPTIONAL_MEMORY_BUDGET_EXT;
        di->enabledExtensionCount       = required_extc + 1;
        di->ppEnabledExtensionNames     = combined_exts;
        ftpp_logf(FTPP_LOG_DEBUG, "prepare_device_create_info: VK_EXT_memory_budget available, enabling");
    } else {
        di->enabledExtensionCount       = required_extc;
        di->ppEnabledExtensionNames     = device_exts;
        ftpp_logf(FTPP_LOG_WARNING, "prepare_device_create_info: VK_EXT_memory_budget not available, GPU memory will report n/a");
    }
    di->pNext               = out_feat2;
    di->pEnabledFeatures    = NULL;
    return 1;
}

int vk_create_device_and_queues(int graphics_family, int present_family)
{
    if (graphics_family < 0) {
        ftpp_logf(FTPP_LOG_ERROR, "Invalid graphics queue family index: %d", graphics_family);
        return 0;
    }

    /* ******************************* Querying ******************************** */

    /* Build a unique list of queue families we need (graphics [+ present]). */
    uint32_t unique_fams[2];
    uint32_t unique_count = 0;
    unique_fams[unique_count++] = (uint32_t)graphics_family;
    if (present_family >= 0 && present_family != graphics_family)
        unique_fams[unique_count++] = (uint32_t)present_family;

    float qprio = 1.0f;         // single queue per family, so priority is always 1.0.
    VkDeviceQueueCreateInfo qis[2];
    memset(qis, 0, sizeof(qis));

    for (uint32_t i = 0; i < unique_count; ++i) {
        qis[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qis[i].queueFamilyIndex = unique_fams[i];
        qis[i].queueCount = 1;
        qis[i].pQueuePriorities = &qprio;
    }

    // onto some random vulkan bullshit again
    VkPhysicalDeviceFeatures            device_features;
    VkPhysicalDeviceFeatures2           feat2;          // yes because why wont i need a feature2 struct
    VkPhysicalDeviceVulkan13Features    feat13;         // ah yes let's add a feature13, who's gonna stop us anyway? OpenGL?
    VkDeviceCreateInfo                  di;
    if (!prepare_device_create_info(&di, qis, unique_count, &device_features, &feat2, &feat13)) {
        ftpp_logf(FTPP_LOG_ERROR, "vk_create_device_and_queues: failed to prepare device create info");
        return 0;
    }

    /* ******************************* Create Device ******************************** */

    VkResult r = vkCreateDevice(g_physical, &di, NULL, &g_device);
    if (r != VK_SUCCESS) {
        ftpp_logf(FTPP_LOG_ERROR, "Failed to create logical device (VkResult %d)", (int)r);
        return 0;
    }
    ftpp_logf(FTPP_LOG_DEBUG, "Logical device created");
    g_msaa_samples = choose_msaa_samples();
    ftpp_logf(FTPP_LOG_DEBUG, "vk_create_device_and_queues: MSAA samples=%d", (int)g_msaa_samples);
    
    /* store the chosen family indices in global context for later use (recreate) */
    g_graphics_family   = graphics_family;
    g_present_family    = present_family;
    vk_acquire_queues(graphics_family, present_family);
    /* Populate device-level dispatch entries for swapchain/WSI functions */
    if (g_device != VK_NULL_HANDLE) {
        g_dldi.vkCreateSwapchainKHR     = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(g_device,       "vkCreateSwapchainKHR");
        g_dldi.vkDestroySwapchainKHR    = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(g_device,      "vkDestroySwapchainKHR");
        g_dldi.vkGetSwapchainImagesKHR  = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(g_device,    "vkGetSwapchainImagesKHR");
        g_dldi.vkAcquireNextImageKHR    = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(g_device,      "vkAcquireNextImageKHR");
        g_dldi.vkQueuePresentKHR        = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(g_device,          "vkQueuePresentKHR");
        ftpp_logf(FTPP_LOG_DEBUG, "vk_create_device_and_queues: loaded device-level dispatch: createSwap=%d getImages=%d acquireNext=%d present=%d", g_dldi.vkCreateSwapchainKHR != NULL, g_dldi.vkGetSwapchainImagesKHR != NULL, g_dldi.vkAcquireNextImageKHR != NULL, g_dldi.vkQueuePresentKHR != NULL);
    }
    return 1;
}

void vk_destroy_device(void)
{
    if (g_device != VK_NULL_HANDLE)
        vkDestroyDevice(g_device, NULL);
    g_device = VK_NULL_HANDLE;
}
