#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include "engine/vk/device/instance.h"
#include <string.h>
#include "config.h"
#include "engine/vk/context.h"
#include "engine/vk/device/validation.h"
#include "engine/vk/device/debug_messenger.h"
#include "iostream/logger_bridge.h"

static int prepare_debug_utils( const char **base_exts,                             unsigned int base_count,
                                const char ***out_inst_exts,                        unsigned int *out_inst_ext_count,
                                VkDebugUtilsMessengerCreateInfoEXT *out_debug_ci,   int *out_debug_ci_present);

static int prepare_instance_extensions_and_layers(  int enable_validation,
                                                    const char **base_exts,             unsigned int base_count,
                                                    const char ***out_inst_exts,        unsigned int *out_inst_ext_count,
                                                    const char ***out_layers,           uint32_t *out_layer_count,
                                    VkDebugUtilsMessengerCreateInfoEXT *out_debug_ci,   int *out_debug_ci_present)
{
    if (!out_inst_exts || !out_inst_ext_count || !out_layers || !out_layer_count || !out_debug_ci || !out_debug_ci_present) return 0;
    
    /* default to base extensions */
    *out_inst_exts          = base_exts;
    *out_inst_ext_count     = base_count;
    *out_layers             = NULL;
    *out_layer_count        = 0;
    *out_debug_ci_present   = 0;
    memset(out_debug_ci, 0, sizeof(*out_debug_ci));

    if (!enable_validation) return 1; // if we don't want validation layer, stop here

    /* Validation layers */
    uint32_t vcount         = 0;
    const char **vlayers    = vk_validation_get_layers(&vcount);
    if (vcount > 0 && vlayers) {
        *out_layer_count    = vcount;
        *out_layers         = vlayers;
        ftpp_logf(FTPP_LOG_DEBUG, "Validation layer enabled: %s", vlayers[0]);
        ftpp_logf(FTPP_LOG_DEBUG, "Available validation layers:");
        for (uint32_t i = 0; i < vcount; ++i) ftpp_logf(FTPP_LOG_DEBUG, "  %s", vlayers[i]);
    } else {
        ftpp_logf(FTPP_LOG_WARNING, "Validation layer not available");
    }

    if (!prepare_debug_utils(base_exts, base_count, out_inst_exts, out_inst_ext_count, out_debug_ci, out_debug_ci_present)) {
        ftpp_logf(FTPP_LOG_ERROR, "Failed to prepare debug utils/extensions");
        return 0;
    }
    return 1;
}

static int prepare_debug_utils( const char **base_exts,         unsigned int base_count,
                                const char ***out_inst_exts,    unsigned int *out_inst_ext_count,
            VkDebugUtilsMessengerCreateInfoEXT *out_debug_ci,   int *out_debug_ci_present)
{
    if (!out_inst_exts || !out_inst_ext_count || !out_debug_ci || !out_debug_ci_present) return 0;
    int have_debug      = 0;
    for (unsigned int i = 0; i < base_count; ++i) {
        if (strcmp(base_exts[i], "VK_EXT_debug_utils") == 0) { have_debug = 1; break; }
    }
    if (!have_debug) {
        uint32_t iexts_count = 0;
        // hérésie btw.
        if (vkEnumerateInstanceExtensionProperties(NULL, &iexts_count, NULL) == VK_SUCCESS && iexts_count > 0) {
            VkExtensionProperties *iexts = malloc(sizeof(VkExtensionProperties) * iexts_count);
            if (iexts) {
                vkEnumerateInstanceExtensionProperties(NULL, &iexts_count, iexts);
                for (uint32_t i = 0; i < iexts_count; ++i) {
                    if (strcmp(iexts[i].extensionName, "VK_EXT_debug_utils") == 0) { have_debug = 1; break; }
                }
                free(iexts);
            }
        }

        if (have_debug) {
            const char **arr = malloc(sizeof(const char*) * (base_count + 1));
            if (arr) {
                for (unsigned int i = 0; i < base_count; ++i) arr[i] = base_exts[i];

                arr[base_count]         = "VK_EXT_debug_utils";
                *out_inst_exts          = arr;
                *out_inst_ext_count     = base_count + 1;
            } else {
                *out_inst_exts      = base_exts;
                *out_inst_ext_count = base_count;
            }
        } else {
            ftpp_logf(FTPP_LOG_WARNING, "VK_EXT_debug_utils not available on this system; debug utils disabled");
            *out_inst_exts          = base_exts;
            *out_inst_ext_count     = base_count;
        }
    } else {
        *out_inst_exts              = base_exts;
        *out_inst_ext_count         = base_count;
    }

    if (have_debug) {
        out_debug_ci->sType             = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        out_debug_ci->messageSeverity   = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        out_debug_ci->messageType       = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        out_debug_ci->pfnUserCallback   = vk_debug_callback;
        *out_debug_ci_present = 1;
    } else {
        *out_debug_ci_present = 0;
    }
    return 1;
}

