#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "engine/vk/device/debug_messenger.h"
#include "engine/vk/context.h"
#include "iostream/logger_bridge.h"

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT     messageSeverity,
                                                VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                void*                                       pUserData)
{
    (void)messageTypes; (void)pUserData;

    const char *msg = (pCallbackData && pCallbackData->pMessage) ? pCallbackData->pMessage : "(no message)";

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        ftpp_logf(FTPP_LOG_ERROR, "Vulkan validation error: %s", msg);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        ftpp_logf(FTPP_LOG_WARNING, "Vulkan validation warning: %s", msg);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        ftpp_logf(FTPP_LOG_INFO, "Vulkan validation info: %s", msg);
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        ftpp_logf(FTPP_LOG_DEBUG, "Vulkan validation verbose: %s", msg);
    } else {
        ftpp_logf(FTPP_LOG_DEBUG, "Vulkan validation unknown severity: %s", msg);
    }

    return VK_FALSE;
}

int vk_create_debug_messenger(void)
{
    if (g_instance == VK_NULL_HANDLE) return 0;
    
    VkDebugUtilsMessengerCreateInfoEXT ci = {0};
    ci.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback  = vk_debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT func = NULL;
    if (g_dldi.vkCreateDebugUtilsMessengerEXT)
        func = g_dldi.vkCreateDebugUtilsMessengerEXT;
    else 
        func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance, "vkCreateDebugUtilsMessengerEXT");
    
    if (!func) return 0;
    if (func(g_instance, &ci, NULL, &g_debug_messenger) != VK_SUCCESS) return 0;

    ftpp_logf(FTPP_LOG_DEBUG, "vk_create_debug_messenger: created messenger %p", (void*)g_debug_messenger);
    return 1;
}

void vk_destroy_debug_messenger(void)
{
    if (g_debug_messenger == VK_NULL_HANDLE) return;

    PFN_vkDestroyDebugUtilsMessengerEXT func = NULL;
    if (g_dldi.vkDestroyDebugUtilsMessengerEXT) 
        func = g_dldi.vkDestroyDebugUtilsMessengerEXT;
    else 
        func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance, "vkDestroyDebugUtilsMessengerEXT");
    
    if (func) func(g_instance, g_debug_messenger, NULL);
    g_debug_messenger = VK_NULL_HANDLE;
    
    ftpp_logf(FTPP_LOG_DEBUG, "vk_destroy_debug_messenger: destroyed messenger");
}

int vk_load_dispatcher(void)
{
    /* Clear table */
    g_dldi.vkCreateDebugUtilsMessengerEXT   = NULL;
    g_dldi.vkDestroyDebugUtilsMessengerEXT  = NULL;
    g_dldi.vkSetDebugUtilsObjectNameEXT     = NULL;
    g_dldi.vkCmdBeginDebugUtilsLabelEXT     = NULL;
    g_dldi.vkCmdEndDebugUtilsLabelEXT       = NULL;

    if (g_instance == VK_NULL_HANDLE)
        return 0;

    /* Load a few useful instance-level extension entrypoints (debug utils) */
    g_dldi.vkCreateDebugUtilsMessengerEXT   = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance,     "vkCreateDebugUtilsMessengerEXT");
    g_dldi.vkDestroyDebugUtilsMessengerEXT  = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_instance,    "vkDestroyDebugUtilsMessengerEXT");
    g_dldi.vkSetDebugUtilsObjectNameEXT     = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(g_instance,       "vkSetDebugUtilsObjectNameEXT");
    g_dldi.vkCmdBeginDebugUtilsLabelEXT     = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(g_instance,       "vkCmdBeginDebugUtilsLabelEXT");
    g_dldi.vkCmdEndDebugUtilsLabelEXT       = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(g_instance,         "vkCmdEndDebugUtilsLabelEXT");

    ftpp_logf(FTPP_LOG_DEBUG, "vk_load_dispatcher: vkCreateDebugUtilsMessengerEXT=%d vkDestroy=%d setName=%d beginLabel=%d endLabel=%d",
        g_dldi.vkCreateDebugUtilsMessengerEXT != NULL, g_dldi.vkDestroyDebugUtilsMessengerEXT != NULL,
        g_dldi.vkSetDebugUtilsObjectNameEXT != NULL, g_dldi.vkCmdBeginDebugUtilsLabelEXT != NULL, g_dldi.vkCmdEndDebugUtilsLabelEXT != NULL);
    return 1;
}