int vk_instance_create(int enable_validation)
{
    unsigned int ext_count = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&ext_count);
    ftpp_logf(FTPP_LOG_DEBUG, "Required Vulkan extensions for GLFW:");
    for (unsigned int i = 0; i < ext_count; i++) {
        ftpp_logf(FTPP_LOG_DEBUG, "  > %s", exts[i]);
    }
    
    VkApplicationInfo ai    = {0};
    ai.sType                = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName     = APP_NAME;
    ai.applicationVersion   = APP_VERSION;
    ai.pEngineName          = ENGINE_NAME;
    ai.engineVersion        = ENGINE_VERSION;
    ai.apiVersion           = VULKAN_API_VERSION;
    ftpp_logf(FTPP_LOG_DEBUG, "Application info set: app '%s' v%d, engine '%s' v%d, API v%d", ai.pApplicationName, ai.applicationVersion, ai.pEngineName, ai.engineVersion, ai.apiVersion);
    
    VkInstanceCreateInfo ici    = {0};
    ici.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo        = &ai;
    const char **inst_exts      = NULL;
    unsigned int inst_ext_count = 0;

    VkDebugUtilsMessengerCreateInfoEXT debug_ci = {0};
    int debug_ci_present                        = 0;
    
    const char **enabled_layers = NULL;
    uint32_t enabled_layer_count = 0;
    if (!prepare_instance_extensions_and_layers(enable_validation, exts, ext_count,
                                                &inst_exts, &inst_ext_count, &enabled_layers, &enabled_layer_count,
                                                &debug_ci, &debug_ci_present)) {
        return 0;
    }

    ici.enabledLayerCount       = enabled_layer_count;
    ici.ppEnabledLayerNames     = enabled_layers;
    if (debug_ci_present)
        ici.pNext               = &debug_ci;
    ici.enabledExtensionCount   = inst_ext_count;
    ici.ppEnabledExtensionNames = inst_exts;

    ftpp_logf(FTPP_LOG_DEBUG, "Creating instance with %d extensions", ici.enabledExtensionCount);
    if (vkCreateInstance(&ici, NULL, &g_instance) != VK_SUCCESS) {
        if (inst_exts != exts && inst_exts) free((void*)inst_exts);
        return 0;
    }
    if (inst_exts != exts && inst_exts)
        free((void*)inst_exts);

    vk_load_dispatcher();
    if (enable_validation)
        vk_create_debug_messenger();
    return 1;
}

void vk_instance_destroy(void)
{
    /* Destroy debug messenger first if created */
    vk_destroy_debug_messenger();
    if (g_instance != VK_NULL_HANDLE) 
        vkDestroyInstance(g_instance, NULL);
    g_instance = VK_NULL_HANDLE;
}

int vk_surface_create(GLFWwindow *win)
{
    if (!g_instance) 
        return 0;
    return glfwCreateWindowSurface(g_instance, win, NULL, &g_surface) == VK_SUCCESS;
}

void vk_surface_destroy(void)
{
    if (g_surface != VK_NULL_HANDLE) 
        vkDestroySurfaceKHR(g_instance, g_surface, NULL);
    g_surface = VK_NULL_HANDLE;
}


